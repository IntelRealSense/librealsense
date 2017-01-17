function require_package {
   package_name=$1
   printf "verify package=%s\n" "${package_name}"
	if [ $(dpkg-query -W -f='${Status}' ${package_name} 2>/dev/null | grep -c "ok installed") -eq 0 ];
	then
		printf "Installing %s package" ${package_name}
		sudo apt-get install ${package_name}
	fi
}

function try_module_insert {
	module_name=$1
	src_ko=$2
	tgt_ko=$3
	backup_available=1
	
	# Unload existing modules if resident	
	printf "Unloading module %s\n" ${module_name}
	sudo modprobe -r ${module_name}
	
	# backup the existing module (if available) for recovery
	if [ -f ${tgt_ko} ];
	then
		sudo cp ${tgt_ko} ${tgt_ko}.bckup
	else
		backup_available=0
	fi

	# copy the patched module to target location	
	sudo cp ${src_ko} ${tgt_ko}
	
	# try to load the new module
	set --
	modprobe_failed=0
	printf "trying to insert patched %s\n" ${module_name}
	sudo modprobe ${module_name} || modprobe_failed=$?

	# Check and revert the backup module if 'modprobe' operation crashed
	if [ $modprobe_failed -ne 0 ];
	then
		echo "Failed to insert the patched module. Operation is aborted, the original module is restored"
		echo "Verify that the current kernel version is aligned to the patched module versoin"
		if [ backup_available -ne 0 ];
		then
			sudo cp ${tgt_ko}.bckup ${tgt_ko}
			sudo modprobe ${module_name}
			printf "The original %s module was reloaded\n" ${module_name}
		fi
		exit 1
	else
		# Everything went OK, delete backup
		printf "Inserting the patched module %s succeeded\n" ${module_name}
		sudo rm ${tgt_ko}.bckup
	fi
}

#backup the existing modules
#sudo cp /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko.bckup
#sudo cp /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko.bckup
#sudo cp /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko.bckup

## Copy the patched modules out to target directory for deployment
#sudo cp ~/$LINUX_BRANCH-uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
#sudo cp ~/$LINUX_BRANCH-hid-sensor-accel-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko
#sudo cp ~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko
