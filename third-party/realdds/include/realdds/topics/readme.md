
The files in this directory are, for the most part, automatically generated from files in
    `third-party/realdds/include/topics/*/*.idl`


## Topics list

The various topics are arranged hierarchically:

* `realsense/`
	* `device-info` — device name, serial-number, product-line, lock status, and topic root enumeration
	* `<model>/`
		* `<serial-number>/` — topic root per device pointed-to in device-info
			* `control` — client -> camera messaging
			* `notification` — camera -> client notifications, responses, etc.
			* `<stream-name>` — frame-based streaming for each stream enumerating upon client subscription
				* `metadata`

For a complete overview of the topics structure please refer to [DDS ICD]("https://github.com/IntelRealSense/librealsense/blob/dds/third-party/realdds/doc/DDS ICD.md")

## Generating

A utility from FastDDS called [FastDDSGen](https://fast-dds.docs.eprosima.com/en/latest/fastddsgen/introduction/introduction.html#fastddsgen-intro) is required to convert the [IDL](https://fast-dds.docs.eprosima.com/en/latest/fastddsgen/dataTypes/dataTypes.html) files to the headers that're needed.
Unfortunately, this utility requires a java installation and sometimes not even the latest.

Instead, we found it easiest to download a Docker image for FastDDS (last used is version 2.7.1) and generate the files within.

1. Download the Docker image from [here](https://www.eprosima.com/index.php?option=com_ars&view=browses&layout=normal) (link was valid at time of writing; if no longer, register with eProsima and update it)

    ```zsh
    docker load -i /mnt/c/work/ubuntu-fastdds_v2.7.1.tar
    ```

2. In a `zsh` shell:

    ```zsh
    # from third-party/realdds/
    #
    for topic in control device-info image notification
    do
    cd include/realdds/topics/${topic}
    cid=`docker run -itd --privileged ubuntu-fastdds:v2.7.1`
    docker exec $cid mkdir /idl /idl/out
    docker cp *.idl $cid:idl/
    docker exec -w /idl/out $cid fastddsgen -typeobject /idl/`ls -1 *.idl`
    docker cp $cid:/idl/out .
    docker kill $cid
    cd out
    for cxx in *.cxx; do mv -- "$cxx" "../../../../../src/topics/${cxx%.cxx}.cpp"; done
    mv -- *TypeObject.h "../../../../../src/topics/"
    mv * ..
    cd ..
    rmdir out
    cd ../../../..
    done
    ```

    This automatically renames .cxx to .cpp, places them in the right directory, etc.

3. Once files are generated, they still need some manipulation:
	* Certain `#include` statements (mainly in the .cpp files moved into src/) need to be updated (e.g., `#include "control.h"`  changed to `#include <realdds/topics/control/control.h>`)
	* Copyright notices in all the files needs updating to LibRealSense's
