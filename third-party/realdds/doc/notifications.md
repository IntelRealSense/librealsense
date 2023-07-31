
See also: [device](device.md), [initialization](initialization.md)


# Notifications

A topic is used to deliver [flexible](../include/realdds/topics/flexible/) messages from a server to its clients.

> `<topic-root>` /notification

These notifications can fall into many types, encompass almost any data, and happen at ANY time. Several are pre-defined and listed below.

Notifications should always be a JSON (possibly CBOR) object (`{'x': true}` and not just `true`) and identified by an `id` field inside, e.g.:

```JSON
{
    "id": "some-message-id",
    "message": "this is a field value"
}
```

Notification content is dependent on the `id`. Fields that are not recognized are ignored by the client. Notifications that are unrecognized are ignored. This allows the server to include additional content as needed.

Messages are broadcast to all clients rather than to a specific one.


#### Quality of Service

- Reliability: `RELIABLE`
- Durability: `VOLATILE`


### Logging

As part of the usual runtime of the server, conditions may warrant some kind of output for logging purposes, as is customary in software products.

These may be enabled or disabled via some mechanism, but it is assumed that some basic level of logging will always happen, even if only to communicate error details, state changes, etc.

These logs are output as notifications.


#### `log`

Multiple log entries can be packaged together. It is up to the server to decide the frequency of notification. It is recommended that logs be flushed prior to any other notification being sent out, to maintain temporal cohesion in the logs.

- `entries` is an array containing 1 or more log entries
    - Each log entry is a JSON array of `[timestamp, type, text, data]` containing:
        - `timestamp`: when the event occurred (see [timestamps](#timestamps))
        - `type`: one of `EWID` (Error, Warning, Info, Debug)
        - `text`: any text that needs output
        - `data`: optional; an object containing any pertinent information about the event

```JSON
{
    "id": "log",
    "entries": [
        [1234567890, "E", "Error message", { "file": "some.cpp", "line-number": 35 }],
        [1234567892, "W", "Message"],
        ...
    ]
}
```


### Timestamps

Timestamps are communicated as nanoseconds. DDS timestamps are usually in the form of (seconds,nanoseconds), so:
>timestamp = seconds * 1000000000ull + nanoseconds

On Fast-DDS, this is the time offset since last epoch, defined as:
`system_clock::now().time_since_epoch()`. It is up to the server to define this offset, as long as the units are nanoseconds.

The C++ type should be `unsigned long long`.


### Combining Multiple Notifications

Combining multiple log notifications can be done by packaging them together into an array:

```JSON
[
    { "id": "log", "entries": [ ... ] },
    { "id": "notification-2", ... }
]
```


### Notification Source

Anybody can write to the notifications topic; but only a single participant is ever the real device - this is the participant from which the [device-info](discovery.md) was published.

It is recommended that the device implementation on the Client ignore notifications that do not originate from this participant.
