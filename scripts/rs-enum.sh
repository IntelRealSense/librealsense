#!/bin/bash

while [[ $# -gt 0 ]]; do
  case $1 in
    -i|--info)
      info=1
      shift
    ;;
    *)
      info=0
      shift
    ;;
    esac
done

declare -A camera_idx=( [a]=0 [b]=1 [c]=2 [d]=3 )


declare -A d4xx_vc=([1]=1 [2]=3 [4]=5)
declare -A d4xx_vc_named=([depth]=1 [rgb]=3 ["motion detection"]=5 [imu]=7)
declare -A camera_names=( [depth]=depth [rgb]=rgb ["motion detection"]=ir [imu]=imu )

dot=$(media-ctl --print-dot)
printf "Bus\tCamera\tSensor\tNode Type\tVideo Node\tRS Link\n"
# for all muxes
for camera in a b c d; do
  for sens in "${!d4xx_vc_named[@]}"; do
    d4xx_sens=$(echo "${dot}" | grep "D4XX $sens $camera" | awk '{print $1}')
    [[ -z $d4xx_sens ]] && continue; # echo "SENS $sens NOT FOUND" && continue
    d4xx_sens_mux=$(echo "${dot}" | grep $d4xx_sens:port0 | awk '{print $3}' | awk -F':' '{print $1}')
    csi2=$(echo "${dot}" | grep $d4xx_sens_mux:port0 | awk '{print $3}' | awk -F':' '{print $1}')
    be_soc=$(echo "${dot}" | grep $csi2:port1 | grep -v dashed | awk '{print $3}' | awk -F':' '{print $1}')
    vid_nd=$(echo "${dot}" | grep "$be_soc:port${d4xx_vc_named[${sens}]}" | grep -v dashed | awk '{print $3}' | awk -F':' '{print $1}')
    [[ -z $vid_nd ]] && continue; # echo "SENS $sens NOT FOUND" && continue
    vid=$(echo "${dot}" | grep "${vid_nd}" | grep video | tr '\\n' '\n' | grep video | awk -F'"' '{print $1}')
    dev_ln="/dev/video-rs-${camera_names["${sens}"]}-${camera_idx[${camera}]}"
    dev_name=$(v4l2-ctl -d $vid -D | grep Model | awk -F':' '{print $2}')
    # echo Sensor: $sens,\t Device: $vid,\t Link $dev_ln
    printf '%s\t%d\t%s\tStreaming\t%s\t%s\n' "$dev_name" ${camera_idx[${camera}]} ${camera_names["${sens}"]} $vid $dev_ln
    if [[ $info -eq 0 ]]; then
      [[ -e $dev_ln ]] && sudo unlink $dev_ln
      sudo ln -s $vid $dev_ln
      v4l2-ctl -d $dev_ln -c enumerate_graph_link=1
    fi
    # metadata link
    [[ ${camera_names["${sens}"]} == 'ir' ]] && continue
    vid_num=$(echo $vid | grep -o '[0-9]\+')
    dev_md_ln="/dev/video-rs-${camera_names["${sens}"]}-md-${camera_idx[${camera}]}"
    dev_name=$(v4l2-ctl -d "/dev/video$(($vid_num+1))" -D | grep Model | awk -F':' '{print $2}')
    # echo Metadata: $sens,\t Device: "/dev/video$(($vid_num+1))",\t Link: $dev_md_ln
    printf '%s\t%d\t%s\tMetadata\t/dev/video%s\t%s\n' "$dev_name" ${camera_idx[${camera}]} ${camera_names["${sens}"]} $(($vid_num+1)) $dev_md_ln
    if [[ $info -eq 0 ]]; then
      [[ -e $dev_md_ln ]] && sudo unlink $dev_md_ln
      sudo ln -s "/dev/video$(($vid_num+1))" $dev_md_ln
    fi
  done
done

