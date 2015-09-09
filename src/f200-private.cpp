#include "f200-private.h"

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

    void IVCAMHardwareIO::GetCalibrationRawData(uint8_t * data, size_t & bytesReturned)
    {
        uint8_t request[IVCAM_MONITOR_HEADER_SIZE];
        size_t requestSize = sizeof(request);
        uint32_t responseOp;

        if (PrepareUSBCommand(request, requestSize, (uint32_t) IVCAMMonitorCommand::GetCalibrationTable) <= 0) 
			throw std::runtime_error("usb transfer to retrieve calibration data failed");

        ExecuteUSBCommand(request, requestSize, responseOp, data, bytesReturned);
    }

    void IVCAMHardwareIO::ProjectionCalibrate(uint8_t * rawCalibData, int len, CameraCalibrationParameters * calprms)
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
    }

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

		auto result = PerfomAndSendHWmonitorCommand(cmd);
		return result;
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
		 }
		 return false;
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

        bool result = PerfomAndSendHWmonitorCommand(command);
        if (result)
		{
			IRtemp = (int8_t) command.receivedCommandData[0];
		}
        return false;
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

	
    bool IVCAMHardwareIO::StartTempCompensationLoop()
	{
		temperatureThread = std::thread(&IVCAMHardwareIO::TemperatureControlLoop, this);
		return true; // tofix
	}

    void IVCAMHardwareIO::StopTempCompensationLoop()
	{
		runTemperatureThread = false;
		if (temperatureThread.joinable())
			temperatureThread.join();
	}

    void IVCAMHardwareIO::TemperatureControlLoop()
    {
		IVCAMTemperatureData t;
		IVCAMASICCoefficients coeffs;

		while(runTemperatureThread) 
		{
			//@tofix, this will throw if bad, but might periodically fail anyway. try/catch
			ReadTemperatures(t);

			bool updateNeeded = false;
			{
				std::lock_guard<std::mutex> guard(temperatureMutex);
				updateNeeded = calibration->updateParamsAccordingToTemperature(t.LiguriaTemp, t.IRTemp);
			}

			if (updateNeeded)
			{
				// @todo, fix for qres -
				// calibration->generateCalibrationCoefficients({640, 480}, true, coeffs.CoefValueArray);
				if (UpdateASICCoefs(&coeffs) != true)
				{
					if (UpdateASICCoefs(&coeffs) != true) 
					{
						continue;
					}
				}
			}

			// ... is this correct? at the very least, this will block and prevent us from exiting cleanly
			std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		}
    }

    IVCAMHardwareIO::IVCAMHardwareIO(uvc::device device) : device(device)
    {
        device.claim_interface(IVCAM_MONITOR_INTERFACE);

        uint8_t rawCalibrationBuffer[HW_MONITOR_BUFFER_SIZE];
        size_t bufferLength = HW_MONITOR_BUFFER_SIZE;
        GetCalibrationRawData(rawCalibrationBuffer, bufferLength);

        calibration.reset(new IVCAMCalibrator);

        CameraCalibrationParameters calibratedParameters;
        ProjectionCalibrate(rawCalibrationBuffer, (int) bufferLength, &calibratedParameters);

        parameters = calibratedParameters;
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
