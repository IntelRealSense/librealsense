/*****************************************************************************
*                                                                            *
*  OpenNI 2.x Alpha                                                          *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/
#include "OniPlatform.h"

#define ONI_VERSION_MAJOR	2
#define ONI_VERSION_MINOR	2
#define ONI_VERSION_MAINTENANCE	0
#define ONI_VERSION_BUILD	33

/** OpenNI version (in brief string format): "Major.Minor.Maintenance (Build)" */ 
#define ONI_BRIEF_VERSION_STRING \
	ONI_STRINGIFY(ONI_VERSION_MAJOR) "." \
	ONI_STRINGIFY(ONI_VERSION_MINOR) "." \
	ONI_STRINGIFY(ONI_VERSION_MAINTENANCE) \
	" (Build " ONI_STRINGIFY(ONI_VERSION_BUILD) ")"

/** OpenNI version (in numeric format): (OpenNI major version * 100000000 + OpenNI minor version * 1000000 + OpenNI maintenance version * 10000 + OpenNI build version). */
#define ONI_VERSION (ONI_VERSION_MAJOR*100000000 + ONI_VERSION_MINOR*1000000 + ONI_VERSION_MAINTENANCE*10000 + ONI_VERSION_BUILD)
#define ONI_CREATE_API_VERSION(major, minor) ((major)*1000 + (minor))
#define ONI_API_VERSION ONI_CREATE_API_VERSION(ONI_VERSION_MAJOR, ONI_VERSION_MINOR)

/** OpenNI version (in string format): "Major.Minor.Maintenance.Build-Platform (MMM DD YYYY HH:MM:SS)". */ 
#define ONI_VERSION_STRING \
	ONI_BRIEF_VERSION_STRING  "-" \
	ONI_PLATFORM_STRING " (" ONI_TIMESTAMP ")"
