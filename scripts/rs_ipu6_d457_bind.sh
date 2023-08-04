#!/bin/bash
# Dependency: v4l-utils
v4l2_util=$(which v4l2-ctl)
media_util=$(which media-ctl)
if [ -z ${v4l2_util} ]; then
  echo "v4l2-ctl not found, install with: sudo apt install v4l-utils"
  exit 1
fi

# d457_bind.sh
metadata_enabled=1
while [[ $# -gt 0 ]]; do
  case $1 in
    -q|--quiet)
      quiet=1
      shift
    ;;
    -m|--mux)
      shift
      mux_param=$1
      shift
    ;;
    -n|--no-metadata)
      metadata_enabled=0
      shift
    ;;
    *)
      quiet=0
      shift
    ;;
    esac
done

declare -A media_mux_capture_link=( [a]='' [b]='1 ' [c]='2 ' [d]='3 ' [e]='' [f]='1 ' [g]='2 ' [h]='3 ')
declare -A media_mux_csi2_link=( [a]=0 [b]=1 [c]=2 [d]=3 [e]=0 [f]=1 [g]=2 [h]=3)

declare -A media_mux_capture_pad=( [a]=0 [b]=1 [c]=2 [d]=3 [e]=0 [f]=1 [g]=2 [h]=3)

mux_list=${mux_param:-'a b c d e f g h'}

mdev=$(${v4l2_util} --list-devices | grep -A1 ipu6 | grep media)
[[ -z "${mdev}" ]] && exit 0

media_ctl_cmd="${media_util} -d ${mdev}"
#media-ctl -r
# cache media-ctl output
dot=$($media_ctl_cmd --print-dot)

# DS5 MUX. Can be {a, b, c, d}.
for mux in $mux_list; do
is_mux=$(echo "${dot}" | grep "DS5 mux ${mux}")
[[ -z $is_mux ]] && continue;

[[ $quiet -eq 0 ]] && echo -n "Bind DS5 mux ${mux} .. "
csi2_be_soc="CSI2 BE SOC ${media_mux_csi2_link[${mux}]}"

csi2="CSI-2 ${media_mux_csi2_link[${mux}]}"

be_soc_cap="BE SOC ${media_mux_capture_link[${mux}]}capture"

cap_pad=0
$media_ctl_cmd -v -l "\"Intel IPU6 ${csi2}\":1 -> \"Intel IPU6 ${csi2_be_soc}\":0[1]" 1>/dev/null
$media_ctl_cmd -v -l "\"DS5 mux ${mux}\":0 -> \"Intel IPU6 ${csi2}\":0[1]" 1>/dev/null


cap_pad=0
if [ "${mux}" \> "d" ]; then
	cap_pad=6
fi

# DEPTH video streaming node
$media_ctl_cmd -v -l "\"Intel IPU6 ${csi2_be_soc}\":$((${cap_pad}+1)) -> \"Intel IPU6 ${be_soc_cap} $((${cap_pad}+0))\":0[5]" 1>/dev/null
# DEPTH metadata node
if [[ $metadata_enabled -eq 1 ]]; then
  $media_ctl_cmd -v -l "\"Intel IPU6 ${csi2_be_soc}\":$((${cap_pad}+2)) -> \"Intel IPU6 ${be_soc_cap} $((${cap_pad}+1))\":0[5]" 1>/dev/null
fi

# RGB link
$media_ctl_cmd -v -l "\"Intel IPU6 ${csi2_be_soc}\":$((${cap_pad}+3)) -> \"Intel IPU6 ${be_soc_cap} $((${cap_pad}+2))\":0[5]" 1>/dev/null
# RGB metadata node
if [[ $metadata_enabled -eq 1 ]]; then
  $media_ctl_cmd -v -l "\"Intel IPU6 ${csi2_be_soc}\":$((${cap_pad}+4)) -> \"Intel IPU6 ${be_soc_cap} $((${cap_pad}+3))\":0[5]" 1>/dev/null
fi

# IR link
$media_ctl_cmd -v -l "\"Intel IPU6 ${csi2_be_soc}\":$((${cap_pad}+5)) -> \"Intel IPU6 ${be_soc_cap} $((${cap_pad}+4))\":0[5]" 1>/dev/null

# IMU link
$media_ctl_cmd -v -l "\"Intel IPU6 ${csi2_be_soc}\":$((${cap_pad}+6)) -> \"Intel IPU6 ${be_soc_cap} $((${cap_pad}+5))\":0[5]" 1>/dev/null

[[ $quiet -eq 0 ]] && echo done
done
