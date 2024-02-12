
# Control

Controls are commands from a client to the server, e.g. start streaming, set option value etc...


## Replies

All control messages need to be re-broadcast with a result, for other clients to be notified and made aware. The [notifications](notifications.md) topic is used for this.

When a control is sent, some way to identify a reply to that specific control must exist:

- Replies will have the same `id` as the original control. This means **controls and notifications must have separate identifiers**
- The `id` may be retrieved from the attached `control` (see below) if available with `sample`

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
    "control": {
        "id": "query-option",
        "option-name": "opt16"
        },
    "sample": ["010f9a5f64d95fd300000000.403", 1],
    "status": "error",
    "explanation": "Test Device does not support option opt16"
}
```


### Combining Multiple Controls

Unlike [notifications](notifications.md), this is not possible.


## Control Messages


### `query-option` & `set-option`

Returns or sets the `value` of a single option within the device.

- `id` is `query-option` or `set-option`
- `option-name` is the option name, a string value
- `stream-name` is the name of the stream the option is in
    - For device options, this should be omitted
- for `set-option`, a `value` supplies the value to set
    - Note: the reply may provide a different value that was actually set

E.g.:
```JSON
{
    "id": "query-option",
    "option-name": "IP address"
}
```

The reply should include the original control plus a `value`:
```JSON
{
    "sample": ["010f9a5f64d95fd300000000.403", 1],
    "value": "1.2.3.4",
    "control": {
        "id": "query-option",
        "option-name": "IP address"
    }
}
```

The same exact behavior for `set-option` except a `value` is provided in the control:
```JSON
{
    "sample": ["010f9a5f64d95fd300000000.403", 1],
    "value": 10.0,
    "control": {
        "id": "set-option",
        "stream-name": "Color",
        "option-name": "Exposure",
        "value": 10.5
    }
}
```
New option values should conform to each specific option's value range as communicated when the device was [initialized](initialization.md).


### `query-options`: bulk queries

Like `query-option`, but asking for all option values at once, based on either stream, sensor, or device.

- For stream-specific options, use `stream-name`
    - For device-specific options, use `"stream-name": ""`
- For sensor-specific options, use `sensor-name`
- To globally get ALL options for all streams including device options, use neither

E.g.:
```JSON
{
    "id": "query-options",
    "stream-name": "Color"
}
```

A stream-name-to-option-name-to-value mapping called `option-values` is returned:
```JSON
{
    "sample": ["010f9a5f64d95fd300000000.403", 1],
    "option-values": {
        "IP address": "1.2.3.4",
        "Color": {
            "Exposure": 10.0,
            "Gain": 5
        },
        "Depth": {
            "Exposure": 15.0
        }
    },
    "control": {
        "id": "query-options"
    }
}
```
Device options are directly embedded in `option-values`; stream options are in hierarchies of their own.

#### periodic updates

Option values that are changed *by the server* (without a control message) are expected to send `query-options` notifications with no `control` or `sample` fields:
```JSON
{
    "id": "query-options",
    "option-values": {
        "Color": {
            "Exposure": 8.0,
        },
        "Depth": {
            "Exposure": 20.0
        }
    }
}
```

It is recommended this happens at least periodically (based on possible configuration setting?) to avoid the clients all having to query the device repeatedly.


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

* `data` is required

It is up to the server to validate and make sure everything is as expected.


```JSON
{
    "id": "hwm",
    "data": "1400abcd1000000000000000000000000000000000000000"
}
```

The above is a `GVD` HWM command.

A reply can be expected. Attaching the control is recommended so it's clear what was done. The reply `data` should be preset:

```JSON
{
    "sample": ["010f9a5f64d95fd300000000.403", 1],
    "control": {
        "id": "hwm",
        "data": "1400abcd1000000000000000000000000000000000000000"
        },
    "data": "10000000011004000e01ffffff01010102000f05000000000000000064006b0000001530ffffffffffffffffffffffffffffffff0365220706600000ffff3935343031300094230500730000ffffffffffffffff0aff3939394146509281a1ffffffffffffffffff9281a1ffffffffffffffffffffffffffffffffffffffffff1f0fffffffffffffffff000027405845ffffffffffffffff908907ffffff01ffffff050aff00260000000200000001000000010000000100000001000000000600010001000100030303020200000000000100070001ffffffffffffffffffffffffffffffffffff014a34323038362d31303001550400ae810100c50004006441050011d00000388401002e0000002dc00000ff"
}
```

#### A more verbose method

You may also instruct the server to **build the HWM** command if you have knowledge of the available opcodes and data needed:

* `opcode` (string) is required
* `param1`, `param2`, `param3`, `param4` are all 32-bit integers whose meaning depends on the `opcode`, and with a default of `0`
* The `data` field, in this case, points to the command data, if needed, and becomes part of the generated HWM

If an optional `build-command` field is present and `true`, then the reply `data` will contain the generated HWM command **without running it**.


#### `hexarray` type

A `hexarray` is a special encoding for a `bytearray`, or array of bytes, in JSON.

Rather than representing as `[0,100,2,255]`, a `hexarray` is instead represented as a string `"006402ff"` which is a hexadecimal representation of all the consecutive bytes, from index 0 onwards.

* Only lower-case alphabetical letters are used (`[0-9a-f]`)
* The length must be even (2 hex digits per byte)
* The first hex pair is for the first byte, the second hex pair for the second, onwards


### `dfu-start` and `dfu-apply`

These exist to support a device-update capability. They should be available even in recovery mode.

`dfu-start` starts the DFU process. The device will subscribe to a `dfu` topic under the topic root (accepting a `blob` message, reliable), and reply.

The client will then publish the binary image over the `dfu` topic.

Once the image is verified, the device will send back a notification, `dfu-ready`. If not `OK`, then the device is expected to go back to the pre-DFU state and the process needs to start over. The `dfu` subscription can be removed at this time.

When ready, a client can then send a `dfu-apply`. The device will reply then take whatever time is needed (progress notifications are recommended) to have the image take effect. The device will reply, send a disconnection on `device-info`, and perform a hardware-reset after applying the image.
