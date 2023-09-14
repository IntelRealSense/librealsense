
# Control

Controls are commands from a client to the server, e.g. start streaming, set option value etc...


## Replies

All control messages need to be re-broadcast with a result, for other clients to be notified and made aware. The [notifications](notifications.md) topic is used for this.

When a control is sent, some way to identify a reply to that specific control must exist:

- Replies will have the same `id` as the original control. This means **controls and notifications must have separate identifiers**.

Additionally, when the server gets a control message, the DDS sample will contain a "publication handle". This is the `GUID` of the writer (including the participant) to identify the control source. A unique sequence number for each control sample is available from the `sample-identity`. Without these, it is impossible to identify the origin of the control and therefore, whether we're waiting on it or not.

- Replies must contain a `sample` field containing the GUID of the participant from which the control request originated, plus the sequence-number, e.g.: `"sample": ["010f6ad8708c95e000000000.303",4]`

Replies must also include a success/failure status:

- Replies will include a `status` for anything other than `OK`
- Replies will include an `explanation` if status is not `OK`

It is recommended that replies also contain the original control request:

- A `control` field in the reply should contain the original control request, though some freedom exists depending on each control

Replies must function even in recovery mode.


### Errors

An error is a reply where the `status` is not OK:

- the `explanation` is a human-readable string explaining the failure (like an exception's `what`)

```JSON
{
    "id": "query-option",
    "sample": ["010f9a5f64d95fd300000000.403", 1],
    "control": {
        "id": "query-option",
        "option-name": "opt16"
        },
    "status": "error",
    "explanation": "Test Device does not support option opt16"
}
```


### Combining Multiple Controls

Unlike [notifications](notifications.md), this is not possible.


## Control Messages

### `query-option` & `set-option`

Returns or sets the current `value` of an option within a device.

- `id` is `query-option` or `set-option`
- `option-name` is the unique option name within its owner
- `stream-name` is the unique name of the stream it's in within the device
    - for a device option, this may be omitted
- for `set-option`, `value` is the new value (float)

```JSON
{
    "id": "query-option",
    "option-name": "opt-name"
}
```

The reply should include the original control, plus:

- `value` will include the current or new value

```JSON
{
    "id": "query-option",
    "sample": ["010f9a5f64d95fd300000000.403", 1],
    "value": 0.5,
    "control": {
        "id": "query-option",
        "option-name": "opt-name"
        }
}
```

Both querying and setting options involve a very similar reply that can be handled in the same manner.

A new option value should conform to the specific option's value range as communicated when the device was [initialized](initialization.md).


### `hw-reset`

Can be used to cause the server to perform a "hardware reset", if available, bringing it back to the same state as after power-up.

```JSON
{
    "id": "hw-reset"
}
```

* If boolean `recovery` is set, then the device will reset to recovery mode (error if this is not possible)

A reply can be expected.

A [disconnection event](discovery.md#disconnection) can be expected if the reply is a success.


### `hwm`

Can be used to send internal commands to the hardware and may brick the device if used. May or may not be implemented, and is not documented.

* `opcode` (string) is suggested

Plus any additional fields necessary. It is up to the server to validate and make sure everything is as expected.

```JSON
{
    "id": "hwm",
    "opcode": "WWD",
    "data": ["kaboom"]
}
```

A reply can be expected. Attaching the control is recommended so it's clear what was done.


### `dfu-start` and `dfu-apply`

These exist to support a device-update capability. They should be available even in recovery mode.

`dfu-start` starts the DFU process. The device will subscribe to a `dfu` topic under the topic root (accepting a `blob` message, reliable), and reply.

The client will then publish the binary image over the `dfu` topic.

Once the image is verified, the device will send back a notification, `dfu-ready`. If not `OK`, then the device is expected to go back to the pre-DFU state and the process needs to start over. The `dfu` subscription can be removed at this time.

When ready, a client can then send a `dfu-apply`. The device will reply then take whatever time is needed (progress notifications are recommended) to have the image take effect. The device will reply, send a disconnection on `device-info`, and perform a hardware-reset after applying the image.
