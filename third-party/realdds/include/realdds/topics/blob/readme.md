
The files in this directory are, for the most part, automatically generated from the IDL.

See the [`topics`](../) readme for generation, etc.


# blob

This message format can convey any data in any format:

```idl
struct blob
{
    sequence< octet > data;
};
```

This can be used for anything, including big data transfers. It is better to use [flexible](../flexible/readme.md) If your data can fit within its limits.

## Topic type

The DDS topic type is:
>udds::blob


## Quality of Service

QoS depends on usage.
