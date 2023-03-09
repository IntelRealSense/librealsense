#!/bin/bash
# d457_bind.sh
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
    *)
      quiet=0
      shift
    ;;
    esac
done

declare -A media_mux_capture_link=( [a]='' [b]='1 ' [c]='2 ' [d]='3 ' )
declare -A media_mux_csi2_link=( [a]=0 [b]=1 [c]=2 [d]=3 )

declare -A serdes_mux_i2c_bus=( [a]=2 [b]=2 [c]=4 [d]=4 )
mux_list=${mux_param:-'a b c d'}

#media-ctl -r
# cache media-ctl output
dot=$(media-ctl --print-dot)

# DS5 MUX. Can be {a, b, c, d}.
for mux in $mux_list; do
is_mux=$(echo "${dot}" | grep "DS5 mux ${mux}")
[[ -z $is_mux ]] && continue;

[[ $quiet -eq 0 ]] && echo -n "Bind DS5 mux ${mux} .. "
csi2_be_soc="CSI2 BE SOC ${media_mux_csi2_link[${mux}]}"

csi2="CSI-2 ${media_mux_csi2_link[${mux}]}"

be_soc_cap="BE SOC ${media_mux_capture_link[${mux}]}capture"

media-ctl -v -l "\"Intel IPU6 ${csi2}\":1 -> \"Intel IPU6 ${csi2_be_soc}\":0[1]" 1>/dev/null
media-ctl -v -l "\"DS5 mux ${mux}\":0 -> \"Intel IPU6 ${csi2}\":0[1]" 1>/dev/null

media-ctl -v -l "\"D4XX depth ${mux}\":0 -> \"DS5 mux ${mux}\":1[1]" 1>/dev/null
# video streaming node
media-ctl -v -l "\"Intel IPU6 ${csi2_be_soc}\":1 -> \"Intel IPU6 ${be_soc_cap} 0\":0[5]" 1>/dev/null
# metadata node
media-ctl -v -l "\"Intel IPU6 ${csi2_be_soc}\":2 -> \"Intel IPU6 ${be_soc_cap} 1\":0[5]" 1>/dev/null

media-ctl -v -l "\"D4XX rgb ${mux}\":0 -> \"DS5 mux ${mux}\":2[1]" 1>/dev/null
# RGB link
media-ctl -v -l "\"Intel IPU6 ${csi2_be_soc}\":3 -> \"Intel IPU6 ${be_soc_cap} 2\":0[5]" 1>/dev/null
# RGB metadata node
media-ctl -v -l "\"Intel IPU6 ${csi2_be_soc}\":4 -> \"Intel IPU6 ${be_soc_cap} 3\":0[5]" 1>/dev/null

# IR link
media-ctl -v -l "\"D4XX motion detection ${mux}\":0 -> \"DS5 mux ${mux}\":3[1]" 1>/dev/null
media-ctl -v -l "\"Intel IPU6 ${csi2_be_soc}\":5 -> \"Intel IPU6 ${be_soc_cap} 4\":0[5]" 1>/dev/null

# IMU link
media-ctl -v -l "\"D4XX imu ${mux}\":0 -> \"DS5 mux ${mux}\":4[1]" 1>/dev/null
media-ctl -v -l "\"Intel IPU6 ${csi2_be_soc}\":6 -> \"Intel IPU6 ${be_soc_cap} 5\":0[5]" 1>/dev/null
[[ $quiet -eq 0 ]] && echo done
done
