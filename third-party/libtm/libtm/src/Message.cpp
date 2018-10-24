/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include "Message.h"
#include "Common.h"

/**
* @brief This function initilize the message request header with needed length and message ID
*
* @param[in] message_request - message request buffer.
* @param[in] dwLength - message request length.
* @param[in] wMessageID - message ID.
* @return None
*/
void init_message_request_header(IN unsigned char * message_request, IN uint32_t dwLength, IN uint16_t wMessageID)
{
    ((bulk_message_request_header*)message_request)->dwLength = dwLength;
    ((bulk_message_request_header*)message_request)->wMessageID = wMessageID;
}


/**
* @brief This function prints all supported request/response messages
*
* @param[in] message - message buffer.
* @return None
*/
void print_message(IN unsigned char * message)
{
    bulk_message_request_header* message_header = ((bulk_message_request_header*)message);
    unsigned int i = 0;
    unsigned int j = 0;

    printf("message->header.dwLength = %d\n", message_header->dwLength);
    printf("message->header.wMessageID = 0x%X\n", message_header->wMessageID);

    switch (message_header->wMessageID)
    {
        case (DEV_GET_DEVICE_INFO):
        {
            if (message_header->dwLength > sizeof(bulk_message_request_get_device_info))
            {
                /* bulk_message_response_get_device_info */
                bulk_message_response_get_device_info* message_response = ((bulk_message_response_get_device_info*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
                if (message_response->header.wStatus == 0)
                {
                    printf("message->bDeviceType = 0x%X\n", message_response->message.bDeviceType);
                    printf("message->bHwVersion = 0x%X\n", message_response->message.bHwVersion);
                    printf("message->bStatus = 0x%X\n", message_response->message.bStatus);
                    printf("message->wReserved = 0x%X\n", message_response->message.wReserved);
                    printf("message->dwRomVersion = 0x%X\n", message_response->message.dwRomVersion);
                    printf("message->dwFWVersion = 0x%X\n", message_response->message.dwFWVersion);
                    printf("message->dwCalibrationVersion = 0x%X\n", message_response->message.dwCalibrationVersion);
                    printf("message->llExtendedStatus = %lu\n", message_response->message.llExtendedStatus);
                }
            }
            break;
        }

        case (DEV_GET_TIME):
        {
            if (message_header->dwLength > sizeof(bulk_message_request_get_time))
            {
                /* bulk_message_response_get_time */
                bulk_message_response_get_time* message_response = ((bulk_message_response_get_time*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
                if (message_response->header.wStatus == 0)
                {
                    printf("message->llNanoseconds = %lu\n", message_response->llNanoseconds);
                }
            }
            break;
        }

        case (DEV_GET_SUPPORTED_RAW_STREAMS):
        {
            if (message_header->dwLength > sizeof(bulk_message_request_get_supported_raw_streams))
            {
                /* bulk_message_response_get_supported_raw_streams */
                bulk_message_response_get_supported_raw_streams* message_response = ((bulk_message_response_get_supported_raw_streams*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
                if (message_response->header.wStatus == 0)
                {
                    printf("message->wNumSupportedStreams = %d\n", message_response->wNumSupportedStreams);
                    if (message_response->wNumSupportedStreams > 0)
                    {
                        for (i = 0; i < message_response->wNumSupportedStreams; i++)
                        {
                            printf("message->stream[%d].bSensorID = 0x%X\n", i, message_response->stream[i].bSensorID);
                            printf("message->stream[%d].wFramesPerSecond = %d\n", i, message_response->stream[i].wFramesPerSecond);
                            printf("message->stream[%d].wWidth = %d\n", i, message_response->stream[i].wWidth);
                            printf("message->stream[%d].wHeight = %d\n", i, message_response->stream[i].wHeight);
                            printf("message->stream[%d].wReserved = %d\n", i, message_response->stream[i].wReserved);
                        }
                    }
                }
            }
            break;
        }

        case (DEV_RAW_STREAMS_CONTROL):
        {
            if (message_header->dwLength > sizeof(bulk_message_response_raw_streams_control))
            {
                /* bulk_message_request_raw_streams_control */
                bulk_message_request_raw_streams_control* message_request = ((bulk_message_request_raw_streams_control*)message);
                printf("message->wNumEnabledStreams = %d\n", message_request->wNumEnabledStreams);
                if (message_request->wNumEnabledStreams > 0)
                {
                    for (i = 0; i < message_request->wNumEnabledStreams; i++)
                    {
                        printf("message->stream[%d].bSensorID = 0x%X\n", i, message_request->stream[i].bSensorID);
                        printf("message->stream[%d].wFramesPerSecond = %d\n", i, message_request->stream[i].wFramesPerSecond);
                        printf("message->stream[%d].wWidth = %d\n", i, message_request->stream[i].wWidth);
                        printf("message->stream[%d].wHeight = %d\n", i, message_request->stream[i].wHeight);
                        printf("message->stream[%d].wReserved = %d\n", i, message_request->stream[i].wReserved);
                    }
                }
            }
            else
            {
                /* bulk_message_response_raw_streams_control */
                bulk_message_response_raw_streams_control* message_response = ((bulk_message_response_raw_streams_control*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
            }

            break;
        }

        case (DEV_GET_CAMERA_INTRINSICS):
        {
            if (message_header->dwLength == sizeof(bulk_message_request_get_camera_intrinsics))
            {
                /* bulk_message_request_get_camera_intrinsics */
                bulk_message_request_get_camera_intrinsics* message_request = ((bulk_message_request_get_camera_intrinsics*)message);
                printf("message->bCameraID = 0x%X\n", message_request->bCameraID);
            }
            else
            {
                /* bulk_message_response_get_camera_intrinsics */
                bulk_message_response_get_camera_intrinsics* message_response = ((bulk_message_response_get_camera_intrinsics*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
                if (message_response->header.wStatus == 0)
                {
                    printf("message->intrinsics.dwWidth = %d\n", message_response->intrinsics.dwWidth);
                    printf("message->intrinsics.dwHeight = %d\n", message_response->intrinsics.dwHeight);
                    printf("message->intrinsics.flPpx = %f\n", message_response->intrinsics.flPpx);
                    printf("message->intrinsics.flPpy = %f\n", message_response->intrinsics.flPpy);
                    printf("message->intrinsics.flFx = %f\n", message_response->intrinsics.flFx);
                    printf("message->intrinsics.flFy = %f\n", message_response->intrinsics.flFy);
                    printf("message->intrinsics.dwDistortionModel = %d\n", message_response->intrinsics.dwDistortionModel);
                    for (i = 0; i < 5; i++)
                    {
                        printf("message->intrinsics.flCoeffs[%d] = %f\n", i, message_response->intrinsics.flCoeffs[i]);
                    }
                }
            }
            break;
        }

        case (DEV_GET_MOTION_INTRINSICS):
        {
            if (message_header->dwLength == sizeof(bulk_message_request_get_motion_intrinsics))
            {
                /* bulk_message_request_get_motion_module_intrinsics */
                bulk_message_request_get_motion_intrinsics* message_request = ((bulk_message_request_get_motion_intrinsics*)message);
                printf("message->bMotionID = 0x%X\n", message_request->bMotionID);
            }
            else
            {
                /* bulk_message_response_get_motion_module_intrinsics */
                bulk_message_response_get_motion_intrinsics* message_response = ((bulk_message_response_get_motion_intrinsics*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
                if (message_response->header.wStatus == 0)
                {
                    for (i = 0; i < 3; i++)
                    {
                        for (j = 0; j < 4; j++)
                        {
                            printf("message->intrinsics.flData[%d][%d] = %f\n", i, j, message_response->intrinsics.flData[i][j]);
                        }
                    }

                    for (i = 0; i < 3; i++)
                    {
                        printf("message->intrinsics.flNoiseVariances[%d] = %f\n", i, message_response->intrinsics.flNoiseVariances[i]);
                    }

                    for (i = 0; i < 3; i++)
                    {
                        printf("message->intrinsics.flBiasVariances[%d] = %f\n", i, message_response->intrinsics.flBiasVariances[i]);
                    }
                }
            }
            break;
        }

        case (DEV_GET_EXTRINSICS):
        {
            if (message_header->dwLength == sizeof(bulk_message_request_get_extrinsics))
            {
                /* bulk_message_request_get_motion_module_intrinsics */
                bulk_message_request_get_extrinsics* message_request = ((bulk_message_request_get_extrinsics*)message);
                printf("message->bSensorID = 0x%X\n", message_request->bSensorID);
                printf("message->bReferenceSensorID = 0x%X\n", message_request->bReferenceSensorID);
            }
            else
            {
                /* bulk_message_response_get_extrinsics */
                bulk_message_response_get_extrinsics* message_response = ((bulk_message_response_get_extrinsics*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
                if (message_response->header.wStatus == 0)
                {
                    for (i = 0; i < 9; i++)
                    {
                        printf("message->extrinsics.flRotation[%d] = %f\n", i, message_response->extrinsics.flRotation[i]);
                    }

                    for (i = 0; i < 3; i++)
                    {
                        printf("message->extrinsics.flTranslation[%d] = %f\n", i, message_response->extrinsics.flTranslation[i]);
                    }
                }
            }
            break;
        }

        case (DEV_SET_CAMERA_INTRINSICS):
        {
            if (message_header->dwLength == sizeof(bulk_message_request_set_camera_intrinsics))
            {
                /* bulk_message_request_set_camera_intrinsics */
                bulk_message_request_set_camera_intrinsics* message_request = ((bulk_message_request_set_camera_intrinsics*)message);
                printf("message->bCameraID = 0x%X\n", message_request->bCameraID);
                printf("message->bReserved = %d\n", message_request->bReserved);
                printf("message->intrinsics.dwWidth = %d\n", message_request->intrinsics.dwWidth);
                printf("message->intrinsics.dwHeight = %d\n", message_request->intrinsics.dwHeight);
                printf("message->intrinsics.flPpx = %f\n", message_request->intrinsics.flPpx);
                printf("message->intrinsics.flPpy = %f\n", message_request->intrinsics.flPpy);
                printf("message->intrinsics.flFx = %f\n", message_request->intrinsics.flFx);
                printf("message->intrinsics.flFy = %f\n", message_request->intrinsics.flFy);
                printf("message->intrinsics.dwDistortionModel = %d\n", message_request->intrinsics.dwDistortionModel);
                for (i = 0; i < 5; i++)
                {
                    printf("message->intrinsics.flCoeffs[%d] = %f\n", i, message_request->intrinsics.flCoeffs[i]);
                }
            }
            else
            {
                /* bulk_message_response_set_camera_intrinsics */
                bulk_message_response_set_camera_intrinsics* message_response = ((bulk_message_response_set_camera_intrinsics*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
            }
            break;
        }

        case (DEV_SET_MOTION_INTRINSICS):
        {
            if (message_header->dwLength == sizeof(bulk_message_request_set_motion_intrinsics))
            {
                /* bulk_message_request_set_camera_intrinsics */
                bulk_message_request_set_motion_intrinsics* message_request = ((bulk_message_request_set_motion_intrinsics*)message);
                printf("message->bMotionID = 0x%X\n", message_request->bMotionID);
                printf("message->bReserved = %d\n", message_request->bReserved);
                for (i = 0; i < 3; i++)
                {
                    for (j = 0; j < 4; j++)
                    {
                        printf("message->intrinsics.flData[%d][%d] = %f\n", i, j, message_request->intrinsics.flData[i][j]);
                    }
                }

                for (i = 0; i < 3; i++)
                {
                    printf("message->intrinsics.flNoiseVariances[%d] = %f\n", i, message_request->intrinsics.flNoiseVariances[i]);
                }

                for (i = 0; i < 3; i++)
                {
                    printf("message->intrinsics.flBiasVariances[%d] = %f\n", i, message_request->intrinsics.flBiasVariances[i]);
                }
            }
            else
            {
                /* bulk_message_response_set_motion_intrinsics */
                bulk_message_response_set_motion_intrinsics* message_response = ((bulk_message_response_set_motion_intrinsics*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
            }
            break;
        }

        case (DEV_SET_EXTRINSICS):
        {
            if (message_header->dwLength == sizeof(bulk_message_request_set_extrinsics))
            {
                /* bulk_message_request_set_extrinsics */
                bulk_message_request_set_extrinsics* message_request = ((bulk_message_request_set_extrinsics*)message);
                printf("message->bSensorID = 0x%X\n", message_request->bSensorID);
                printf("message->bReferenceSensorID = %d\n", message_request->bReferenceSensorID);
                for (i = 0; i < 9; i++)
                {
                    printf("message->extrinsics.flRotation[%d] = %f\n", i, message_request->extrinsics.flRotation[i]);
                }

                for (i = 0; i < 3; i++)
                {
                    printf("message->extrinsics.flTranslation[%d] = %f\n", i, message_request->extrinsics.flTranslation[i]);
                }
            }
            else
            {
                /* bulk_message_response_set_extrinsics */
                bulk_message_response_set_extrinsics* message_response = ((bulk_message_response_set_extrinsics*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
            }
            break;
        }

        case (DEV_LOG_CONTROL):
        {
            if (message_header->dwLength == sizeof(bulk_message_request_log_control))
            {
                /* bulk_message_request_log_control */
                bulk_message_request_log_control* message_request = ((bulk_message_request_log_control*)message);
                printf("message->bVerbosity = %d\n", message_request->bVerbosity);
                printf("message->bLogMode = 0x%X\n", message_request->bLogMode);
            }
            else
            {
                /* bulk_message_response_log_control */
                bulk_message_response_log_control* message_response = ((bulk_message_response_log_control*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
            }
            break;
        }

        case (DEV_GET_POSE):
        {
            if (message_header->dwLength > sizeof(bulk_message_request_get_pose))
            {
                /* bulk_message_response_get_pose & interrupt_message_get_pose */
                bulk_message_response_get_pose* message_response = ((bulk_message_response_get_pose*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
                if (message_response->header.wStatus == 0)
                {
                    printf("message->pose.flX = %f\n", message_response->pose.flX);
                    printf("message->pose.flY = %f\n", message_response->pose.flY);
                    printf("message->pose.flZ = %f\n", message_response->pose.flZ);
                    printf("message->pose.flQi = %f\n", message_response->pose.flQi);
                    printf("message->pose.flQj = %f\n", message_response->pose.flQj);
                    printf("message->pose.flQk = %f\n", message_response->pose.flQk);
                    printf("message->pose.flQr = %f\n", message_response->pose.flQr);
                    printf("message->pose.flVx = %f\n", message_response->pose.flVx);
                    printf("message->pose.flVy = %f\n", message_response->pose.flVy);
                    printf("message->pose.flVz = %f\n", message_response->pose.flVz);
                    printf("message->pose.flVAX = %f\n", message_response->pose.flVAX);
                    printf("message->pose.flVAY = %f\n", message_response->pose.flVAY);
                    printf("message->pose.flVAZ = %f\n", message_response->pose.flVAZ);
                    printf("message->pose.flAx = %f\n", message_response->pose.flAx);
                    printf("message->pose.flAy = %f\n", message_response->pose.flAy);
                    printf("message->pose.flAz = %f\n", message_response->pose.flAz);
                    printf("message->pose.flAAX = %f\n", message_response->pose.flAAX);
                    printf("message->pose.flAAY = %f\n", message_response->pose.flAAY);
                    printf("message->pose.flAAZ = %f\n", message_response->pose.flAAZ);
                    printf("message->pose.llNanoseconds = %lu\n", message_response->pose.llNanoseconds);
                }
            }
            break;
        }

        case (SLAM_GET_OCCUPANCY_MAP_TILES):
        {
            if (message_header->dwLength > sizeof(bulk_message_request_get_occupancy_map_tiles))
            {
                /* bulk_message_response_get_occupancy_map_tiles */
                bulk_message_response_get_occupancy_map_tiles* message_response = ((bulk_message_response_get_occupancy_map_tiles*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
                if (message_response->header.wStatus == 0)
                {
                    printf("message->bAccuracy = %d\n", message_response->map_tiles.bAccuracy);
                    printf("message->wTileCount = %d\n", message_response->map_tiles.wTileCount);

                    if (message_response->map_tiles.wTileCount > 0)
                    {
                        for (i = 0; i < message_response->map_tiles.wTileCount; i++)
                        {
                            printf("message->tiles[%d].dwX = %d\n", i, message_response->map_tiles.tiles[i].dwX);
                            printf("message->tiles[%d].dwY = %d\n", i, message_response->map_tiles.tiles[i].dwY);
                            printf("message->tiles[%d].dwZ = %d\n", i, message_response->map_tiles.tiles[i].dwZ);
                        }
                    }
                }
            }
            break;
        }

        case (SLAM_GET_LOCALIZATION_DATA):
        {
            if (message_header->dwLength > sizeof(bulk_message_request_get_localization_data))
            {
                /* bulk_message_response_get_localization_data */
                bulk_message_response_get_localization_data* message_response = ((bulk_message_response_get_localization_data*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
                if (message_response->header.wStatus == 0)
                {
                    int localizationDataSize = message_response->header.dwLength - offsetof(bulk_message_response_get_localization_data, message);
                    if (localizationDataSize > 0)
                    {
                        printf("message->bLocalizationData:\n");
                        print_data(&message_response->message.bLocalizationData[0], localizationDataSize);
                    }
                }
            }
            break;
        }

        case (SLAM_SET_LOCALIZATION_DATA):
        {
            if (message_header->dwLength > sizeof(bulk_message_response_set_localization_data))
            {
                /* bulk_message_request_set_localization_data - Printing bLocalizationData is TBD*/
                /* bulk_message_request_set_localization_data* message_request = ((bulk_message_request_set_localization_data*)message); */
                /* Printing bLocalizationData is TBD */
            }
            else
            {
                /* bulk_message_response_set_localization_data */
                bulk_message_response_set_localization_data* message_response = ((bulk_message_response_set_localization_data*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
            }
            break;
        }

        case (SLAM_SET_6DOF_INTERRUPT_RATE):
        {
            if (message_header->dwLength == sizeof(bulk_message_request_set_6dof_interrupt_rate))
            {
                /* bulk_message_request_set_6dof_interrupt_rate */
                bulk_message_request_set_6dof_interrupt_rate* message_request = ((bulk_message_request_set_6dof_interrupt_rate*)message);
                printf("message->bInterruptRate = %d\n", message_request->message.bInterruptRate);
            }
            else
            {
                /* bulk_message_response_set_6dof_interrupt_rate */
                bulk_message_response_set_6dof_interrupt_rate* message_response = ((bulk_message_response_set_6dof_interrupt_rate*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
            }
            break;
        }

        case (SLAM_6DOF_CONTROL):
        {
            if (message_header->dwLength == sizeof(bulk_message_request_6dof_control))
            {
                /* bulk_message_request_6dof_control */
                bulk_message_request_6dof_control* message_request = ((bulk_message_request_6dof_control*)message);
                printf("message->bEnable = %d\n", message_request->bEnable);
            }
            else
            {
                /* bulk_message_response_6dof_control */
                bulk_message_response_6dof_control* message_response = ((bulk_message_response_6dof_control*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
            }
            break;
        }

        case (SLAM_OCCUPANCY_MAP_CONTROL):
        {
            if (message_header->dwLength > sizeof(bulk_message_response_occupancy_map_control))
            {
                /* bulk_message_request_occupancy_map_control */
                bulk_message_request_occupancy_map_control* message_request = ((bulk_message_request_occupancy_map_control*)message);
                printf("message->bEnable = %d\n", message_request->message.bEnable);
            }
            else
            {
                /* bulk_message_response_occupancy_map_control */
                bulk_message_response_occupancy_map_control* message_response = ((bulk_message_response_occupancy_map_control*)message);
                printf("message->header->wStatus = %d\n", message_response->header.wStatus);
            }
            break;
        }

        case (DEV_GET_AND_CLEAR_EVENT_LOG): /* Fall through */
        default:
        {
            printf("Print unsupported\n");
            break;
        }

    }
    printf("\n");

}
