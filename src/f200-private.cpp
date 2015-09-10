#include "f200-private.h"
#include <iostream>

namespace rsimpl { namespace f200
{
    #define IVCAM_VID                       0x8086
    #define IVCAM_PID                       0x0A66
    #define IVCAM_MONITOR_INTERFACE         0x4
    #define IVCAM_MONITOR_ENDPOINT_OUT      0x1
    #define IVCAM_MONITOR_ENDPOINT_IN       0x81
    #define IVCAM_MONITOR_MAGIC_NUMBER      0xcdab
    #define IVCAM_MIN_SUPPORTED_VERSION     13
    #define IVCAM_MONITOR_MAX_BUFFER_SIZE   1024
    #define IVCAM_MONITOR_HEADER_SIZE       (sizeof(uint32_t)*6)
    #define IVCAM_MONITOR_MUTEX_TIMEOUT     3000

    #define NUM_OF_CALIBRATION_PARAMS       (100)
    #define PARAMETERS_BUFFER_SIZE          (50)
    #define MAX_SIZE_OF_CALIB_PARAM_BYTES   (800)
    #define SIZE_OF_CALIB_PARAM_BYTES       (512)
    #define SIZE_OF_CALIB_HEADER_BYTES      (4)

    int bcdtoint(uint8_t * buf, int bufsize)
    {
        int r = 0;
        for(int i = 0; i < bufsize; i++)
            r = r * 10 + *buf++;
        return r;
    }

    int getVersionOfCalibration(uint8_t * validation, uint8_t * version)
    {
        uint8_t valid[2] = {0X14, 0x0A};
        if (memcmp(valid, validation, 2) != 0) return 0;
        else return bcdtoint(version, 2);
    }

    //////////////////////////
    // Private Hardware I/O //
    //////////////////////////

    int IVCAMHardwareIO::PrepareUSBCommand(uint8_t * request, size_t & requestSize, uint32_t op,
                          uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4, uint8_t * data, size_t dataLength)
    {

        if (requestSize < IVCAM_MONITOR_HEADER_SIZE)
            return 0;

        int index = sizeof(uint16_t);
        *(uint16_t *)(request + index) = IVCAM_MONITOR_MAGIC_NUMBER;
        index += sizeof(uint16_t);
        *(uint32_t *)(request + index) = op;
        index += sizeof(uint32_t);
        *(uint32_t *)(request + index) = p1;
        index += sizeof(uint32_t);
        *(uint32_t *)(request + index) = p2;
        index += sizeof(uint32_t);
        *(uint32_t *)(request + index) = p3;
        index += sizeof(uint32_t);
        *(uint32_t *)(request + index) = p4;
        index += sizeof(uint32_t);

        if (dataLength)
        {
            memcpy(request + index , data, dataLength);
            index += dataLength;
        }

        // Length doesn't include header size (sizeof(uint32_t))
        *(uint16_t *)request = (uint16_t)(index - sizeof(uint32_t));
        requestSize = index;
        return index;
    }

    void IVCAMHardwareIO::ExecuteUSBCommand(uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize)
    {
        // write
        errno = 0;

        int outXfer;

        if (!usbMutex.try_lock_for(std::chrono::milliseconds(IVCAM_MONITOR_MUTEX_TIMEOUT))) throw std::runtime_error("usbMutex timed out");
        std::lock_guard<std::timed_mutex> guard(usbMutex, std::adopt_lock);

        device.bulk_transfer(IVCAM_MONITOR_ENDPOINT_OUT, out, (int) outSize, &outXfer, 1000); // timeout in ms

        // read
        if (in && inSize)
        {
            uint8_t buf[IVCAM_MONITOR_MAX_BUFFER_SIZE];

            errno = 0;

            device.bulk_transfer(IVCAM_MONITOR_ENDPOINT_IN, buf, sizeof(buf), &outXfer, 1000);
            if (outXfer < (int)sizeof(uint32_t)) throw std::runtime_error("incomplete bulk usb transfer");

            op = *(uint32_t *)buf;
            if (outXfer > (int)inSize) throw std::runtime_error("bulk transfer failed - user buffer too small");
            inSize = outXfer;
            memcpy(in, buf, inSize);
        }
    }

    void IVCAMHardwareIO::FillUSBBuffer(int opCodeNumber, int p1, int p2, int p3, int p4, char * data, int dataLength, char * bufferToSend, int & length)
    {
        uint16_t preHeaderData = IVCAM_MONITOR_MAGIC_NUMBER;

        char * writePtr = bufferToSend;
        int header_size = 4;

        // TBD !!! This may change. Need to define it as part of API
        //typeSize = sizeof(float);

        int cur_index = 2;
        *(uint16_t *)(writePtr + cur_index) = preHeaderData;
        cur_index += sizeof(uint16_t);
        *(int *)( writePtr + cur_index) = opCodeNumber;
        cur_index += sizeof(uint32_t);
        *(int *)( writePtr + cur_index) = p1;
        cur_index += sizeof(uint32_t);
        *(int *)( writePtr + cur_index) = p2;
        cur_index += sizeof(uint32_t);
        *(int *)( writePtr + cur_index) = p3;
        cur_index += sizeof(uint32_t);
        *(int *)( writePtr + cur_index) = p4;
        cur_index += sizeof(uint32_t);

        if (dataLength)
        {
            memcpy(writePtr + cur_index , data, dataLength);
            cur_index += dataLength;
        }

        length = cur_index;
        *(uint16_t *) bufferToSend = (uint16_t)(length - header_size);// Length doesn't include header

    }

    void IVCAMHardwareIO::GetCalibrationRawData(IVCAMDataSource src, uint8_t * data, size_t & bytesReturned)
    {
		IVCAMCommand command(IVCAMMonitorCommand::GetCalibrationTable);
		command.Param1 = (uint32_t) src;

		bool result = PerfomAndSendHWmonitorCommand(command);
		if (result)
		{
			memcpy(data, command.receivedCommandData, HW_MONITOR_BUFFER_SIZE);
			bytesReturned = command.receivedCommandDataLength;
		}
	}

    /*void IVCAMHardwareIO::GetCalibrationRawData(uint8_t * data, size_t & bytesReturned)
    {
        uint8_t request[IVCAM_MONITOR_HEADER_SIZE];
        size_t requestSize = sizeof(request);
        uint32_t responseOp;

        if (PrepareUSBCommand(request, requestSize, (uint32_t) IVCAMMonitorCommand::GetCalibrationTable) <= 0) 
            throw std::runtime_error("usb transfer to retrieve calibration data failed");

        ExecuteUSBCommand(request, requestSize, responseOp, data, bytesReturned);
    }*/

    void IVCAMHardwareIO::ProjectionCalibrate(uint8_t * rawCalibData, int len, CameraCalibrationParameters * calprms)
    {
        uint8_t * bufParams = rawCalibData + 4;

        IVCAMCalibration CalibrationData;
        IVCAMTesterData TesterData;

        memset(&CalibrationData, 0, sizeof(IVCAMCalibration));

        int ver = getVersionOfCalibration(bufParams, bufParams + 2);

		std::cout << "######## Calibration Version: " << ver << std::endl;

        if (ver == IVCAM_MIN_SUPPORTED_VERSION)
        {
            float *params = (float *)bufParams;

            calibration->buildParameters(params, 100);

            // calprms; // breakpoint here to debug

            memcpy(calprms, params+1, sizeof(CameraCalibrationParameters));
            memcpy(&TesterData, bufParams, SIZE_OF_CALIB_HEADER_BYTES);

            memset((uint8_t*)&TesterData+SIZE_OF_CALIB_HEADER_BYTES, 0, sizeof(IVCAMTesterData) - SIZE_OF_CALIB_HEADER_BYTES);
        }
        else if (ver > IVCAM_MIN_SUPPORTED_VERSION)
        {
            rawCalibData = rawCalibData + 4;

            int size = (sizeof(IVCAMCalibration) > len) ? len : sizeof(IVCAMCalibration);

            auto fixWithVersionInfo = [&](IVCAMCalibration &d, int size, uint8_t * data)
            {
                memcpy((uint8_t*)&d + sizeof(int), data, size - sizeof(int));
            };

            fixWithVersionInfo(CalibrationData, size, rawCalibData);

            memcpy(calprms, &CalibrationData.CalibrationParameters, sizeof(CameraCalibrationParameters));
            calibration->buildParameters(CalibrationData.CalibrationParameters);

            // calprms; // breakpoint here to debug

            memcpy(&TesterData, rawCalibData, SIZE_OF_CALIB_HEADER_BYTES);  //copy the header: valid + version

            //copy the tester data from end of calibration
            int EndOfCalibratioData = SIZE_OF_CALIB_PARAM_BYTES + SIZE_OF_CALIB_HEADER_BYTES;
            memcpy((uint8_t*)&TesterData + SIZE_OF_CALIB_HEADER_BYTES , rawCalibData + EndOfCalibratioData , sizeof(IVCAMTesterData) - SIZE_OF_CALIB_HEADER_BYTES);

            calibration->InitializeThermalData(TesterData.TemperatureData, TesterData.ThermalLoopParams);
        }
        else if (ver > 17)
        {
            throw std::runtime_error("calibration table is not compatible with this API");
        }

        // Dimitri: debugging dangerously (assume we are pulling color + depth)
        EnableTimeStamp(true, true);
    }

    /*void IVCAMHardwareIO::ProjectionCalibrate(uint8_t * rawCalibData, int len, CameraCalibrationParameters * calprms)
    {
        uint8_t * bufParams = rawCalibData + 4;

        CameraCalibrationParametersVersion CalibrationData;
        IVCAMTesterData TesterData;

        memset(&CalibrationData, 0, sizeof(CameraCalibrationParametersVersion));

        int ver = getVersionOfCalibration(bufParams, bufParams + 2);

        if (ver == IVCAM_MIN_SUPPORTED_VERSION)
        {
            float *params = (float *)bufParams;

            calibration->buildParameters(params, 100);

            // calprms; // breakpoint here to debug

            memcpy(calprms, params+1, sizeof(CameraCalibrationParameters));
            memcpy(&TesterData, bufParams, SIZE_OF_CALIB_HEADER_BYTES);

            memset((uint8_t*)&TesterData+SIZE_OF_CALIB_HEADER_BYTES,0,sizeof(IVCAMTesterData) - SIZE_OF_CALIB_HEADER_BYTES);
        }
        else if (ver > IVCAM_MIN_SUPPORTED_VERSION)
        {
            rawCalibData = rawCalibData + 4;

            int size = (sizeof(CameraCalibrationParametersVersion) > len) ? len : sizeof(CameraCalibrationParametersVersion);

            auto fixWithVersionInfo = [&](CameraCalibrationParametersVersion &d, int size, uint8_t * data)
            {
                memcpy((uint8_t*)&d + sizeof(int), data, size - sizeof(int));
            };

            fixWithVersionInfo(CalibrationData, size, rawCalibData);

            memcpy(calprms, &CalibrationData.CalibrationParameters, sizeof(CameraCalibrationParameters));
            calibration->buildParameters(CalibrationData.CalibrationParameters);

            // calprms; // breakpoint here to debug

            memcpy(&TesterData,  rawCalibData, SIZE_OF_CALIB_HEADER_BYTES);  //copy the header: valid + version

            //copy the tester data from end of calibration
            int EndOfCalibratioData = SIZE_OF_CALIB_PARAM_BYTES + SIZE_OF_CALIB_HEADER_BYTES;
            memcpy((uint8_t*)&TesterData + SIZE_OF_CALIB_HEADER_BYTES , rawCalibData + EndOfCalibratioData , sizeof(IVCAMTesterData) - SIZE_OF_CALIB_HEADER_BYTES);

            calibration->InitializeThermalData(TesterData.TemperatureData, TesterData.ThermalLoopParams);
        }
        else if (ver > 17)
        {
            throw std::runtime_error("calibration table is not compatible with this API");
        }

        // Dimitri: debugging dangerously (assume we are pulling color + depth)
        EnableTimeStamp(true, true);
    }*/

    void IVCAMHardwareIO::ForceHardwareReset()
    {
        IVCAMCommand cmd(IVCAMMonitorCommand::HWReset);
        cmd.oneDirection = true;
        PerfomAndSendHWmonitorCommand(cmd);
    }

    bool IVCAMHardwareIO::EnableTimeStamp(bool colorEnable, bool depthEnable)
    {
        IVCAMCommand cmd(IVCAMMonitorCommand::TimeStampEnable);
        cmd.Param1 = depthEnable ? 1 : 0;
        cmd.Param2 = colorEnable ? 1 : 0;
        return PerfomAndSendHWmonitorCommand(cmd);
    }

    void IVCAMHardwareIO::ReadTemperatures(IVCAMTemperatureData & data)
    {
        data = {0};

        int irTemp;
        if (!GetIRtemp(irTemp))
            throw std::runtime_error("could not get IR temperature");

        data.IRTemp = (float) irTemp;

        float memsTemp;
        if (!GetMEMStemp(memsTemp))
            throw std::runtime_error("could not get liguria temperature");

        data.LiguriaTemp = memsTemp;
    }

	bool IVCAMHardwareIO::GetFwLastError(FirmwareError & error)
	{
		IVCAMCommand cmd(IVCAMMonitorCommand::GetFWLastError);
        memset(cmd.data, 0, 4);

		bool result = PerfomAndSendHWmonitorCommand(cmd);

		if (result)
		{
			auto fwError = *reinterpret_cast<FirmwareError *>(cmd.receivedCommandData);
		}

		return false;
	}

    bool IVCAMHardwareIO::PerfomAndSendHWmonitorCommand(IVCAMCommand & newCommand)
    {
        bool result = true;

        uint32_t opCodeXmit = (uint32_t) newCommand.cmd;

        IVCAMCommandDetails details;
        details.oneDirection = newCommand.oneDirection;
        details.TimeOut = newCommand.TimeOut;

        FillUSBBuffer(opCodeXmit,
                      newCommand.Param1,
                      newCommand.Param2,
                      newCommand.Param3,
                      newCommand.Param4,
                      newCommand.data,
                      newCommand.sizeOfSendCommandData,
                      details.sendCommandData,
                      details.sizeOfSendCommandData);

        result = SendHWmonitorCommand(details);

        // Error/exit conditions
        if (result == false) return result;
        if (newCommand.oneDirection) return result;

        memcpy(newCommand.receivedOpcode, details.receivedOpcode, 4);
        memcpy(newCommand.receivedCommandData, details.receivedCommandData,details.receivedCommandDataLength);
        newCommand.receivedCommandDataLength = details.receivedCommandDataLength;

        // endian? 
        uint32_t opCodeAsUint32 = pack(details.receivedOpcode[3], details.receivedOpcode[2], details.receivedOpcode[1], details.receivedOpcode[0]);
        if (opCodeAsUint32 != opCodeXmit)
        {
            throw std::runtime_error("opcodes do not match");
        }

        return result;
    }

    bool IVCAMHardwareIO::SendHWmonitorCommand(IVCAMCommandDetails & details)
    {
        unsigned char outputBuffer[HW_MONITOR_BUFFER_SIZE];

        uint32_t op;
        size_t receivedCmdLen = HW_MONITOR_BUFFER_SIZE;

        ExecuteUSBCommand((uint8_t*) details.sendCommandData, (size_t) details.sizeOfSendCommandData, op, outputBuffer, receivedCmdLen);
        details.receivedCommandDataLength = receivedCmdLen;

        if (details.oneDirection) return true;

        if (details.receivedCommandDataLength >= 4)
        {
            details.receivedCommandDataLength -= 4;
            memcpy(details.receivedOpcode, outputBuffer, 4);
        }
        else return false;

        if (details.receivedCommandDataLength > 0 )
            memcpy(details.receivedCommandData, outputBuffer + 4, details.receivedCommandDataLength);

        return true;
    }

    bool IVCAMHardwareIO::GetMEMStemp(float & MEMStemp)
    {
         IVCAMCommand command(IVCAMMonitorCommand::GetMEMSTemp);
         command.Param1 = 0;
         command.Param2 = 0;
         command.Param3 = 0;
         command.Param4 = 0;
         command.sizeOfSendCommandData = 0;
         command.TimeOut = 5000;
         command.oneDirection = false;

         bool result = PerfomAndSendHWmonitorCommand(command);
         if (result)
         {
            int32_t t = *((int32_t*)(command.receivedCommandData));
            MEMStemp = (float) t;
            MEMStemp /= 100;
            return true;
         }
		 else return false;
    }

    bool IVCAMHardwareIO::GetIRtemp(int & IRtemp)
    {
        IVCAMCommand command(IVCAMMonitorCommand::GetIRTemp);
        command.Param1 = 0;
        command.Param2 = 0;
        command.Param3 = 0;
        command.Param4 = 0;
        command.sizeOfSendCommandData = 0;
        command.TimeOut = 5000;
        command.oneDirection = false;

        if (PerfomAndSendHWmonitorCommand(command))
        {
            IRtemp = (int8_t) command.receivedCommandData[0];
            return true;
        }
		else return false;
    }

    bool IVCAMHardwareIO::UpdateASICCoefs(IVCAMASICCoefficients * coeffs)
    {
         IVCAMCommand command(IVCAMMonitorCommand::UpdateCalib);

         memcpy(command.data, coeffs->CoefValueArray, NUM_OF_CALIBRATION_COEFFS * sizeof(float));
         command.Param1 = 0;
         command.Param2 = 0;
         command.Param3 = 0;
         command.Param4 = 0;
         command.oneDirection = false;
         command.sizeOfSendCommandData = NUM_OF_CALIBRATION_COEFFS * sizeof(float);
         command.TimeOut = 5000;

         return PerfomAndSendHWmonitorCommand(command);
    }

    void IVCAMHardwareIO::GenerateAsicCalibrationCoefficients(std::vector<int> resolution, const bool isZMode, float * values) const
    {
        auto params = this->parameters;

        //handle vertical crop at 360p - change to full 640x480 and crop at the end
        bool isQres = resolution[0] == 640 && resolution[1] == 360;

        if (isQres)
        {
            resolution[1] = 480;
        }

        //generate coefficients
        int scale = 5;

        float width = (float) resolution[0] * scale;
        float height = (float) resolution[1];

        int PrecisionBits = 16;
        int CodeBits = 14;
        int TexturePrecisionBits = 12;
        float ypscale = (float)(1<<(CodeBits+1-10));
        float ypoff = 0;

        float s1 = (float)(1<<(PrecisionBits)) / 2047; // (range max)
        float s2 = (float)(1<<(CodeBits))-ypscale*0.5f;

        float alpha = 2/(width*params.Kc[0][0]);
        float beta  = -(params.Kc[0][2]+1)/params.Kc[0][0];
        float gamma = 2/(height*params.Kc[1][1]);
        float delta = -(params.Kc[1][2]+1)/params.Kc[1][1];

        float a  = alpha/gamma;
        float a1 = 1;
        float b  = 0.5f*scale*a  + beta/gamma;
        float c  = 0.5f*a1 + delta/gamma;

        float d0 = 1;                 
        float d1 = params.Invdistc[0]*pow(gamma,(float)2.0);  
        float d2 = params.Invdistc[1]*pow(gamma,(float)4.0);  
        float d5 = (float)( (double)(params.Invdistc[4]) * pow((double)gamma,6.0) );  
        float d3 = params.Invdistc[2]*gamma;
        float d4 = params.Invdistc[3]*gamma;

        float q  = 1/pow(gamma,(float)2.0);
        float p1 = params.Pp[2][3]*s1;
        float p2 = -s1*s2*(params.Pp[1][3]+params.Pp[2][3]);

        if (isZMode)
        {
            p1 = p1*sqrt(q);
            p2 = p2*sqrt(q);
        }

        float p3 = -params.Pp[2][0];
        float p4 = -params.Pp[2][1];
        float p5 = -params.Pp[2][2]/gamma;
        float p6 = s2*(params.Pp[1][0]+params.Pp[2][0]);
        float p7 = s2*(params.Pp[1][1]+params.Pp[2][1]);
        float p8 = s2*(params.Pp[1][2]+params.Pp[2][2])/gamma;

        //Reprojection parameters
        float sreproj = 2;
        float ax = -(1+params.Kp[0][2])/params.Kp[0][0];
        float ay = -(1+params.Kp[1][2])/params.Kp[1][1];

        float f0 = (params.Pp[0][1]+params.Pp[2][1]) / (params.Pp[0][0]+params.Pp[2][0]) / params.Kp[0][0]; 
        float f1 = (params.Pp[0][2]+params.Pp[2][2]) / (params.Pp[0][0]+params.Pp[2][0]) / gamma / params.Kp[0][0];
        float f2 = 0; //(Pp(2,1)+Pp(3,1)) / (Pp(1,1)+Pp(3,1)) / Kp(5);
        float f3 = 0; //(Pp(2,2)+Pp(3,2)) / (Pp(1,1)+Pp(3,1)) / Kp(5);
        float f4 = 0; //(Pp(2,3)+Pp(3,3)) / (Pp(1,1)+Pp(3,1)) / gamma / Kp(5);
        float f5 = 2*params.Pp[2][0] / (params.Pp[0][0]+params.Pp[2][0]) / sreproj;
        float f6 = 2*params.Pp[2][1] / (params.Pp[0][0]+params.Pp[2][0]) / sreproj;
        float f7 = 2*params.Pp[2][2] / (params.Pp[0][0]+params.Pp[2][0]) / sreproj / gamma;
        float f8 = (float) ( (double)(params.Pp[0][3] + params.Pp[2][3]) / (params.Pp[0][0]+params.Pp[2][0]) * s1 / params.Kp[0][0] );
        float f9 = (params.Pp[1][3] + params.Pp[2][3]) / (params.Pp[0][0]+params.Pp[2][0]) * s1 / params.Kp[1][1];
        float f10 = 2*params.Pp[2][3] / (params.Pp[0][0]+params.Pp[2][0]) * s1 / sreproj;
        if (isZMode)
        {
            f8  = f8  * sqrt(q);
            f9  = 0; //f9  * sqrt(q);
            f10 = f10 * sqrt(q);
        }
        float f11 = 1 / params.Kp[0][0];

        // Fix projection center 
        f11 = f11 + ax*f5;
        f0  = f0  + ax*f6;
        f1  = f1  + ax*f7;
        f8  = f8  + ax*f10;
        f2  = f2  + ay*f5;
        f3  = f3  + ay*f6;
        f4  = f4  + ay*f7;
        f9  = f9  + ay*f10;

        // Texture coeffs
        float suv = (float) ( (1<<TexturePrecisionBits)-1 );

        float h0 =  (params.Pt[0][1] +  params.Pt[2][1]) / (params.Pt[0][0]+params.Pt[2][0]); 
        float h1 =  (params.Pt[0][2] +  params.Pt[2][2]) / (params.Pt[0][0]+params.Pt[2][0]) / gamma;
        float h2 =  (params.Pt[1][0] +  params.Pt[2][0]) / (params.Pt[0][0]+params.Pt[2][0]);
        float h3 =  (params.Pt[1][1] +  params.Pt[2][1]) / (params.Pt[0][0]+params.Pt[2][0]);
        float h4 =  (params.Pt[1][2] +  params.Pt[2][2]) / (params.Pt[0][0]+params.Pt[2][0]) / gamma;
        float h5 = 2*params.Pt[2][0] / (params.Pt[0][0]  +  params.Pt[2][0]) / suv;
        float h6 = 2*params.Pt[2][1] / (params.Pt[0][0]  +  params.Pt[2][0]) / suv;
        float h7 = 2*params.Pt[2][2] / (params.Pt[0][0]  +  params.Pt[2][0]) / suv / gamma;
        float h8 =  (params.Pt[0][3] +  params.Pt[2][3]) / (params.Pt[0][0]+params.Pt[2][0]) * s1;
        float h9 =  (params.Pt[1][3] +  params.Pt[2][3]) / (params.Pt[0][0]+params.Pt[2][0]) * s1;
        float h10 = 2*params.Pt[2][3] / (params.Pt[0][0]+params.Pt[2][0]) * s1 / suv;
        float h11 = 1;

        if (isZMode)
        {
            h8  = h8  * sqrt(q);
            h9  = h9  * sqrt(q);
            h10 = h10 * sqrt(q);
        }

        float o1 =  (1+params.Kp[0][2])/params.Kp[0][0];
        float o2 = -(1+params.Kp[1][2])/params.Kp[1][1];
        float o3 = 1/s2/params.Kp[1][1];
        float o4 = 0; //s2*(1+Kp(8));

        float dp1 = params.Distp[0];
        float dp2 = params.Distp[1];
        float dp3 = params.Distp[2];
        float dp4 = params.Distp[3];
        float dp5 = params.Distp[4];

        float ip0 = params.Kp[1][1]*s2;
        float ip1 = (s2*params.Invdistp[0]*params.Kp[1][1]) + s2*(1+params.Kp[1][2]);
        float ip2 = (s2*params.Invdistp[1]*params.Kp[1][1]);
        float ip3 = (s2*params.Invdistp[2]*params.Kp[1][1]);
        float ip4 = (s2*params.Invdistp[3]*params.Kp[1][1]);
        float ip5 = (s2*params.Invdistp[4]*params.Kp[1][1]);

        if (isQres)
            c *= 0.75;

        //to simplify the ordering of the coefficients, initialize it in list syntax and copy to allocated memory.
        float coeffs [NUM_OF_CALIBRATION_COEFFS] = {1.0f,3.0f,a,a1,b,c,d0,d1,d2,
            d3,d4,d5,q,p1,p2,p3,p4,p5,p6,p7,p8,h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11,f0,f1,
            f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,o1,o2,o3,o4,dp1,dp2,dp3,dp4,dp5,ip0,ip1,ip2,ip3,
            ip4,ip5,ypscale,ypoff,0,0};

        memcpy(values, coeffs, NUM_OF_CALIBRATION_COEFFS  * sizeof(float));
        return ;
    }
    
    void IVCAMHardwareIO::StartTempCompensationLoop()
    {
		runTemperatureThread = true;
        temperatureThread = std::thread(&IVCAMHardwareIO::TemperatureControlLoop, this);
    }

    void IVCAMHardwareIO::StopTempCompensationLoop()
    {
        runTemperatureThread = false;
        temperatureCv.notify_one();
        if (temperatureThread.joinable())
            temperatureThread.join();
    }

    void IVCAMHardwareIO::TemperatureControlLoop()
    {
        IVCAMTemperatureData t;
        IVCAMASICCoefficients coeffs;

        std::unique_lock<std::mutex> lock(temperatureMutex);
        while (runTemperatureThread) 
        {
            //@tofix, this will throw if bad, but might periodically fail anyway. try/catch
            try
            {
                ReadTemperatures(t);

                if (calibration->updateParamsAccordingToTemperature(t.LiguriaTemp, t.IRTemp))
                {
                    //@tofix, qRes mode
                    std::cout << "[Debug] updating asic with new temperature calibration coefficients" << std::endl;
                    GenerateAsicCalibrationCoefficients({640, 480}, true, coeffs.CoefValueArray);
                    if (UpdateASICCoefs(&coeffs) != true)
                    {
                        continue; // try again if we couldn't update the coefficients
                    }
                }
            }
            catch(...) { std::cout << "[Debug] temperature read failed" << std::endl; }
			
            temperatureCv.wait_for(lock, std::chrono::seconds(10));
        }
    }

    IVCAMHardwareIO::IVCAMHardwareIO(uvc::device device, bool sr300) : device(device)
    {
        const uvc::guid IVCAM_WIN_USB_DEVICE_GUID = {0x175695CD, 0x30D9, 0x4F87, {0x8B, 0xE3, 0x5A, 0x82, 0x70, 0xF4, 0x9A, 0x31}};
        device.claim_interface(IVCAM_WIN_USB_DEVICE_GUID, IVCAM_MONITOR_INTERFACE);

        // Grab binary blob from the camera - SR300 - firmware gets the data from different stores
        uint8_t rawCalibrationBuffer[HW_MONITOR_BUFFER_SIZE];
        size_t bufferLength = HW_MONITOR_BUFFER_SIZE;		
        GetCalibrationRawData(sr300 ? IVCAMDataSource::TakeFromRAM : IVCAMDataSource::TakeFromRW, rawCalibrationBuffer, bufferLength);

        calibration.reset(new IVCAMCalibrator);

		CameraCalibrationParameters calibratedParameters;
		if (sr300)
		{
			SR300RawCalibration rawCalib;
			memcpy(&rawCalib, rawCalibrationBuffer, bufferLength);
			calibratedParameters = rawCalib.CalibrationParameters;
		}
        ProjectionCalibrate(rawCalibrationBuffer, (int) bufferLength, &calibratedParameters);

        parameters = calibratedParameters;

		StartTempCompensationLoop();
    }

	IVCAMHardwareIO::~IVCAMHardwareIO()
	{
		StopTempCompensationLoop();
	}

    ////////////////////////////////////
    // IVCAMCalibrator Implementation //
    ////////////////////////////////////

    bool IVCAMCalibrator::updateParamsAccordingToTemperature(float liguriaTemp, float IRTemp)
    {
        if (!thermalModeData.ThermalLoopParams.IRThermalLoopEnable)
            return false;

        double IrBaseTemperature = thermalModeData.BaseTemperatureData.IRTemp; //should be taken from the parameters
        double liguriaBaseTemperature = thermalModeData.BaseTemperatureData.LiguriaTemp; //should be taken from the parameters

        // calculate deltas from the calibration and last fix
        double IrTempDelta = IRTemp - IrBaseTemperature;
        double liguriaTempDelta = liguriaTemp - liguriaBaseTemperature;
        double weightedTempDelta = liguriaTempDelta * thermalModeData.ThermalLoopParams.LiguriaTempWeight + IrTempDelta * thermalModeData.ThermalLoopParams.IrTempWeight;
        double tempDetaFromLastFix = abs(weightedTempDelta - lastTemperatureDelta);

        //read intrinsic from the calibration working point
        double Kc11 = originalParams.Kc[0][0];
        double Kc13 = originalParams.Kc[0][2];

        // Apply model
        if (tempDetaFromLastFix >= thermalModeData.TempThreshold)
        {
            //if we are during a transition, fix for after the transition
            double tempDeltaToUse = weightedTempDelta;
            if (tempDeltaToUse > 0 && tempDeltaToUse < thermalModeData.ThermalLoopParams.TransitionTemp)
            {
                tempDeltaToUse =  thermalModeData.ThermalLoopParams.TransitionTemp;
            }

            //calculate fixed values
            double fixed_Kc11 = Kc11 + (thermalModeData.FcxSlope * tempDeltaToUse) + thermalModeData.FcxOffset;
            double fixed_Kc13 = Kc13 + (thermalModeData.UxSlope * tempDeltaToUse) + thermalModeData.UxOffset;

            //write back to intrinsic hfov and vfov
            params.Kc[0][0] = (float) fixed_Kc11;
            params.Kc[1][1] = originalParams.Kc[1][1] * (float)(fixed_Kc11/Kc11);
            params.Kc[0][2] = (float) fixed_Kc13;
            lastTemperatureDelta = weightedTempDelta;
            return true;
        }
        return false;
    }

    bool IVCAMCalibrator::buildParameters(const CameraCalibrationParameters & _params)
    {
        params = _params;
        isInitialized = true;
        lastTemperatureDelta = DELTA_INF;
        std::memcpy(&originalParams, &params, sizeof(CameraCalibrationParameters));
        return true;
    }

    bool IVCAMCalibrator::buildParameters(const float* paramData, int nParams)
    {
        memcpy(&params, paramData + 1, sizeof(CameraCalibrationParameters)); // skip the first float or 2 uint16
        isInitialized = true;
        lastTemperatureDelta = DELTA_INF;
        memcpy(&originalParams, &params, sizeof(CameraCalibrationParameters));
        return true;
    }

} } // namespace rsimpl::f200
