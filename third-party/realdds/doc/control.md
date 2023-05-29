
# Control

Controls are commands from a client to the server, e.g. start streaming, set option value etc...

## This is WIP


## Replies

All control messages need to be re-broadcast with a result, for other clients to be notified. The [notifications](notifications.md) topic is used for this.

I.e., when a control is sent, some way to identify a reply to that specific control must exist.

- Replies will have the same `id` as the original control. This means controls and notifications must have separate identifiers.

Additionally, when the server gets a control message, the DDS sample will contain a "publication handle". This is the GUID of the writer (including the participant) to identify the control source. A unique sequence number for each control sample is available from the `sample-identity`.

- Replies will contain a `sample` field containing the GUID of the participant from which the control request originated, plus the sequence-number, e.g.: `"sample": ["010f6ad8708c95e000000000.303",4]`

Replies must include a success/failure status:

- Replies will include a `status` for anything other than `OK`
- Replies will include an `explanation` if status is not `OK`

Replies may also contain the original control request:

- Optionally (depending on the control), a `control` field in the reply should contain the original control request (rather than copying many fields, for example)


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
- `owner-name` is the name of the owner
    - for a device option, this may be omitted
    - for a stream option, this is the unique stream name within the device
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
