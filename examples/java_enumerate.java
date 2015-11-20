/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

import com.intel.rs.*;

public class java_enumerate
{
	public static void main(String[] args)
	{
		Context ctx = new Context();
		int count = ctx.getDeviceCount();
		System.out.println(count + " device(s) detected");

		for(int i=0; i<count; i++)
		{
			Device dev = ctx.getDevice(i);
			System.out.println("Device " + i + " is an " + dev.getName());

			for(Stream s : Stream.values())
			{
				int modeCount = dev.getStreamModeCount(s);
				System.out.println("Stream " + s + " has " + modeCount + " modes");

				for(int k=0; k<modeCount; k++)
				{
					Out<Integer> width = new Out<Integer>(), height = new Out<Integer>(), fps = new Out<Integer>();
					Out<Format> format = new Out<Format>();
					dev.getStreamMode(s, k, width, height, format, fps);
					System.out.println(width.value + "x" + height.value + " " + format.value + " @ " + fps.value + "Hz");
				}

				if(modeCount > 0)
				{
					dev.enableStreamPreset(s, Preset.BEST_QUALITY);
					
					Intrinsics intrin = new Intrinsics();
					dev.getStreamIntrinsics(s, intrin);
					System.out.println("Selected " + intrin.width + "x" + intrin.height + " " + intrin.model);
					System.out.println(intrin.coeffs[0] + ", " + intrin.coeffs[1] + ", " + intrin.coeffs[2] + ", " + intrin.coeffs[3] + ", " + intrin.coeffs[4]);
				}
			}

			System.out.println("Starting device...");
			dev.start();
			System.out.println("Device started");

			for(int j=0; j<10; ++j)
			{
				dev.waitForFrames();
				System.out.println("Got depth frame at time " + dev.getFrameTimestamp(Stream.DEPTH));
			}

			System.out.println("Stopping device...");
			dev.stop();
			System.out.println("Device stopped");
		}
		
		ctx.close();
	}
}
