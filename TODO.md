# TODO
## MeshService is untestable
Solution: Separate MeshService into MeshCore and thin layer MeshService

## Payload vocabulary index can be out of range
Solution: Validate received packets. Size, values and version.

## Lost radio interrupt can get radio stuck
Solution: Check return from queue send and include timers. If interrupt has no response, handle error. Detect unfinished routes.

## NVS data load and save is not fully implemented
Solution: Finish implementation

## Versions are ambiguous
Soultion: Separate protocol, storage and vocabulary versions. Set up checks for them all.

## Utils owns protocol features
Solution: Move serialization/deserialization of data to protocol.

## Mesh initiates device features
Move NVS init to AppController init.

## MAC set in wrong place
Solution: Move setting of MAC into separate default device init function.

## Add host tests

## Adjust task priority

---

## MeshService::send_payload() should return bool, and AppController check if request was accepted into queue