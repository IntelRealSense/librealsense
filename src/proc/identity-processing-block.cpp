#include "identity-processing-block.h"
#include "image.h"

librealsense::identity_processing_block::identity_processing_block() :
    identity_processing_block("Identity processing block")
{
}

librealsense::identity_processing_block::identity_processing_block(const char * name) :
    stream_filter_processing_block(name)
{
}

rs2::frame librealsense::identity_processing_block::process_frame(const rs2::frame_source & source, const rs2::frame & f)
{
    return f;
}
