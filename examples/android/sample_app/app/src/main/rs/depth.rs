#pragma version(1)
#pragma rs java_package_name(com.intel.realsense.libusbhost)
#pragma rs_fp_relaxed

uchar4 RS_KERNEL depth2rgba(ushort in, uint32_t x, uint32_t y) {
  uchar4 out;
  uchar d=in/256.0f;
  out.r = d;
  out.g = d;
  out.b = d;
  out.a = 255;
  return out;
}