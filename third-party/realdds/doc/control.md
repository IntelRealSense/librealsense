
# Control

Controls are commands from a client to the server, e.g. start streaming, set option value etc...

## This is WIP


## Replies

All control messages need to be re-broadcast with a result, for other clients to be notified. The [notifications](notifications.md) topic is used for this.

I.e., when a control is sent, some way to identify a reply to that specific control must exist.

- Replies will have the same `id` as the original control. This means controls and notifications must have separate identifiers.

Additionally, when the server gets a control message, the sample will contain a GUID for its publisher (called the "publication handle"). This is the GUID of the writer used. The participant can be deduced from this and attached to a reply:

Additionally, a unique sequence number for each such sample is available from the `sample-identity`.

- Replies will contain a `sample` field containing the GUID of the participant from which the control request originated, plus the sequence-number, e.g.: `"sample": "010f6ad8708c95e000000000.4"`

Replies must include a success/failure status:

- Replies will include a `status` for anything other than `OK`
- Replies will include an `explanation` is status is not `OK`


## Errors
