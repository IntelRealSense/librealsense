#!/bin/bash

# This script is designed to ensure all relevant header and source files contain correct copyright

set +e

sudo apt-get install dos2unix

ok=0
fixed=0

function check_folder {
    for filename in $(find $1 -type f \( -iname \*.cpp -o -iname \*.h -o -iname \*.hpp -o -iname \*.js -o -iname \*.bat -o -iname \*.sh -o -iname \*.txt \)); do
          if [[ $(grep -oP "Software License Agreement" $filename | wc -l) -ne 0 ]]; then
               echo "[WARNING] $filename contains 3rd-party license agreement"
          else
               if [[ ! $filename == *"usbhost"* ]]; then
                    # Only check files that are not .gitignore-d
                    if [[ $(git check-ignore $filename | wc -l) -eq 0 ]]; then
                         if [[ $(grep -oP "(?<=\(c\) )(.*)(?= Intel)" $filename | wc -l) -eq 0 ]]; then
                              echo "[ERROR] $filename is missing the copyright notice"
                              ok=$((ok+1))

                              if [[ $2 == *"fix"* ]]; then
                                   if [[ $(date +%Y) == "2019" ]]; then
                                        if [[ $filename == *".hpp"* ]]; then
                                             echo "Trying to auto-resolve...";
                                             ex -sc '1i|// Copyright(c) 2019 Intel Corporation. All Rights Reserved.' -cx $filename
                                             fixed=$((fixed+1))
                                        fi
                                        if [[ $filename == *".cpp"* ]]; then
                                             echo "Trying to auto-resolve...";
                                             ex -sc '1i|// Copyright(c) 2019 Intel Corporation. All Rights Reserved.' -cx $filename
                                             fixed=$((fixed+1))
                                        fi
                                        if [[ $filename == *".h"* ]]; then
                                             echo "Trying to auto-resolve...";
                                             ex -sc '1i|/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */' -cx $filename
                                             fixed=$((fixed+1))
                                        fi
                                        if [[ $filename == *".js"* ]]; then
                                             echo "Trying to auto-resolve...";
                                             ex -sc '1i|// Copyright(c) 2019 Intel Corporation. All Rights Reserved.' -cx $filename
                                             fixed=$((fixed+1))
                                        fi
                                        if [[ $filename == *".txt"* ]]; then
                                             echo "Trying to auto-resolve...";
                                             ex -sc '1i|# Copyright(c) 2019 Intel Corporation. All Rights Reserved.' -cx $filename
                                             fixed=$((fixed+1))
                                        fi
                                   else
                                        echo Please update pr_check to auto-resolve missing copyright
                                   fi
                              fi
                         fi

                         if [[ $(grep -oP "Apache 2.0" $filename | wc -l) -eq 0 ]]; then
                              echo "[ERROR] $filename is missing license notice"
                              ok=$((ok+1))

                              if [[ $2 == *"fix"* ]]; then
                                   if [[ $filename == *".hpp"* ]]; then
                                        echo "Trying to auto-resolve...";
                                        ex -sc '1i|// License: Apache 2.0. See LICENSE file in root directory.' -cx $filename
                                        fixed=$((fixed+1))
                                   fi
                                   if [[ $filename == *".cpp"* ]]; then
                                        echo "Trying to auto-resolve...";
                                        ex -sc '1i|// License: Apache 2.0. See LICENSE file in root directory.' -cx $filename
                                        fixed=$((fixed+1))
                                   fi
                                   if [[ $filename == *".h"* ]]; then
                                        echo "Trying to auto-resolve...";
                                        ex -sc '1i|/* License: Apache 2.0. See LICENSE file in root directory. */' -cx $filename
                                        fixed=$((fixed+1))
                                   fi
                                   if [[ $filename == *".js"* ]]; then
                                        echo "Trying to auto-resolve...";
                                        ex -sc '1i|// License: Apache 2.0. See LICENSE file in root directory.' -cx $filename
                                        fixed=$((fixed+1))
                                   fi
                                   if [[ $filename == *".txt"* ]]; then
                                        echo "Trying to auto-resolve...";
                                        ex -sc '1i|# License: Apache 2.0. See LICENSE file in root directory.' -cx $filename
                                        fixed=$((fixed+1))
                                   fi
                              fi
                         fi

                         if [[ $(grep -o -P '\t' $filename | wc -l) -ne 0 ]]; then
                              echo "[ERROR] $filename has tabs (this project is using spaces as delimiters)"
                              ok=$((ok+1))

                              if [[ $2 == *"fix"* ]]; then
                                   echo "Trying to auto-resolve...";
                                   sed -i.bak $'s/\t/    /g' $filename
                                   fixed=$((fixed+1))
                              fi
                         fi

                         if [[ $(file ${filename} | grep -o -P 'CRLF' | wc -l) -ne 0 ]]; then
                              echo "[ERROR] $filename is using DOS line endings (this project is using Unix line-endings)"
                              ok=$((ok+1))

                              if [[ $2 == *"fix"* ]]; then
                                   echo "Trying to auto-resolve...";
                                   dos2unix $filename
                                   fixed=$((fixed+1))
                              fi
                         fi
                    fi
               fi
          fi
     done
}

if [[ $1 == *"help"* ]]; then
     echo Pull-Request Check tool
     echo "Usage: (run from repo scripts directory)"
     echo "    ./pr_check.sh [--help] [--fix]"
     echo "    --fix    Try to auto-fix defects"
     exit 0
fi
	
cd ..
check_folder include $1
check_folder src $1
check_folder examples $1
check_folder third-party/libtm $1
check_folder tools $1
cd scripts

if [[ ${fixed} -ne 0 ]]; then
     echo "Re-running pr_check..."
     ./pr_check.sh
else
     if [[ ${ok} -ne 0 ]]; then
          echo Pull-Request check failed, please address ${ok} the errors reported above
          exit 1
     fi
fi

exit 0
