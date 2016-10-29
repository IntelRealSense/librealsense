#!/bin/bash
#
# Script to enable the Debian sources for kernel
#

# Ensure we only use safe paths
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin"

function update_apt_listfile {
  local found=0
  local apt_list=""

  if [ -z "$@" ]
  then
      echo "No APT list file."
      return ${found}
  else
      apt_list="$1"
  fi

  local apt_list_src="/tmp/sources.list.src.$$"
  local apt_tmp="/tmp/apt_src_diff.$$"

  # Make a copy for updates
  cp -a ${apt_list} ${apt_list_src}
  egrep -v '^\s*#|^\s*$' ${apt_list} | egrep ' main|main ' | sed -e 's/deb /deb-src /' | sort | uniq -u >> ${apt_list_src}

  cat ${apt_list_src} ${apt_list} | sort | uniq -u > ${apt_tmp}
  if [ -s ${apt_tmp} ]
  then
    echo "Updating ${apt_list} ..."
    sudo cp -a ${apt_list} ${apt_list}.save
    sudo cp -a ${apt_list_src} ${apt_list}
    sudo chown root.root ${apt_list}
    sudo rm -f ${apt_list_src}
    found=1
  else
    rm -f ${APT_LIST_SRC}
  fi

  rm -f ${APT_TMP}

  return ${found}
}

echo "Checking for APT Debian sources ..."
echo ""
echo "****************************************"
echo "  Updates will require root privileges;"
echo "  you may be prompted for your password."
echo "****************************************"
echo ""

COUNT=0

APT_FILES=$(find /etc/apt -name \*.list)
for apt_list in ${APT_FILES}
do
  update_apt_listfile $apt_list
  COUNT=$((${COUNT}+$?))
done

if [ ${COUNT} -gt 0 ]
then
  echo ""
  echo "Sources added, Updating APT ..."
  sudo apt-get update
else
  echo "No Updates required."
fi

exit 0
