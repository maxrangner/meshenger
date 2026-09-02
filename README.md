<h1 align="center">LoRa Meshenger</h1>

<p align="center">
  An emergency messaging device that uses private group-based mesh networking to share short preset status updates.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/status-active-brightgreen?style=flat-square" alt="status">
  <img src="https://img.shields.io/badge/ESP--IDF-v5.5.4-E7352C?style=flat-square&logo=espressif" alt="Tested with ESP-IDF v5.5.4">
</p>

## ◆ Features

| | | |
|---|---|---|
| ◆ **Custom lightweight protocol** | Small packets well suited for LoRa networking | WIP |
| ◆ **Mesh networking** | Relaying of packets | WIP |
| ◆ **Recovery of missed statuses** | Data loss from a temporary disconnect is healed by periodic network synchronization | Not implemented |


## ◆ Design

### Protocol

The network sends a serialized packet with:
- `groupId`,`deviceId`, `messageNum` and `payload`.

### Mesh network

Current design:
- Each packet includes a `payload` and a unique `messageId` comprised of `groupId` + `deviceId` + `messageNum`
- `messageNum` is saved in NVS to prevent the network thinking new updates are stale because of a reboot.
- Status updates are screened for freshness when a packet is received using the unique `messageId`.
- Each device holds the latest status updates from all devices within the local group in memory. This is used to compare which status updates should be discarded, or saved and relayed.

Known limitations:
- If a part of the group network is disconnected, it will not self heal. Each device has to update the status again to become visible to the previously disconnected part of the network. This might be fixed with a healing group sync at set intervals.
- Packet collision when two nodes relay a packet at the same time. Can be mitigated by introducing a randomized delay for retransmissions.
- There is currently no delivery guarantee.
- No encryption yet.
- A factory reset device can be read as stale until it catches up the old `messageNum`. Re-adding the device to clear the counter might be an early solution.