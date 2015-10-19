import rs;

def main():
    
    ctx = rs.Context()
    count = ctx.get_device_count()
    print(count, "device(s) detected")

    for i in range(0, count):
        dev = ctx.get_device(i)
        print("Device", i, "is an", dev.get_name())

        for stream in range(0, rs.STREAM_COUNT):
            mode_count = dev.get_stream_mode_count(stream)
            print("Stream", rs.stream_to_string(stream), "has", mode_count, "modes")

            for mode in range(0, mode_count):
                (w, h, pf, fps) = dev.get_stream_mode(stream, mode)
                print(w, "x", h, rs.format_to_string(pf), "@", fps, "Hz")

            if mode_count > 0:
                dev.enable_stream(stream, rs.PRESET_BEST_QUALITY)
                intrin = dev.get_stream_intrinsics(stream)
                format = dev.get_stream_format(stream)
                fps = dev.get_stream_framerate(stream)
                print("Selected ", intrin.width, "x", intrin.height, rs.format_to_string(format), "@", fps, "Hz")

        extrin = dev.get_extrinsics(rs.STREAM_DEPTH, rs.STREAM_COLOR);
        print("Depth to color rotation = ", [extrin.rotation[i] for i in range(0,9)])
        print("Depth to color translation = ", [extrin.translation[i] for i in range(0,3)])

        print("Starting device...");
        dev.start()
        print("Device started");
        
        for j in range(0, 10):
            dev.wait_for_frames()
            for stream in range(0, rs.STREAM_COUNT):
                if(dev.is_stream_enabled(stream)):
                    print("Got", rs.stream_to_string(stream), "frame at time", dev.get_frame_timestamp(stream))

        print("Stopping device...")
        dev.stop()
        print("Device stopped")

    ctx.free()

main()
