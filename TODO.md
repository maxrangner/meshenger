# TODO
## MeshService is untestable
Solution: Separate MeshService into MeshCore and thin layer MeshService

## Payload vocabulary index can be out of range
Solution: Validate received packets. Size, values and version.

## Add host tests

## Add non-blocking random delay to relays.

## handle_received_frame() does not check return on relay_packet()

---

## MeshService::send_payload() should return bool, and AppController check if request was accepted into queue