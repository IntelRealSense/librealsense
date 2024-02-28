
The files in this directory are, for the most part, automatically generated from the IDL.

See the [`topics`](../) readme for generation, etc.


# Flexible

This message format can convey any data in any format, as long as it is bounded by the maximum size defined by the IDL, currently 4K bytes:

```idl
struct flexible
{
    flexible_data_format data_format;
    octet version[4];
    sequence< octet, 4096 > data;
};
```

The format is usually JSON, but can be other formats (CUSTOM and CBOR are currently defined but not really in use).

The version is currently always set to 0.

Assuming JSON format, the data would then be the text representation (1 byte per character) of said JSON.

If CBOR, it would be the binary representation of the JSON data.
If CUSTOM, the bytes are to be interpreted by the client using its own data structures.


## Topic type

The DDS topic type is:
>realdds::topics::raw::flexible


## Quality of Service

All flexible topics are usually reliable (as opposed to data streams that are best-effort), unless otherwise stated. One notable exception is [metadata](../../../../doc/metadata.md).

- Reliability: `RELIABLE`
- Durability: `VOLATILE`
