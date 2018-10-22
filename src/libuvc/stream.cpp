/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (C) 2010-2012 Ken Tossell
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the author nor other contributors may be
*     used to endorse or promote products derived from this software
*     without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/
/**
 * @defgroup streaming Streaming control functions
 * @brief Tools for creating, managing and consuming video streams
 */

#include "libuvc.h"
#include "libuvc_internal.h"
#include "errno.h"
#include <chrono>

#ifdef _MSC_VER

#define DELTA_EPOCH_IN_MICROSECS  116444736000000000Ui64

// gettimeofday - get time of day for Windows;
// A gettimeofday implementation for Microsoft Windows;
// Public domain code, author "ponnada";
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag = 0;
    if (NULL != tv)
    {
        GetSystemTimeAsFileTime(&ft);
        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;
        tmpres /= 10;
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }
    return 0;
}
#endif // _MSC_VER
uvc_frame_desc_t *uvc_find_frame_desc_stream(uvc_stream_handle_t *strmh,
    uint16_t format_id, uint16_t frame_id);
uvc_frame_desc_t *uvc_find_frame_desc(uvc_device_handle_t *devh,
    uint16_t format_id, uint16_t frame_id);
void *_uvc_user_caller(void *arg);
void _uvc_populate_frame(uvc_stream_handle_t *strmh);

struct format_table_entry {
  enum uvc_frame_format format;
  uint8_t abstract_fmt;
  uint8_t guid[16];
  int children_count;
  enum uvc_frame_format *children;
};

struct format_table_entry *_get_format_entry(enum uvc_frame_format format) {
  #define ABS_FMT(_fmt, _num, ...) \
    case _fmt: { \
    static enum uvc_frame_format _fmt##_children[] = __VA_ARGS__; \
    static struct format_table_entry _fmt##_entry = { \
      _fmt, 0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, _num, _fmt##_children }; \
    return &_fmt##_entry; }

  #define FMT(_fmt, ...) \
    case _fmt: { \
    static struct format_table_entry _fmt##_entry = { \
      _fmt, 0, __VA_ARGS__, 0, NULL }; \
    return &_fmt##_entry; }

  switch(format) {
    /* Define new formats here */
    ABS_FMT(UVC_FRAME_FORMAT_ANY, 2,
      {UVC_FRAME_FORMAT_UNCOMPRESSED, UVC_FRAME_FORMAT_COMPRESSED})

    ABS_FMT(UVC_FRAME_FORMAT_UNCOMPRESSED, 3,
      {UVC_FRAME_FORMAT_YUYV, UVC_FRAME_FORMAT_UYVY, UVC_FRAME_FORMAT_GRAY8})
    FMT(UVC_FRAME_FORMAT_YUYV,
      {'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
    FMT(UVC_FRAME_FORMAT_UYVY,
      {'U',  'Y',  'V',  'Y', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
    FMT(UVC_FRAME_FORMAT_GRAY8,
      {'Y',  '8',  '0',  '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
    FMT(UVC_FRAME_FORMAT_BY8,
      {'B',  'Y',  '8',  ' ', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})

    ABS_FMT(UVC_FRAME_FORMAT_COMPRESSED, 1,
      {UVC_FRAME_FORMAT_MJPEG})
    FMT(UVC_FRAME_FORMAT_MJPEG,
      {'M',  'J',  'P',  'G'})

    default:
      return NULL;
  }

  #undef ABS_FMT
  #undef FMT
}

static uvc_streaming_interface_t *_uvc_get_stream_if(uvc_device_handle_t *devh, int interface_idx);

static uint8_t _uvc_frame_format_matches_guid(enum uvc_frame_format fmt, uint8_t guid[16]) {
  struct format_table_entry *format;
  int child_idx;

  format = _get_format_entry(fmt);
  if (!format)
    return 0;

  if (!format->abstract_fmt && !memcmp(guid, format->guid, 16))
    return 1;

  for (child_idx = 0; child_idx < format->children_count; child_idx++) {
    if (_uvc_frame_format_matches_guid(format->children[child_idx], guid))
      return 1;
  }

  return 0;
}

static enum uvc_frame_format uvc_frame_format_for_guid(uint8_t guid[16]) {
  struct format_table_entry *format;
  int fmt;

  for (fmt = 0; fmt < UVC_FRAME_FORMAT_COUNT; ++fmt) {
    format = _get_format_entry((uvc_frame_format)fmt);
    if (!format || format->abstract_fmt)
      continue;
    if (!memcmp(format->guid, guid, 16))
      return format->format;
  }

  return UVC_FRAME_FORMAT_UNKNOWN;
}

/** @internal
 * Run a streaming control query
 * @param[in] devh UVC device
 * @param[in,out] ctrl Control block
 * @param[in] probe Whether this is a probe query or a commit query
 * @param[in] req Query type
 */
uvc_error_t uvc_query_stream_ctrl(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uint8_t probe,
    enum uvc_req_code req) {
  uint8_t buf[48];
  size_t len;
  int err;

  memset(buf, 0, sizeof(buf));

  switch(devh->info->ctrl_if.bcdUVC) {
      case 0x0110:
          len = 34;
          break;
      case 0x0150:
          len = 48;
          break;
      default:
          len = 26;
          break;
  }

  /* prepare for a SET transfer */
  if (req == UVC_SET_CUR) {
    SHORT_TO_SW(ctrl->bmHint, buf);
    buf[2] = ctrl->bFormatIndex;
    buf[3] = ctrl->bFrameIndex;
    INT_TO_DW(ctrl->dwFrameInterval, buf + 4);
    SHORT_TO_SW(ctrl->wKeyFrameRate, buf + 8);
    SHORT_TO_SW(ctrl->wPFrameRate, buf + 10);
    SHORT_TO_SW(ctrl->wCompQuality, buf + 12);
    SHORT_TO_SW(ctrl->wCompWindowSize, buf + 14);
    SHORT_TO_SW(ctrl->wDelay, buf + 16);
    INT_TO_DW(ctrl->dwMaxVideoFrameSize, buf + 18);
    INT_TO_DW(ctrl->dwMaxPayloadTransferSize, buf + 22);
    INT_TO_DW(0, buf + 18);
    INT_TO_DW(0, buf + 22);

    if (len >= 34) {
      INT_TO_DW ( ctrl->dwClockFrequency, buf + 26 );
      buf[30] = ctrl->bmFramingInfo;
      buf[31] = ctrl->bPreferredVersion;
      buf[32] = ctrl->bMinVersion;
      buf[33] = ctrl->bMaxVersion;
      /** @todo support UVC 1.1 */
    }

    if (len == 48) {
        buf[34] = ctrl->bUsage;
        buf[35] = ctrl->bBitDepthLuma;
        buf[36] = ctrl->bmSettings;
        buf[37] = ctrl->bMaxNumberOfRefFramesPlus1;
        SHORT_TO_SW(ctrl->bmRateControlModes, buf + 38);
        QUAD_TO_QW(ctrl->bmLayoutPerStream, buf + 40);

    }

  }

  /* do the transfer */
  err = libusb_control_transfer(
      devh->usb_devh,
      req == UVC_SET_CUR ? 0x21 : 0xA1,
      req,
      probe ? (UVC_VS_PROBE_CONTROL << 8) : (UVC_VS_COMMIT_CONTROL << 8),
      ctrl->bInterfaceNumber,
      buf, len, 5000
  );

  if (err <= 0) {
    return (uvc_error_t)err;
  }

  /* now decode following a GET transfer */
  if (req != UVC_SET_CUR) {
    ctrl->bmHint = SW_TO_SHORT(buf);
    ctrl->bFormatIndex = buf[2];
    ctrl->bFrameIndex = buf[3];
    ctrl->dwFrameInterval = DW_TO_INT(buf + 4);
    ctrl->wKeyFrameRate = SW_TO_SHORT(buf + 8);
    ctrl->wPFrameRate = SW_TO_SHORT(buf + 10);
    ctrl->wCompQuality = SW_TO_SHORT(buf + 12);
    ctrl->wCompWindowSize = SW_TO_SHORT(buf + 14);
    ctrl->wDelay = SW_TO_SHORT(buf + 16);
    ctrl->dwMaxVideoFrameSize = DW_TO_INT(buf + 18);
    ctrl->dwMaxPayloadTransferSize = DW_TO_INT(buf + 22);

    if (len >= 34) {
      ctrl->dwClockFrequency = DW_TO_INT ( buf + 26 );
      ctrl->bmFramingInfo = buf[30];
      ctrl->bPreferredVersion = buf[31];
      ctrl->bMinVersion = buf[32];
      ctrl->bMaxVersion = buf[33];
      /** @todo support UVC 1.1 */
    }

    if ( len == 48) {
        ctrl->bUsage = buf[34];
        ctrl->bBitDepthLuma = buf[35];
        ctrl->bmSettings = buf[36];
        ctrl->bMaxNumberOfRefFramesPlus1 = buf[37];
        ctrl->bmRateControlModes = DW_TO_INT(buf + 38);
        ctrl->bmLayoutPerStream = QW_TO_QUAD(buf + 40);


    }
    else
      ctrl->dwClockFrequency = devh->info->ctrl_if.dwClockFrequency;

    /* fix up block for cameras that fail to set dwMax* */
    if (ctrl->dwMaxVideoFrameSize == 0) {
      uvc_frame_desc_t *frame = uvc_find_frame_desc(devh, ctrl->bFormatIndex, ctrl->bFrameIndex);

      if (frame) {
        ctrl->dwMaxVideoFrameSize = frame->dwMaxVideoFrameBufferSize;
      }
    }
  }

  return UVC_SUCCESS;
}

/** @brief Reconfigure stream with a new stream format.
 * @ingroup streaming
 *
 * This may be executed whether or not the stream is running.
 *
 * @param[in] strmh Stream handle
 * @param[in] ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 */
uvc_error_t uvc_stream_ctrl(uvc_stream_handle_t *strmh, uvc_stream_ctrl_t *ctrl) {
  uvc_error_t ret;

  if (strmh->stream_if->bInterfaceNumber != ctrl->bInterfaceNumber)
    return UVC_ERROR_INVALID_PARAM;

  /* @todo Allow the stream to be modified without restarting the stream */
  if (strmh->running)
    return UVC_ERROR_BUSY;

  ret = uvc_query_stream_ctrl(strmh->devh, ctrl, 0, UVC_SET_CUR);
  if (ret != UVC_SUCCESS)
    return ret;

  strmh->cur_ctrl = *ctrl;
  return UVC_SUCCESS;
}

/** @internal
 * @brief Find the descriptor for a specific frame configuration
 * @param stream_if Stream interface
 * @param format_id Index of format class descriptor
 * @param frame_id Index of frame descriptor
 */
static uvc_frame_desc_t *_uvc_find_frame_desc_stream_if(uvc_streaming_interface_t *stream_if,
    uint16_t format_id, uint16_t frame_id) {

  uvc_format_desc_t *format = NULL;
  uvc_frame_desc_t *frame = NULL;

  DL_FOREACH(stream_if->format_descs, format) {
    if (format->bFormatIndex == format_id) {
      DL_FOREACH(format->frame_descs, frame) {
        if (frame->bFrameIndex == frame_id)
          return frame;
      }
    }
  }

  return NULL;
}

uvc_frame_desc_t *uvc_find_frame_desc_stream(uvc_stream_handle_t *strmh,
    uint16_t format_id, uint16_t frame_id) {
  return _uvc_find_frame_desc_stream_if(strmh->stream_if, format_id, frame_id);
}

/** @internal
 * @brief Find the descriptor for a specific frame configuration
 * @param devh UVC device
 * @param format_id Index of format class descriptor
 * @param frame_id Index of frame descriptor
 */
uvc_frame_desc_t *uvc_find_frame_desc(uvc_device_handle_t *devh,
    uint16_t format_id, uint16_t frame_id) {

  uvc_streaming_interface_t *stream_if;
  uvc_frame_desc_t *frame;

  DL_FOREACH(devh->info->stream_ifs, stream_if) {
    frame = _uvc_find_frame_desc_stream_if(stream_if, format_id, frame_id);
    if (frame)
      return frame;
  }

  return NULL;
}

/** Return all device formats available
 * @ingroup streaming
 *
 * @param[in] devh Device Handle
 * @param[out] formats returned
 */

#define SWAP_UINT32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))

uvc_error_t uvc_get_available_formats(
        uvc_device_handle_t *devh,
        int interface_idx,
        uvc_format_t **formats) {

  uvc_streaming_interface_t *stream_if = NULL;
  uvc_format_t *prev_format = NULL;
  uvc_format_desc_t *format;

  stream_if = _uvc_get_stream_if(devh, interface_idx);
  if (stream_if == NULL) {
    return UVC_ERROR_INVALID_PARAM;
  }

  DL_FOREACH(stream_if->format_descs, format) {
    uint32_t *interval_ptr;
    uvc_frame_desc_t *frame_desc;

    DL_FOREACH(format->frame_descs, frame_desc) {
      for (interval_ptr = frame_desc->intervals;
           *interval_ptr;
           ++interval_ptr) {
        uvc_format_t *cur_format = (uvc_format_t *)malloc(sizeof(uvc_format_t));
        cur_format->height = frame_desc->wHeight;
        cur_format->width = frame_desc->wWidth;
        cur_format->fourcc = SWAP_UINT32(*(const uint32_t *) format->guidFormat);


        cur_format->fps = 10000000 / *interval_ptr;
        cur_format->next = NULL;

        if (prev_format != NULL) {
            prev_format->next = cur_format;
        } else {
            *formats = cur_format;
        }

        prev_format = cur_format;
      }
    }
  }
  return UVC_SUCCESS;
}

uvc_error_t uvc_get_available_formats_all(
        uvc_device_handle_t *devh,
        uvc_format_t **formats) {

  uvc_streaming_interface_t *stream_if = NULL;
  uvc_format_t *prev_format = NULL;
  uvc_format_desc_t *format;

  DL_FOREACH(devh->info->stream_ifs, stream_if ) {
    DL_FOREACH(stream_if->format_descs, format) {
      uint32_t *interval_ptr;
      uvc_frame_desc_t *frame_desc;

      DL_FOREACH(format->frame_descs, frame_desc) {
        for (interval_ptr = frame_desc->intervals;
             *interval_ptr;
             ++interval_ptr) {
          uvc_format_t *cur_format = (uvc_format_t *)malloc(sizeof(uvc_format_t));
          cur_format->height = frame_desc->wHeight;
          cur_format->width = frame_desc->wWidth;
          cur_format->fourcc = SWAP_UINT32(*(const uint32_t *) format->guidFormat);


          cur_format->fps = 10000000 / *interval_ptr;
          cur_format->next = NULL;

          if (prev_format != NULL) {
            prev_format->next = cur_format;
          } else {
            *formats = cur_format;
          }

          prev_format = cur_format;
        }
      }
    }
  }
  return UVC_SUCCESS;
}

uvc_error_t uvc_get_available_streams(
        uvc_device_handle_t *devh, uvc_stream_info_t **streams) {
  uvc_streaming_interface_t *stream_if;
  uvc_stream_info_t *stream_first = NULL;
  uvc_stream_info_t *stream_prev = NULL;

  DL_FOREACH(devh->info->stream_ifs, stream_if) {
    uvc_stream_info_t *stream = (uvc_stream_info_t *)malloc(sizeof(uvc_stream_info_t));
    stream->interface_idx = stream_if->bInterfaceNumber;
    stream->next = NULL;
    if (stream_first == NULL) {
      stream_first = stream;
    }

    if (stream_prev != NULL) {
      stream_prev->next = stream;
    }

    stream_prev = stream;
  }

  *streams = stream_first;
  return UVC_SUCCESS;
}

uvc_error_t uvc_free_streams(uvc_stream_info_t *streams) {
  uvc_stream_info_t *cur_stream = streams;
  while(cur_stream != NULL) {
    uvc_stream_info_t *stream = cur_stream;
    cur_stream = cur_stream->next;
    free (stream);
  }

  return UVC_SUCCESS;

}

uvc_error_t uvc_free_formats(uvc_format_t *formats) {
    uvc_format_t *cur_format = formats;
    while(cur_format != NULL) {
        uvc_format_t *format = cur_format;
        cur_format = cur_format->next;
        free (format);
    }

    return UVC_SUCCESS;

}

/** Get a negotiated streaming control block for some common parameters.
 * @ingroup streaming
 *
 * @param[in] devh Device handle
 * @param[in,out] ctrl Control block
 * @param[in] format_class Type of streaming format
 * @param[in] width Desired frame width
 * @param[in] height Desired frame height
 * @param[in] fps Frame rate, frames per second
 */
uvc_error_t uvc_get_stream_ctrl_format_size(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uint32_t fourcc,
    int width, int height,
    int fps,
    int interface_idx
    ) {
  uvc_streaming_interface_t *stream_if;
  uvc_format_desc_t *format;

  stream_if = _uvc_get_stream_if(devh, interface_idx);
  if (stream_if == NULL) {
    return UVC_ERROR_INVALID_PARAM;
  }

  DL_FOREACH(stream_if->format_descs, format) {
    uvc_frame_desc_t *frame;

    if (SWAP_UINT32(fourcc) != *(const uint32_t *)format->guidFormat)
      continue;

    DL_FOREACH(format->frame_descs, frame) {
      if (frame->wWidth != width || frame->wHeight != height)
        continue;

      uint32_t *interval;

      if (frame->intervals) {
        for (interval = frame->intervals; *interval; ++interval) {
          // allow a fps rate of zero to mean "accept first rate available"
          if (10000000 / *interval == (unsigned int) fps || fps == 0) {

            /* get the max values -- we need the interface number to be able
               to do this */
            ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
            uvc_query_stream_ctrl( devh, ctrl, 1, UVC_GET_MAX);

            ctrl->bmHint = (1 << 0); /* don't negotiate interval */
            ctrl->bFormatIndex = format->bFormatIndex;
            ctrl->bFrameIndex = frame->bFrameIndex;
            ctrl->dwFrameInterval = *interval;

            goto found;
          }
        }
      } else {
        uint32_t interval_100ns = 10000000 / fps;
        uint32_t interval_offset = interval_100ns - frame->dwMinFrameInterval;

        if (interval_100ns >= frame->dwMinFrameInterval
            && interval_100ns <= frame->dwMaxFrameInterval
            && !(interval_offset
                 && (interval_offset % frame->dwFrameIntervalStep))) {

          /* get the max values -- we need the interface number to be able
             to do this */
          ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
          uvc_query_stream_ctrl( devh, ctrl, 1, UVC_GET_MAX);

          ctrl->bmHint = (1 << 0);
          ctrl->bFormatIndex = format->bFormatIndex;
          ctrl->bFrameIndex = frame->bFrameIndex;
          ctrl->dwFrameInterval = interval_100ns;

          goto found;
        }
      }
    }
  }

  return UVC_ERROR_INVALID_MODE;

found:
  return uvc_probe_stream_ctrl(devh, ctrl);
}

uvc_error_t uvc_get_stream_ctrl_format_size_all(
        uvc_device_handle_t *devh,
        uvc_stream_ctrl_t *ctrl,
        uint32_t fourcc,
        int width, int height,
        int fps
) {
  uvc_streaming_interface_t *stream_if;
  uvc_format_desc_t *format;

  DL_FOREACH(devh->info->stream_ifs, stream_if) {
    DL_FOREACH(stream_if->format_descs, format) {
      uvc_frame_desc_t *frame;

      if (SWAP_UINT32(fourcc) != *(const uint32_t *) format->guidFormat)
        continue;

      DL_FOREACH(format->frame_descs, frame) {
        if (frame->wWidth != width || frame->wHeight != height)
          continue;

        uint32_t *interval;

        if (frame->intervals) {
          for (interval = frame->intervals; *interval; ++interval) {
            // allow a fps rate of zero to mean "accept first rate available"
            if (10000000 / *interval == (unsigned int) fps || fps == 0) {

              /* get the max values -- we need the interface number to be able
                 to do this */
              ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
              uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);

              ctrl->bmHint = (1 << 0); /* don't negotiate interval */
              ctrl->bFormatIndex = format->bFormatIndex;
              ctrl->bFrameIndex = frame->bFrameIndex;
              ctrl->dwFrameInterval = *interval;

              goto found;
            }
          }
        } else {
          uint32_t interval_100ns = 10000000 / fps;
          uint32_t interval_offset = interval_100ns - frame->dwMinFrameInterval;

          if (interval_100ns >= frame->dwMinFrameInterval
              && interval_100ns <= frame->dwMaxFrameInterval
              && !(interval_offset
                   && (interval_offset % frame->dwFrameIntervalStep))) {

            /* get the max values -- we need the interface number to be able
               to do this */
            ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
            uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);

            ctrl->bmHint = (1 << 0);
            ctrl->bFormatIndex = format->bFormatIndex;
            ctrl->bFrameIndex = frame->bFrameIndex;
            ctrl->dwFrameInterval = interval_100ns;

            goto found;
          }
        }
      }
    }
  }

  return UVC_ERROR_INVALID_MODE;

  found:
  return uvc_probe_stream_ctrl(devh, ctrl);
}

/** @internal
 * Negotiate streaming parameters with the device
 *
 * @param[in] devh UVC device
 * @param[in,out] ctrl Control block
 */
uvc_error_t uvc_probe_stream_ctrl(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl) {

  uvc_claim_if(devh, ctrl->bInterfaceNumber);

  uvc_query_stream_ctrl(
      devh, ctrl, 1, UVC_SET_CUR
  );

  uvc_query_stream_ctrl(
      devh, ctrl, 1, UVC_GET_CUR
  );

  /** @todo make sure that worked */
  return UVC_SUCCESS;
}

/** @internal
 * @brief Swap the working buffer with the presented buffer and notify consumers
 */
void _uvc_swap_buffers(uvc_stream_handle_t *strmh) {
  uint8_t *tmp_buf;
  {
      /* swap the buffers */
      tmp_buf = strmh->holdbuf;
      strmh->hold_bytes = strmh->got_bytes;
      strmh->holdbuf = strmh->outbuf;
      strmh->outbuf = tmp_buf;
      strmh->hold_last_scr = strmh->last_scr;
      strmh->hold_pts = strmh->pts;
      strmh->hold_seq = strmh->seq;
  }
  strmh->cb_cond.notify_all();

  strmh->seq++;
  strmh->got_bytes = 0;
  strmh->last_scr = 0;
  strmh->pts = 0;
}

/** @internal
 * @brief Process a payload transfer
 *
 * Processes stream, places frames into buffer, signals listeners
 * (such as user callback thread and any polling thread) on new frame
 *
 * @param payload Contents of the payload transfer, either a packet (isochronous) or a full
 * transfer (bulk mode)
 * @param payload_len Length of the payload transfer
 */
void _uvc_process_payload(uvc_stream_handle_t *strmh, uint8_t *payload, size_t payload_len) {
  size_t header_len;
  uint8_t header_info;
  size_t data_len;
  struct libusb_iso_packet_descriptor *pkt;

  /* magic numbers for identifying header packets from some iSight cameras */
  static uint8_t isight_tag[] = {
    0x11, 0x22, 0x33, 0x44,
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xfa, 0xce
  };

  /* ignore empty payload transfers */
  if (payload_len == 0)
    return;

  /* Certain iSight cameras have strange behavior: They send header
   * information in a packet with no image data, and then the following
   * packets have only image data, with no more headers until the next frame.
   *
   * The iSight header: len(1), flags(1 or 2), 0x11223344(4),
   * 0xdeadbeefdeadface(8), ??(16)
   */

  if (strmh->devh->is_isight &&
      (payload_len < 14 || memcmp(isight_tag, payload + 2, sizeof(isight_tag))) &&
      (payload_len < 15 || memcmp(isight_tag, payload + 3, sizeof(isight_tag)))) {
    /* The payload transfer doesn't have any iSight magic, so it's all image data */
    header_len = 0;
    data_len = payload_len;
  } else {
    header_len = payload[0];

        if (header_len > payload_len) {
            UVC_DEBUG("bogus packet: actual_len=%zd, header_len=%zd\n", payload_len, header_len);
            return;
        }

        if (header_len <= strmh->metadata_size) {
            memcpy( strmh->metadata_buf, payload, header_len);
            strmh->metadata_bytes = header_len;
        }

    if (strmh->devh->is_isight)
      data_len = 0;
    else
      data_len = payload_len - header_len;
  }

  if (header_len < 2) {
    header_info = 0;
  } else {
    /** @todo we should be checking the end-of-header bit */
    size_t variable_offset = 2;

    header_info = payload[1];

    if (header_info & 0x40) {
      UVC_DEBUG("bad packet: error bit set");
      return;
    }

    if (strmh->fid != (header_info & 1) && strmh->got_bytes != 0) {
      /* The frame ID bit was flipped, but we have image data sitting
         around from prior transfers. This means the camera didn't send
         an EOF for the last transfer of the previous frame. */
      _uvc_swap_buffers(strmh);
    }

    strmh->fid = header_info & 1;

    if (header_info & (1 << 2)) {
      strmh->pts = DW_TO_INT(payload + variable_offset);
      variable_offset += 4;
    }

    if (header_info & (1 << 3)) {
      /** @todo read the SOF token counter */
      strmh->last_scr = DW_TO_INT(payload + variable_offset);
      variable_offset += 6;
    }
  }

  if (data_len > 0) {
    memcpy(strmh->outbuf + strmh->got_bytes, payload + header_len, data_len);
    strmh->got_bytes += data_len;

    if (header_info & (1 << 1)) {
      /* The EOF bit is set, so publish the complete frame */
      _uvc_swap_buffers(strmh);
    }
  }
}

/** @internal
 * @brief Stream transfer callback
 *
 * Processes stream, places frames into buffer, signals listeners
 * (such as user callback thread and any polling thread) on new frame
 *
 * @param transfer Active transfer
 */
void LIBUSB_CALL _uvc_stream_callback(struct libusb_transfer *transfer) {
  uvc_stream_handle_t *strmh = (uvc_stream_handle_t *)transfer->user_data;
  
    
  int resubmit = 1;
  std::unique_lock<std::mutex> lock(strmh->cb_mutex);
  switch (transfer->status) {
      case LIBUSB_TRANSFER_COMPLETED:
      {
          if (strmh && !strmh->running)
              break;
          if (transfer->num_iso_packets == 0) {
              /* This is a bulk mode transfer, so it just has one payload transfer */
              _uvc_process_payload(strmh, transfer->buffer, transfer->actual_length);
          }
          else
          {
              /* This is an isochronous mode transfer, so each packet has a payload transfer */
              int packet_id;
              
              for (packet_id = 0; packet_id < transfer->num_iso_packets; ++packet_id) {
                  uint8_t *pktbuf;
                  struct libusb_iso_packet_descriptor *pkt;
                  
                  pkt = transfer->iso_packet_desc + packet_id;
                  
                  if (pkt->status != 0) {

                      UVC_DEBUG("bad packet (isochronous transfer); status: %d", pkt->status);
                      continue;
                  }
                  
                  pktbuf = libusb_get_iso_packet_buffer_simple(transfer, packet_id);
                  
                  _uvc_process_payload(strmh, pktbuf, pkt->actual_length);
                  
              }
          }
          break;
      }
        
  case LIBUSB_TRANSFER_CANCELLED:
  case LIBUSB_TRANSFER_NO_DEVICE: {
    int i;
    UVC_DEBUG("not retrying transfer, status = %d", transfer->status);    
    {
        /* Mark transfer as deleted. */
        for (i = 0; i < LIBUVC_NUM_TRANSFER_BUFS; i++) {
            if (strmh->transfers[i] == transfer) {
                UVC_DEBUG("Freeing transfer %d (%p)", i, transfer);
                free(transfer->buffer);
                libusb_free_transfer(transfer);
                strmh->transfers[i] = NULL;
                strmh->transfer_cancel[i].notify_all();
                break;
            }
        }
        if (i == LIBUVC_NUM_TRANSFER_BUFS) {
            UVC_DEBUG("transfer %p not found; not freeing!", transfer);
        }
        resubmit = 0;
    }

    break;
  }
  case LIBUSB_TRANSFER_TIMED_OUT:
  case LIBUSB_TRANSFER_STALL:
  case LIBUSB_TRANSFER_OVERFLOW:
  case LIBUSB_TRANSFER_ERROR:
    UVC_DEBUG("retrying transfer, status = %d", transfer->status);
    break;
  }
  
    if ( resubmit )
    {
        if ( strmh->running )
        {
            libusb_submit_transfer(transfer);
        }
        else
        {
            int i;
            /* Mark transfer as deleted. */
            for(i=0; i < LIBUVC_NUM_TRANSFER_BUFS; i++) {
                if(strmh->transfers[i] == transfer) {
                    UVC_DEBUG("Freeing orphan transfer %d (%p)", i, transfer);
                    free(transfer->buffer);
                    libusb_free_transfer(transfer);
                    strmh->transfers[i] = NULL;
                    strmh->transfer_cancel[i].notify_all();

                }
            }
            if(i == LIBUVC_NUM_TRANSFER_BUFS ) {
                UVC_DEBUG("orphan transfer %p not found; not freeing!", transfer);
            }
        }
    }
}

/** Begin streaming video from the camera into the callback function.
 * @ingroup streaming
 *
 * @param devh UVC device
 * @param ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 * @param flags Stream setup flags, currently undefined. Set this to zero. The lower bit
 * is reserved for backward compatibility.
 */
uvc_error_t uvc_start_streaming(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uvc_frame_callback_t *cb,
    void *user_ptr,
    uint8_t flags
) {
  uvc_error_t ret;
  uvc_stream_handle_t *strmh;

  ret = uvc_stream_open_ctrl(devh, &strmh, ctrl);
  if (ret != UVC_SUCCESS)
    return ret;

  ret = uvc_stream_start(strmh, cb, user_ptr, flags);
  if (ret != UVC_SUCCESS) {
    uvc_stream_close(strmh);
    return ret;
  }

  return UVC_SUCCESS;
}

/** Begin streaming video from the camera into the callback function.
 * @ingroup streaming
 *
 * @deprecated The stream type (bulk vs. isochronous) will be determined by the
 * type of interface associated with the uvc_stream_ctrl_t parameter, regardless
 * of whether the caller requests isochronous streaming. Please switch to
 * uvc_start_streaming().
 *
 * @param devh UVC device
 * @param ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 */
uvc_error_t uvc_start_iso_streaming(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uvc_frame_callback_t *cb,
    void *user_ptr
) {
  return uvc_start_streaming(devh, ctrl, cb, user_ptr, 0);
}

static uvc_stream_handle_t *_uvc_get_stream_by_interface(uvc_device_handle_t *devh, int interface_idx) {
  uvc_stream_handle_t *strmh;

  DL_FOREACH(devh->streams, strmh) {
    if (strmh->stream_if->bInterfaceNumber == interface_idx)
      return strmh;
  }

  return NULL;
}

static uvc_streaming_interface_t *_uvc_get_stream_if(uvc_device_handle_t *devh, int interface_idx) {
  uvc_streaming_interface_t *stream_if;

  DL_FOREACH(devh->info->stream_ifs, stream_if) {
    if (stream_if->bInterfaceNumber == interface_idx)
      return stream_if;
  }

  return NULL;
}

/** Open a new video stream.
 * @ingroup streaming
 *
 * @param devh UVC device
 * @param ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 */
uvc_error_t uvc_stream_open_ctrl(uvc_device_handle_t *devh, uvc_stream_handle_t **strmhp, uvc_stream_ctrl_t *ctrl) {
  /* Chosen frame and format descriptors */
  uvc_stream_handle_t *strmh = NULL;
  uvc_streaming_interface_t *stream_if;
  uvc_error_t ret;

  UVC_ENTER();

  if (_uvc_get_stream_by_interface(devh, ctrl->bInterfaceNumber) != NULL) {
    ret = UVC_ERROR_BUSY; /* Stream is already opened */
    goto fail;
  }

  stream_if = _uvc_get_stream_if(devh, ctrl->bInterfaceNumber);
  if (!stream_if) {
    ret = UVC_ERROR_INVALID_PARAM;
    goto fail;
  }

  strmh = new uvc_stream_handle_t();

  if (!strmh) {
    ret = UVC_ERROR_NO_MEM;
    goto fail;
  }
  strmh->devh = devh;
  strmh->stream_if = stream_if;
  strmh->frame.library_owns_data = 1;

  ret = uvc_claim_if(strmh->devh, strmh->stream_if->bInterfaceNumber);
  if (ret != UVC_SUCCESS)
    goto fail;

  ret = uvc_stream_ctrl(strmh, ctrl);
  if (ret != UVC_SUCCESS)
    goto fail;

    // Set up the streaming status and data space
    strmh->running = 0;
    /** @todo take only what we need */
    strmh->outbuf = (uint8_t *)malloc( LIBUVC_XFER_BUF_SIZE );
    strmh->holdbuf = (uint8_t *)malloc( LIBUVC_XFER_BUF_SIZE );

    strmh->metadata_buf = (uint8_t *)malloc( 2048 );
    strmh->metadata_size = 2048;

  DL_APPEND(devh->streams, strmh);

  *strmhp = strmh;

  UVC_EXIT(0);
  return UVC_SUCCESS;

fail:
  if(strmh)
    delete strmh;
  UVC_EXIT(ret);
  return ret;
}

/** Begin streaming video from the stream into the callback function.
 * @ingroup streaming
 *
 * @param strmh UVC stream
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 * @param flags Stream setup flags, currently undefined. Set this to zero. The lower bit
 * is reserved for backward compatibility.
 */
uvc_error_t uvc_stream_start(
    uvc_stream_handle_t *strmh,
    uvc_frame_callback_t *cb,
    void *user_ptr,
    uint8_t flags
) {
  /* USB interface we'll be using */
  const struct libusb_interface *interface;
  int interface_id;
  char isochronous;
  uvc_frame_desc_t *frame_desc;
  uvc_format_desc_t *format_desc;
  uvc_stream_ctrl_t *ctrl;
  int ret;
  /* Total amount of data per transfer */
  size_t total_transfer_size;
  struct libusb_transfer *transfer;
  int transfer_id;

  ctrl = &strmh->cur_ctrl;

  UVC_ENTER();

  if (strmh->running) {
    UVC_EXIT(UVC_ERROR_BUSY);
    return UVC_ERROR_BUSY;
  }

  strmh->running = 1;
  strmh->seq = 1;
  strmh->fid = 0;
  strmh->pts = 0;
  strmh->last_scr = 0;

  frame_desc = uvc_find_frame_desc_stream(strmh, ctrl->bFormatIndex, ctrl->bFrameIndex);
  if (!frame_desc) {
    ret = UVC_ERROR_INVALID_PARAM;
    goto fail;
  }
  format_desc = frame_desc->parent;

  strmh->frame_format = uvc_frame_format_for_guid(format_desc->guidFormat);

  // Get the interface that provides the chosen format and frame configuration
  interface_id = strmh->stream_if->bInterfaceNumber;
  interface = &strmh->devh->info->config->interface[interface_id];

  /* A VS interface uses isochronous transfers iff it has multiple altsettings.
   * (UVC 1.5: 2.4.3. VideoStreaming Interface) */
  isochronous = interface->num_altsetting > 1;

  for (transfer_id = 0; transfer_id < LIBUVC_NUM_TRANSFER_BUFS;
      ++transfer_id) {
    transfer = libusb_alloc_transfer(0);
    strmh->transfers[transfer_id] = transfer;
    strmh->transfer_bufs[transfer_id] = (uint8_t *)malloc (
        strmh->cur_ctrl.dwMaxPayloadTransferSize );
    libusb_fill_bulk_transfer ( transfer, strmh->devh->usb_devh,
        format_desc->parent->bEndpointAddress,
        strmh->transfer_bufs[transfer_id],
        strmh->cur_ctrl.dwMaxPayloadTransferSize, _uvc_stream_callback,
        ( void* ) strmh, 5000 );
  }

  strmh->user_cb = cb;
  strmh->user_ptr = user_ptr;

  /* If the user wants it, set up a thread that calls the user's function
   * with the contents of each frame.
   */
  if (cb) {
      strmh->cb_thread = std::thread([&]() {_uvc_user_caller((void*)strmh); });
  }

  for (transfer_id = 0; transfer_id < LIBUVC_NUM_TRANSFER_BUFS;
      transfer_id++) {
    ret = libusb_submit_transfer(strmh->transfers[transfer_id]);
    if (ret != UVC_SUCCESS) {
      UVC_DEBUG("libusb_submit_transfer failed");
      break;
    }
  }

  UVC_EXIT(ret);
  return (uvc_error_t)ret;
fail:
  strmh->running = 0;
  UVC_EXIT(ret);
  return (uvc_error_t)ret;
}

/** Begin streaming video from the stream into the callback function.
 * @ingroup streaming
 *
 * @deprecated The stream type (bulk vs. isochronous) will be determined by the
 * type of interface associated with the uvc_stream_ctrl_t parameter, regardless
 * of whether the caller requests isochronous streaming. Please switch to
 * uvc_stream_start().
 *
 * @param strmh UVC stream
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 */
uvc_error_t uvc_stream_start_iso(
    uvc_stream_handle_t *strmh,
    uvc_frame_callback_t *cb,
    void *user_ptr
) {
  return uvc_stream_start(strmh, cb, user_ptr, 0);
}

/** @internal
 * @brief User callback runner thread
 * @note There should be at most one of these per currently streaming device
 * @param arg Device handle
 */
void *_uvc_user_caller(void *arg) {
  uvc_stream_handle_t *strmh = (uvc_stream_handle_t *) arg;

  uint32_t last_seq = 0;

  do {

      {
          std::unique_lock<std::mutex> lock(strmh->cb_mutex);

          while (strmh->running && last_seq == strmh->hold_seq) {
              strmh->cb_cond.wait(lock);
          }

          if (!strmh->running) {
              break;
          }

          last_seq = strmh->hold_seq;
          _uvc_populate_frame(strmh);
      }

    strmh->user_cb(&strmh->frame, strmh->user_ptr);
  } while(1);

  return NULL; // return value ignored
}

/** @internal
 * @brief Populate the fields of a frame to be handed to user code
 * must be called with stream cb lock held!
 */
void _uvc_populate_frame(uvc_stream_handle_t *strmh) {
  size_t alloc_size = strmh->cur_ctrl.dwMaxVideoFrameSize;
  uvc_frame_t *frame = &strmh->frame;
  uvc_frame_desc_t *frame_desc;

  /** @todo this stuff that hits the main config cache should really happen
   * in start() so that only one thread hits these data. all of this stuff
   * is going to be reopen_on_change anyway
   */

  frame_desc = uvc_find_frame_desc(strmh->devh, strmh->cur_ctrl.bFormatIndex,
				   strmh->cur_ctrl.bFrameIndex);

  frame->frame_format = strmh->frame_format;

  frame->width = frame_desc->wWidth;
  frame->height = frame_desc->wHeight;

  switch (frame->frame_format) {
  case UVC_FRAME_FORMAT_YUYV:
    frame->step = frame->width * 2;
    break;
  case UVC_FRAME_FORMAT_MJPEG:
    frame->step = 0;
    break;
  default:
    frame->step = 0;
    break;
  }

  frame->sequence = strmh->hold_seq;
  /** @todo set the frame time */
  // frame->capture_time

  /* copy the image data from the hold buffer to the frame (unnecessary extra buf?) */
  if (frame->data_bytes < strmh->hold_bytes) {
    frame->data = realloc(frame->data, strmh->hold_bytes);
  }
  frame->data_bytes = strmh->hold_bytes;
  memcpy(frame->data, strmh->holdbuf, frame->data_bytes);

    /* copy the header data from the buffer to the frame */

    if (frame->metadata_bytes < strmh->metadata_bytes) {
        frame->metadata = (uint8_t *)realloc(frame->metadata, strmh->metadata_bytes);
    }

    frame->metadata_bytes = strmh->metadata_bytes;
    memcpy(frame->metadata, strmh->metadata_buf, frame->metadata_bytes);
}

/** Poll for a frame
 * @ingroup streaming
 *
 * @param devh UVC device
 * @param[out] frame Location to store pointer to captured frame (NULL on error)
 * @param timeout_us >0: Wait at most N microseconds; 0: Wait indefinitely; -1: return immediately
 */
uvc_error_t uvc_stream_get_frame(uvc_stream_handle_t *strmh,
			  uvc_frame_t **frame,
			  int32_t timeout_us) {
  time_t add_secs;
  time_t add_nsecs;
  struct timespec ts;
  struct timeval tv;

  if (!strmh->running)
    return UVC_ERROR_INVALID_PARAM;

  if (strmh->user_cb)
    return UVC_ERROR_CALLBACK_EXISTS;

  {
      std::unique_lock<std::mutex> lock(strmh->cb_mutex);

      if (strmh->last_polled_seq < strmh->hold_seq) {
          _uvc_populate_frame(strmh);
          *frame = &strmh->frame;
          strmh->last_polled_seq = strmh->hold_seq;
      }
      else if (timeout_us != -1) {
          if (timeout_us == 0) {
              strmh->cb_cond.wait(lock);
          }
          else {
              add_secs = timeout_us / 1000000;
              add_nsecs = (timeout_us % 1000000) * 1000;
              ts.tv_sec = 0;
              ts.tv_nsec = 0;

#if _POSIX_TIMERS > 0
              clock_gettime(CLOCK_REALTIME, &ts);
#else
              gettimeofday(&tv, NULL);
              ts.tv_sec = tv.tv_sec;
              ts.tv_nsec = tv.tv_usec * 1000;
#endif

              ts.tv_sec += add_secs;
              ts.tv_nsec += add_nsecs;

              /* pthread_cond_timedwait FAILS with EINVAL if ts.tv_nsec > 1000000000 (1 billion)
               * Since we are just adding values to the timespec, we have to increment the seconds if nanoseconds is greater than 1 billion,
               * and then re-adjust the nanoseconds in the correct range.
               * */
              ts.tv_sec += ts.tv_nsec / 1000000000;
              ts.tv_nsec = ts.tv_nsec % 1000000000;

              auto d = std::chrono::seconds{ ts.tv_sec }
              + std::chrono::nanoseconds{ ts.tv_nsec };

              std::chrono::time_point<std::chrono::system_clock> tp{ std::chrono::duration_cast<std::chrono::system_clock::duration>(d) };
              std::cv_status err{ std::cv_status::no_timeout };
              try {
                  err = strmh->cb_cond.wait_until(lock, tp);
              }
              catch (...)
              {
                  *frame = NULL;
                  return UVC_ERROR_OTHER;
              }

              //TODO: How should we handle EINVAL?
              switch (err) {
                  case std::cv_status::timeout:
                      *frame = NULL;
                      return UVC_ERROR_TIMEOUT;
              }
          }

          if (strmh->last_polled_seq < strmh->hold_seq) {
              _uvc_populate_frame(strmh);
              *frame = &strmh->frame;
              strmh->last_polled_seq = strmh->hold_seq;
          }
          else {
              *frame = NULL;
          }
      }
      else {
          *frame = NULL;
      }

  }

  return UVC_SUCCESS;
}

/** @brief Stop streaming video
 * @ingroup streaming
 *
 * Closes all streams, ends threads and cancels pollers
 *
 * @param devh UVC device
 */
void uvc_stop_streaming(uvc_device_handle_t *devh) {
  uvc_stream_handle_t *strmh, *strmh_tmp;

  DL_FOREACH_SAFE(devh->streams, strmh, strmh_tmp) {
    uvc_stream_close(strmh);
  }
}

/** @brief Stop stream.
 * @ingroup streaming
 *
 * Stops stream, ends threads and cancels pollers
 *
 * @param devh UVC device
 */
uvc_error_t uvc_stream_stop(uvc_stream_handle_t *strmh) {
  int i;

  if (!strmh->running)
    return UVC_ERROR_INVALID_PARAM;
    
  {
    std::unique_lock<std::mutex> lock(strmh->cb_mutex);
    strmh->running = 0;
    for (i = 0; i < LIBUVC_NUM_TRANSFER_BUFS; i++)
    {
        if (strmh->transfers[i] != NULL)
        {
            int res = libusb_cancel_transfer(strmh->transfers[i]);
            if (res < 0)
            {
                free(strmh->transfers[i]->buffer);
                libusb_free_transfer(strmh->transfers[i]);
                strmh->transfers[i] = NULL;
            }
        }
    }
      
    for (i = 0; i < LIBUVC_NUM_TRANSFER_BUFS; i++)
    {
        if (strmh->transfers[i] != NULL)
            strmh->transfer_cancel[i].wait(lock);
    }
  
    
   // Kick the user thread awake
   strmh->cb_cond.notify_all();
  }
  /** @todo stop the actual stream, camera side? */

  if (strmh->user_cb) {
    /* wait for the thread to stop (triggered by
     * LIBUSB_TRANSFER_CANCELLED transfer) */
      strmh->cb_thread.join();
    }
  
  auto res = libusb_clear_halt(strmh->devh->usb_devh,strmh->stream_if->bEndpointAddress);

  return UVC_SUCCESS;
}

/** @brief Close stream.
 * @ingroup streaming
 *
 * Closes stream, frees handle and all streaming resources.
 *
 * @param strmh UVC stream handle
 */
void uvc_stream_close(uvc_stream_handle_t *strmh) {
  if (strmh->running)
    uvc_stream_stop(strmh);

  uvc_release_if(strmh->devh, strmh->stream_if->bInterfaceNumber);

  if (strmh->frame.data)
    free(strmh->frame.data);

    if (strmh->frame.metadata)
        free(strmh->frame.metadata);

    free(strmh->outbuf);
    free(strmh->holdbuf);
    free(strmh->metadata_buf);

  DL_DELETE(strmh->devh->streams, strmh);
  delete strmh;
}
