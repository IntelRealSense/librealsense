%define version_maj %{getenv:VERSION_MAJOR}
%define version_min %{getenv:VERSION_MINOR}
%define version %{version_maj}.%{version_min}

Name:		librealsense
Version:	%{version}
Release:	1%{?dist}
Summary:	Intel® RealSense™ SDK 2.0 is a cross-platform library for Intel® RealSense™ depth cameras (D400 series and the SR300).

License:	ASL 2.0
URL:		https://github.com/IntelRealSense/librealsense
Source0:	%{name}-%{version}.tar.gz

BuildRequires: rpm-build
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: libusb-devel
BuildRequires: libX11-devel
BuildRequires: libXrandr-devel
BuildRequires: libXinerama-devel
BuildRequires: libXcursor-devel
BuildRequires: libglvnd-devel
BuildRequires: gtk3-devel

%description
The SDK allows depth and color streaming, and provides intrinsic and extrinsic calibration information.
The library also offers synthetic streams (pointcloud, depth aligned to color and vise-versa), and a built-in support for [record and playback](./src/media/readme.md) of streaming sessions.

%package tools
Summary: Intel® RealSense™ SDK 2.0 tools
Requires: %{name} = %{version}-%{release}
%description tools
Intel® RealSense™ SDK 2.0 tools

%package devel
Summary: Intel® RealSense™ SDK 2.0 development libraries
Requires: %{name} = %{version}-%{release}
Requires: pkgconfig
%description devel
Intel® RealSense™ SDK 2.0 development libraries

%prep
%setup -q

%build
%cmake .
%make_build

%install
%make_install

%check
ctest -V %{?_smp_mflags}

%files
%{_libdir}/librealsense2.so
%{_libdir}/librealsense2.so.{%version_maj}
%{_libdir}/librealsense2.so.{%version}

%files tools
%{_bindir}/realsense-viewer
%{_bindir}/rs-align
%{_bindir}/rs-align-advanced
%{_bindir}/rs-callback
%{_bindir}/rs-capture
%{_bindir}/rs-color
%{_bindir}/rs-convert
%{_bindir}/rs-data-collect
%{_bindir}/rs-depth
%{_bindir}/rs-depth-quality
%{_bindir}/rs-distance
%{_bindir}/rs-enumerate-devices
%{_bindir}/rs-fw-logger
%{_bindir}/rs-hello-realsense
%{_bindir}/rs-measure
%{_bindir}/rs-motion
%{_bindir}/rs-multicam
%{_bindir}/rs-pointcloud
%{_bindir}/rs-post-processing
%{_bindir}/rs-record-playback
%{_bindir}/rs-rosbag-inspector
%{_bindir}/rs-save-to-disk
%{_bindir}/rs-sensor-control
%{_bindir}/rs-software-device
%{_bindir}/rs-terminal
%{_bindir}/rs-trajectory
%{_bindir}/rs-pose
%{_bindir}/rs-pose-predict
%{_bindir}/rs-ar-basic
%{_bindir}/rs-pose-and-image

%files devel
%dir %{_includedir}/librealsense2
%{_includedir}/librealsense2/h/rs_advanced_mode_command.h
%{_includedir}/librealsense2/h/rs_config.h
%{_includedir}/librealsense2/h/rs_context.h
%{_includedir}/librealsense2/h/rs_device.h
%{_includedir}/librealsense2/h/rs_frame.h
%{_includedir}/librealsense2/h/rs_internal.h
%{_includedir}/librealsense2/h/rs_option.h
%{_includedir}/librealsense2/h/rs_pipeline.h
%{_includedir}/librealsense2/h/rs_processing.h
%{_includedir}/librealsense2/h/rs_record_playback.h
%{_includedir}/librealsense2/h/rs_sensor.h
%{_includedir}/librealsense2/h/rs_types.h
%{_includedir}/librealsense2/hpp/rs_options.hpp
%{_includedir}/librealsense2/hpp/rs_context.hpp
%{_includedir}/librealsense2/hpp/rs_device.hpp
%{_includedir}/librealsense2/hpp/rs_export.hpp
%{_includedir}/librealsense2/hpp/rs_frame.hpp
%{_includedir}/librealsense2/hpp/rs_internal.hpp
%{_includedir}/librealsense2/hpp/rs_pipeline.hpp
%{_includedir}/librealsense2/hpp/rs_processing.hpp
%{_includedir}/librealsense2/hpp/rs_record_playback.hpp
%{_includedir}/librealsense2/hpp/rs_sensor.hpp
%{_includedir}/librealsense2/hpp/rs_types.hpp
%{_includedir}/librealsense2/rs.h
%{_includedir}/librealsense2/rs.hpp
%{_includedir}/librealsense2/rs_advanced_mode.h
%{_includedir}/librealsense2/rs_advanced_mode.hpp
%{_includedir}/librealsense2/rsutil.h
%{_libdir}/cmake/realsense2/realsense2Config.cmake
%{_libdir}/cmake/realsense2/realsense2ConfigVersion.cmake
%{_libdir}/cmake/realsense2/realsense2Targets-noconfig.cmake
%{_libdir}/cmake/realsense2/realsense2Targets.cmake
%{_libdir}/librealsense-file.a
%{_libdir}/librealsense2.so
%{_libdir}/libtm.a
%{_libdir}/pkgconfig/realsense2.pc

%changelog
