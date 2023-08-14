
# TBD


**realsense/\<model\>/\<serial\>/metadata** of type flexible - metadata for frames of active streams.


## Metadata

Metadata samples are refering to a specific, streaming, stream. They include some mandatory fields in the header, i.e. frame number and timestamp, and other fields that are relevant to the specific frame and its type, e.g, "exposure" for images.

    "stream-name":"color",
    "header":{"frame-number":1234, "timestamp":123456789, "timestamp-domain":0},
    "metadata":{"Exposure":"123", "Gain":"456"}
