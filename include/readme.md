# librealsense2 API

# File-System Structure

Under `librealsense2` folder you will find two subfolers:
* [h](./librealsense2/h) - Contains headers for the C language
* [hpp](./librealsense2/hpp) - Contains headers for the C++ language, depends on C headers

In addition, you can include [<librealsense2/rs.h>](./librealsense2/rs.h) and [<librealsense2/rs.hpp>](./librealsense2/rs.hpp) to get most of SDK functionality in C and C++ respectively. 

[<librealsense2/rs_advanced_mode.h>](./librealsense2/rs_advanced_mode.h) and [<librealsense2/rs_advanced_mode.hpp>](./librealsense2/rs_advanced_mode.hpp) can be included to get the extra [Advanced Mode](../doc/rs400_advanced_mode.md) functionality (in C and C++ respectively).

[<librealsense2/rsutil.h>](./librealsense2/rsutil.h) contains mathematical helper functions for projection of 2D points into 3D space and back. 

# Files and Classes

## [rs_types.hpp](librealsense2/hpp/rs_types.hpp):
|Class|Description|
|-----|-----------|
|[error](librealsense2/hpp/rs_types.hpp#L76)| |
|[recoverable_error](librealsense2/hpp/rs_types.hpp#L113)| |
|[unrecoverable_error](librealsense2/hpp/rs_types.hpp#L114)| |
|[camera_disconnected_error](librealsense2/hpp/rs_types.hpp#L115)| |
|[backend_error](librealsense2/hpp/rs_types.hpp#L116)| |
|[device_in_recovery_mode_error](librealsense2/hpp/rs_types.hpp#L117)| |
|[invalid_value_error](librealsense2/hpp/rs_types.hpp#L118)| |
|[wrong_api_call_sequence_error](librealsense2/hpp/rs_types.hpp#L119)| |
|[not_implemented_error](librealsense2/hpp/rs_types.hpp#L120)| |

## [rs_context.hpp](librealsense2/hpp/rs_context.hpp):
|Class|Description|
|-----|-----------|
|[context](librealsense2/hpp/rs_context.hpp#L81)| |
|[device_hub](librealsense2/hpp/rs_context.hpp#L185)| |
|[event_information](librealsense2/hpp/rs_context.hpp#L16)| |

## [rs_device.hpp](librealsense2/hpp/rs_device.hpp):
|Class|Description|
|-----|-----------|
|[device](librealsense2/hpp/rs_device.hpp#L109)| |
|[device_list](librealsense2/hpp/rs_device.hpp#L186)| |
|[debug_protocol](librealsense2/hpp/rs_device.hpp#L151)| |

## [rs_processing.hpp](librealsense2/hpp/rs_processing.hpp):
|Class|Description|
|-----|-----------|
|[syncer](librealsense2/hpp/rs_processing.hpp#L260)| |
|[frame_queue](librealsense2/hpp/rs_processing.hpp#L136)| |
|[processing_block](librealsense2/hpp/rs_processing.hpp#L105)| |
|[pointcloud](librealsense2/hpp/rs_processing.hpp#L196)| |
|[asynchronous_syncer](librealsense2/hpp/rs_processing.hpp#L232)| |
|[align](librealsense2/hpp/rs_processing.hpp#L316)| |
|[colorizer](librealsense2/hpp/rs_processing.hpp#L356)| |
|[decimation_filter](librealsense2/hpp/rs_processing.hpp#L391)| |
|[temporal_filter](librealsense2/hpp/rs_processing.hpp#L428)| |
|[spatial_filter](librealsense2/hpp/rs_processing.hpp#L465)| |

## [rs_sensor.hpp](librealsense2/hpp/rs_sensor.hpp):
|Class|Description|
|-----|-----------|
|[roi_sensor](librealsense2/hpp/rs_sensor.hpp#L446)| |
|[sensor](librealsense2/hpp/rs_sensor.hpp#L392)| |
|[notification](librealsense2/hpp/rs_sensor.hpp#L15)| |
|[depth_sensor](librealsense2/hpp/rs_sensor.hpp#L479)| |

## [rs_frame.hpp](librealsense2/hpp/rs_frame.hpp):
|Class|Description|
|-----|-----------|
|[frame](librealsense2/hpp/rs_frame.hpp#L157)| |
|[points](librealsense2/hpp/rs_frame.hpp#L423)| |
|[stream_profile](librealsense2/hpp/rs_frame.hpp#L24)| |
|[video_stream_profile](librealsense2/hpp/rs_frame.hpp#L113)| |
|[video_frame](librealsense2/hpp/rs_frame.hpp#L348)| |
|[depth_frame](librealsense2/hpp/rs_frame.hpp#L480)| |
|[frameset](librealsense2/hpp/rs_frame.hpp#L502)| |
|[iterator](librealsense2/hpp/rs_frame.hpp#L594)| |

## [rs_pipeline.hpp](librealsense2/hpp/rs_pipeline.hpp):
|Class|Description|
|-----|-----------|
|[pipeline_profile](librealsense2/hpp/rs_pipeline.hpp#L22)| |
|[pipeline](librealsense2/hpp/rs_pipeline.hpp#L345)| |
|[config](librealsense2/hpp/rs_pipeline.hpp#L128)| |

## [rs_record_playback.hpp](librealsense2/hpp/rs_record_playback.hpp):
|Class|Description|
|-----|-----------|
|[playback](librealsense2/hpp/rs_record_playback.hpp#L30)| |
|[recorder](librealsense2/hpp/rs_record_playback.hpp#L206)| |

## [rs_internal.hpp](librealsense2\hpp\rs_internal.hpp):
|Class|Description|
|-----|-----------|
|[recording_context](librealsense2\hpp\rs_internal.hpp#L19)| |
|[mock_context](librealsense2\hpp\rs_internal.hpp#L41)| |
