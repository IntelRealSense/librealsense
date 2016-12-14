#!/bin/bash -e

set -e

echo -e "\e[4m\nHost Info:\e[0m\n"
lsb_release -a

echo -e "\e[4m\nKernel Version:\e[0m\n"
uname -mrs

#The patch requires v4l-utils installed
if [ $(dpkg-query -W -f='${Status}' v4l-utils 2>/dev/null | grep -c "ok installed") -eq 0 ];
then
  apt-get install v4l-utils;
fi

#
echo -e "\e[4m\nConnected UVC Devices: \n\e[0m"

#Verify that the camera is properly registered with USB driver
if [ -c /dev/video0 ]; then
    echo "UVC streaming device was found..."
else
	echo -e "\e[31mError\e[0m: No UVC Camera was found, make is it is properly connected to USB3 port and retry"
	exit 1;
fi

#Verify a single ZR300 is properly connected
DS=$(lsusb | grep -i "8086:0ad0" | wc -l)
FE=$(lsusb | grep -i "8086:0acb" | wc -l)
if [ "${DS}" -eq 0 ] && [ "${FE}" -eq 0 ]; then
	echo -e "\e[31m\nError\e[0m: No ZR300 cameras were found,"
	echo -e "Make sure that the device is firmly connected\
 to USB3 port and rerun the script...\e[0m\n"
	exit 1;
else	
	echo -e "\nZR300 Camera was found - \e[32mOk\e[0m"
fi


video_dev_list=$(ls /dev/video* 2>/dev/null)
#Use temp to stored intermediate results
rm -f ./$$.txt

devices_array=(${video_dev_list// / })
for i in "${!devices_array[@]}"
do
    echo "${devices_array[i]} :" >> ./streaming_format.txt    
    v4l2-ctl --list-formats-ext -d ${devices_array[i]} | grep -B1 "Name" >> ./$$.txt
done

echo -e "\e[4m\nVerify formats required by ZR300 Camera\e[0m: \n"
RECOGNIZED_PATCHED_FORMATS=$(egrep '\(Y8I|Y12I|Y16|Z16|GREY|YUYV\)' \
    ./$$.txt | wc -l)

#Check supported vs required by ZR300 formats
REQUIRED_FMT="Y8I:Y12I:Y16:Z16:GREY:YUYV:RW16"
required_formats_array=(${REQUIRED_FMT//:/ })

fmt_patch_in_place=1
for i in "${!required_formats_array[@]}"
do
	available_format=$(egrep ${required_formats_array[i]} ./$$.txt | wc -l)	
	driver_format=$(strings \
	/lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko \
	 | egrep ${required_formats_array[i]} | wc -l)	
	if [ ${driver_format} -ne 0 ];	then		
		echo -e "${required_formats_array[i]}\t-\t\e[32mOk\e[0m"		
	else		
		echo -e "\e[31m${required_formats_array[i]} \t-\tFail\e[0m"
		fmt_patch_in_place=0
	fi
done

#Find all available streaming profiles that appear unrecognized
if [ $(egrep -o '[[:alnum:]]{1,8}-[[:alnum:]]{1,4}-[[:alnum:]]{1,4}-[[:alnum:]]{1,4}-[[:alnum:]]{1,5}' ./$$.txt | wc -l) -ne 0 ];
then
	unknown_formats=$(egrep -o '[[:alnum:]]{1,8}-[[:alnum:]]{1,4}-[[:alnum:]]{1,4}-[[:alnum:]]{1,4}-[[:alnum:]]{1,5}' ./$$.txt | wc -l)
	unknown_fmt_list=(${unpatched_formats// / })
	echo -e "\e[4m\nUnrecognized formats\e[0m: \n"
	for i in "${!unknown_fmt_list[@]}"
	do
		echo -e "\e[31m ${unknown_fmt_list[i]}\e[0m "
	done
fi
rm -f ./$$.txt


if [ "${fmt_patch_in_place}" -eq 0 ]; then
	echo -e "\e[31m\n\n\nThe formats patch has not been applied correctly!\e[0m \n"
else
	echo -e "\e[32m\nThe pixel formats are set properly!\e[0m \n"
fi

echo -e "Done..."
