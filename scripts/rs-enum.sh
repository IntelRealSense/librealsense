#!/bin/bash
# Create symbolic links for video nodes and for metadata nodes - /dev/video-rs-[<sensor>|<sensor>-md]-[camera-index]
# This script intended for mipi devices on Jetson and IPU6.
# After running this script in enumeration mode, it will create links as follow for example:
# Example of the output:
#
# Jetson:
# $ ./rs-enum.sh 
# Bus	  Camera	Sensor	Node Type	Video Node	RS Link
# mipi	0	      depth	  Streaming	/dev/video0	/dev/video-rs-depth-0
# mipi	0	      depth	  Metadata	/dev/video1	/dev/video-rs-depth-md-0
# mipi	0	      color	  Streaming	/dev/video2	/dev/video-rs-color-0
# mipi	0	      color	  Metadata	/dev/video3	/dev/video-rs-color-md-0
# mipi	0	      ir	    Streaming	/dev/video4	/dev/video-rs-ir-0
# mipi	0	      imu	    Streaming	/dev/video5	/dev/video-rs-imu-0
#
# Alderlake:
#$ ./rs-enum.sh 
#Bus	Camera	Sensor	Node Type	Video Node	RS Link
# ipu6	0	depth	  Streaming	/dev/video4 	  /dev/video-rs-depth-0
# ipu6	0	depth	  Metadata	/dev/video5	    /dev/video-rs-depth-md-0
# ipu6	0	ir	    Streaming	/dev/video8	    /dev/video-rs-ir-0
# ipu6	0	imu	    Streaming	/dev/video9	    /dev/video-rs-imu-0
# ipu6	0	color	  Streaming	/dev/video6	    /dev/video-rs-color-0
# ipu6	0	color	  Metadata	/dev/video7	    /dev/video-rs-color-md-0
# i2c 	0	d4xx   	Firmware 	/dev/d4xx-dfu-a	/dev/d4xx-dfu-0
# ipu6	2	depth	  Streaming	/dev/video36	  /dev/video-rs-depth-2
# ipu6	2	depth	  Metadata	/dev/video37  	/dev/video-rs-depth-md-2
# ipu6	2	ir	    Streaming	/dev/video40	  /dev/video-rs-ir-2
# ipu6	2	imu	    Streaming	/dev/video41	  /dev/video-rs-imu-2
# ipu6	2	color 	Streaming	/dev/video38	  /dev/video-rs-color-2
# ipu6	2	color 	Metadata	/dev/video39	  /dev/video-rs-color-md-2
# i2c 	2	d4xx   	Firmware 	/dev/d4xx-dfu-c	/dev/d4xx-dfu-2

# Dependency: v4l-utils
#
# parse command line parameters
# for '-i' parameter, print links only
while [[ $# -gt 0 ]]; do
  case $1 in
    -i|--info)
      info=1
      shift
    ;;
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
      info=0
      quiet=0
      shift
    ;;
    esac
done
#set -x
mux_list=${mux_param:-'a b c d'}

declare -A camera_idx=( [a]=0 [b]=1 [c]=2 [d]=3 )
declare -A d4xx_vc=([1]=1 [2]=3 [4]=5)
declare -A d4xx_vc_named=([depth]=1 [rgb]=3 ["motion detection"]=5 [imu]=6)
declare -A camera_names=( [depth]=depth [rgb]=color ["motion detection"]=ir [imu]=imu )

camera_vid=("depth" "depth-md" "color" "color-md" "ir" "imu")

[[ $quiet -eq 0 ]] && printf "Bus\tCamera\tSensor\tNode Type\tVideo Node\tRS Link\n"

# For Jetson we have simple method
if [ -n "$(v4l2-ctl --list-devices | grep tegra)" ]; then
  for ((i = 0; i < 127; i++)); do
    if [ ! -c /dev/video${i} ]; then
      break;
    fi
    cam_id=$((i/6))
    sens_id=$((i%6))
    vid="/dev/video${i}"
    dev_name=$(v4l2-ctl -d ${vid} -D | grep 'Driver name' | head -n1 | awk -F' : ' '{print $2}')
    dev_ln="/dev/video-rs-${camera_vid[${sens_id}]}-${cam_id}"
    bus="mipi"
    if [ "$dev_name" = "uvcvideo" ]; then
      dev_ln=$vid
      bus="usb"
    fi
    if [ -n "$(echo ${camera_vid[${sens_id}]} | awk /md/)" ]; then
	    type="Metadata"
    else
	    type="Streaming"
    fi
    sensor_name=$(echo "${camera_vid[${sens_id}]}" | awk -F'-' '{print $1}')
    [[ $quiet -eq 0 ]] && printf '%s\t%d\t%s\t%s\t%s\t%s\n' ${bus} ${cam_id} ${sensor_name} ${type} ${vid} ${dev_ln}
    # create link only in case we choose not only to show it
    if [[ $info -eq 0 ]] && [[ $bus != "usb" ]]; then
      [[ -e $dev_ln ]] && sudo unlink $dev_ln
      sudo ln -s $vid $dev_ln
      # Create DFU device link for camera on jetson
      subdev=$(media-ctl --print-dot | grep "D4XX depth ${camera} | awk -F'|' '{print $2}' | awk -F'/' '{print $3}' | tr -d ' '")
      i2cdev=$(ls -l /sys/class/video4linux/${subdev}/device | awk -F'/' '{print $9}')
      dev_dfu_name="/dev/d4xx-dfu-${i2cdev}"
      dev_dfu_ln="/dev/d4xx-dfu-${cam_id}"
      [[ -e $dev_dfu_ln ]] && sudo unlink $dev_dfu_ln
      sudo ln -s $dev_dfu_name $dev_dfu_ln
    fi
  done
exit 0
fi

#ADL-P IPU6
# cache media-ctl output
dot=$(media-ctl --print-dot)
# for all d457 muxes a, b, c and d
for camera in $mux_list; do
  create_dfu_dev=0
  for sens in "${!d4xx_vc_named[@]}"; do
    # get sensor binding from media controller
    d4xx_sens=$(echo "${dot}" | grep "D4XX $sens $camera" | awk '{print $1}')

    [[ -z $d4xx_sens ]] && continue; # echo "SENS $sens NOT FOUND" && continue

    d4xx_sens_mux=$(echo "${dot}" | grep $d4xx_sens:port0 | awk '{print $3}' | awk -F':' '{print $1}')
    csi2=$(echo "${dot}" | grep $d4xx_sens_mux:port0 | awk '{print $3}' | awk -F':' '{print $1}')
    be_soc=$(echo "${dot}" | grep $csi2:port1 | grep -v dashed | awk '{print $3}' | awk -F':' '{print $1}')
    vid_nd=$(echo "${dot}" | grep "$be_soc:port${d4xx_vc_named[${sens}]}" | grep -v dashed | awk '{print $3}' | awk -F':' '{print $1}')

    [[ -z $vid_nd ]] && continue; # echo "SENS $sens NOT FOUND" && continue

    vid=$(echo "${dot}" | grep "${vid_nd}" | grep video | tr '\\n' '\n' | grep video | awk -F'"' '{print $1}')
    [[ -z $vid ]] && continue;
    dev_ln="/dev/video-rs-${camera_names["${sens}"]}-${camera_idx[${camera}]}"
    dev_name=$(v4l2-ctl -d $vid -D | grep Model | awk -F':' '{print $2}')

    [[ $quiet -eq 0 ]] && printf '%s\t%d\t%s\tStreaming\t%s\t%s\n' "$dev_name" ${camera_idx[${camera}]} ${camera_names["${sens}"]} $vid $dev_ln

    # create link only in case we choose not only to show it
    if [[ $info -eq 0 ]]; then
      [[ -e $dev_ln ]] && sudo unlink $dev_ln
      sudo ln -s $vid $dev_ln
      # activate ipu6 link enumeration feature
      v4l2-ctl -d $dev_ln -c enumerate_graph_link=1
    fi
    create_dfu_dev=1 # will create DFU device link for camera
    # metadata link
    # skip IR metadata node for now.
    [[ ${camera_names["${sens}"]} == 'ir' ]] && continue
    # skip IMU metadata node.
    [[ ${camera_names["${sens}"]} == 'imu' ]] && continue

    vid_num=$(echo $vid | grep -o '[0-9]\+')
    dev_md_ln="/dev/video-rs-${camera_names["${sens}"]}-md-${camera_idx[${camera}]}"
    dev_name=$(v4l2-ctl -d "/dev/video$(($vid_num+1))" -D | grep Model | awk -F':' '{print $2}')

    [[ $quiet -eq 0 ]] && printf '%s\t%d\t%s\tMetadata\t/dev/video%s\t%s\n' "$dev_name" ${camera_idx[${camera}]} ${camera_names["${sens}"]} $(($vid_num+1)) $dev_md_ln
    # create link only in case we choose not only to show it
    if [[ $info -eq 0 ]]; then
      [[ -e $dev_md_ln ]] && sudo unlink $dev_md_ln
      sudo ln -s "/dev/video$(($vid_num+1))" $dev_md_ln
    fi
  done
  # create DFU device link for camera
  if [[ ${create_dfu_dev} -eq 1 ]]; then
    dev_dfu_name="/dev/d4xx-dfu-${camera}"
    dev_dfu_ln="/dev/d4xx-dfu-${camera_idx[${camera}]}"
    if [[ $info -eq 0 ]]; then
      [[ -e $dev_dfu_ln ]] && sudo unlink $dev_dfu_ln
      sudo ln -s $dev_dfu_name $dev_dfu_ln
    else
      [[ $quiet -eq 0 ]] && printf '%s\t%d\t%s\tFirmware \t%s\t%s\n' " i2c " ${camera_idx[${camera}]} "d4xx   " $dev_dfu_name $dev_dfu_ln
    fi
  fi
done

# end of file
