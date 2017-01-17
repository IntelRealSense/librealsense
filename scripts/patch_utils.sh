function require_package {
   package_name=$1
   printf "verify package=%s\n" "${package_name}"
	if [ $(dpkg-query -W -f='${Status}' ${package_name} 2>/dev/null | grep -c "ok installed") -eq 0 ];
	then
		printf "Installing %s package" ${package_name}
		sudo apt-get install ${package_name}
	fi
}
