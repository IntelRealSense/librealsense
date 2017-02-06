function require_package {
	package_name=$1
	printf "\e[32mPackage required %s: \e[0m" "${package_name}"
	if [ $(dpkg-query -W -f='${Status}' ${package_name} 2>/dev/null | grep -c "ok installed") -eq 0 ];
	then
		echo -e "\e[31m - not found, installing now...\e[0m"
		sudo apt-get install ${package_name}
		echo -e "\e[32mMissing package installed\e[0m"
	else
		echo -e "\e[32m - found\e[0m"
	fi
}

function try_module_insert {
	module_name=$1
	src_ko=$2
	tgt_ko=$3
	backup_available=1
	
	# Unload existing modules if resident	
	printf "\e[35mUnloading module %s\n\e[0m" ${module_name}
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
	modprobe_failed=0
	printf "\e[36mApplying patched module %s\n\e[0m" ${module_name}
	sudo modprobe ${module_name} || modprobe_failed=$?

	# Check and revert the backup module if 'modprobe' operation crashed
	if [ $modprobe_failed -ne 0 ];
	then
		echo -e "\e[31mFailed to insert the patched module. Operation is aborted, the original module is restored\e[0m"
		echo -e "\e[31mVerify that the current kernel version is aligned to the patched module version\e[0m"
		if [ backup_available -ne 0 ];
		then
			sudo cp ${tgt_ko}.bckup ${tgt_ko}
			sudo modprobe ${module_name}
			printf "\e[34mThe original %s module was reloaded\n\e[0m" ${module_name}
		fi
		exit 1
	else
		# Everything went OK, delete backup
		printf "\e[32mInserting %s succeeded\n\n\e[0m" ${module_name}
		sudo rm ${tgt_ko}.bckup
	fi
}
