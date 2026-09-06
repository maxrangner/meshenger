# Display design

Architecture for the two displays on the Heltec WiFi LoRa 32 V4
(HTIT-WB32LA, ESP32-S3R2 + SX1262, ESP-IDF 5.5.4).

| Panel | Type | Bus | Role |
|---|---|---|---|
| Built-in OLED | SSD1306, 128x64 mono | I2C, addr `0x3C` | Drawn by us |
| External TFT | Colour, not yet on hand | SPI3 | Drawn by LVGL |

## Layers

```
components/display/     panel hardware        no app knowledge
        |
components/ui/          what is drawn         depends on display + protocol
        |
main/                   composition root      wiring only
```

Dependency direction is strictly upward. `display` never depends on `ui`,
`protocol` or anything above it, so it stays reusable.

`main/` holds only `application` and `app_log`. Everything with substance lives
in a component, even when it is app-specific — a component is just a static
library, and depending on `protocol` from `ui` is not a layering violation.

## Classes

| Panel | Hardware — `components/display/` | UI — `components/ui/` |
|---|---|---|
| OLED | `OledDisplay` | `OledUi` |
| TFT | `TftDisplay` | `TftUi` |

Four classes, two per panel. The split is the same on both sides: a hardware
class that knows pixels and a UI class that knows content.

The two UI classes share a name suffix but no base class and no interface. One
draws by hand, the other delegates to LVGL. They have nothing in common to
abstract, so there are no virtuals anywhere in this design.

### Responsibilities

**`OledDisplay`** — I2C panel IO, panel handle, Vext, framebuffer, drawing
primitives (clear, text, rect), `flush()`. Knows nothing about the app.

**`TftDisplay`** — SPI device, panel handle, backlight. **No drawing API.** It
brings the panel up and exposes its `esp_lcd` handles for `esp_lvgl_port`.

**`OledUi`** — owns an `OledDisplay&`. Turns app state into primitive calls.

**`TftUi`** — owns a `TftDisplay&` and all LVGL state, privately. Menus,
sub-menus, navigation and animation live in LVGL's own screen tree.

Both UI classes expose **intent-level** methods — "show status", "show message",
"go back" — never one method per widget or per view. A method per displayable
thing grows without bound and pushes layout decisions back into the caller.

## Ownership and wiring

`Application` creates the buses and passes them down:

```
Application
  ├── i2c_master_bus_handle_t ──> OledDisplay ──> OledUi
  └── spi_host SPI3           ──> TftDisplay  ──> TftUi
```

- A **bus** is a board resource. Created once in `Application`, passed in.
- A **device binding** (panel IO / SPI device) belongs to the display that uses
  it. Each display creates its own from the bus it was given.
- `init()` takes a config struct, not a bare handle. Each panel needs its
  address or CS, reset pin, and geometry.

**Vext caveat:** GPIO36 powers the OLED rail *and* the GNSS socket. `OledDisplay`
owns it for now because nothing else uses it. If GNSS is added, Vext moves up to
`Application` as a shared board resource.

## Threading

| Panel | Task | Notes |
|---|---|---|
| OLED | `app_task` | ~25 ms per full flush at 400 kHz. No separate task. |
| TFT | LVGL's own | `esp_lvgl_port` creates it. |

LVGL is **not thread-safe**. Every call from another task must be wrapped in
`lvgl_port_lock()` / `lvgl_port_unlock()`. `TftUi` keeps all LVGL objects private
so that discipline lives in one class instead of scattered across the codebase.

LVGL widget callbacks fire on the LVGL task. They post `AppEvent`s to the
existing queue rather than calling app logic directly.

## Bus assignment

The TFT goes on **SPI3_HOST**. SPI2 belongs to the SX1262 — `EspHal` defaults to
`SPI2_HOST` and `RadioService` does not override it.

Why not share SPI2:

- RadioLib sets `max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE` (64 on the S3)
  when it initialises the bus. Whoever initialises first sets that ceiling for
  every device on the host.
- `EspHal::spiBeginTransaction` calls `spi_device_acquire_bus(..., portMAX_DELAY)`
  and holds it for the whole transaction. Display flushes would queue behind
  LoRa traffic.

Constraint: SPI3 has no IOMUX pins, so it always routes through the GPIO matrix,
capping at 40 MHz instead of 80. Irrelevant for a write-only display — a
240x320 16bpp frame is ~154 KB, about 31 ms at 40 MHz.

### Pins

| Use | Pins |
|---|---|
| SCLK / MOSI / CS | **33, 47, 48** — adjacent on J2 |
| DC / RST | 45, 46, 15, 16, 3, 4, 6 |

Avoid GPIO 26 (PSRAM chip select) and GPIO 5 (`pa_ctx`, the RF switch driven by
RadioLib's mode table).

Reference — occupied pins: 8-14 LoRa, 17/18/21 OLED, 19/20 USB, 43/44 UART0,
0 PRG, 1 VBAT, 2 FEM_EN, 7 VFEM_Ctrl, 34 VGNSS_Ctrl, 35 LED, 36 Vext, 38-42 GNSS.

## Naming rules

- Name for what a thing **is** (its panel, its layer), never for what it might be
  used for. `OledDisplay`, not `DiagnosticDisplay` — the OLED's final purpose is
  undecided and a wrong name outlives misplaced code.
- `display` = hardware. `ui` = what is drawn. Both are certain.
- Prefix by panel, so the four classes map onto two panels at a glance.

## Open items

- Add a finite-timeout `i2c_master_probe` guard before handing the bus to
  `esp_lcd`. `esp_lcd` hardcodes a `-1` timeout, which maps to `portMAX_DELAY` —
  a stalled bus hangs forever with no error and no crash. This is the only point
  in the path where the timeout is ours to set.
- Add `esp_lcd_panel_mirror(panel, true, true)`. IDF's SSD1306 init sends
  `0xA0` / `0xC0`; Heltec mounts the panel inverted.
- Raise `CONFIG_LOG_MAXIMUM_LEVEL` to DEBUG during bring-up. At INFO the I2C and
  `esp_lcd` diagnostics are compiled out, not merely filtered.
- Enable PSRAM before the colour framebuffer needs it. The board has 2 MB
  (ESP32-S3R2) and `CONFIG_SPIRAM is not set`. 154 KB is uncomfortable in 314 KB
  of DRAM.
- Verify GPIO46 against the v4.3.1 schematic. Heltec's published pinmap shows
  GPIO5 as free and `FEM_PA` on GPIO46, but GPIO5 is in use as `pa_ctx` — the map
  is stale for this revision, so treat GPIO46 as unconfirmed.

## Build order

1. Extract `OledDisplay` from `application.cpp` into `components/display/`.
2. Add `OledUi` in `components/ui/` and draw something real.
3. `TftDisplay`, `TftUi` and LVGL when the hardware arrives.
