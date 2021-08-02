function [] = export_calibratrion(from_serial, to_serial, stereoParams, output_filename)
% Function converts calibration results into a calibration file for
% rs-pointcloud-stitching

% from_serial - the serial number of camera1
% to_serial   - the serial number of camera2
% stereoParam - as exported from stereoCameraCalibrator
% output_filename - where to save the calibration file for
% rs-pointcloud-stitching

    fout = fopen(output_filename, 'w');
    if (fout < 0)
        fprintf('Error openning file %s for writing.\n', output_filename);
        exit();
    end
    eul_1to2 = rotm2eul(stereoParams.RotationOfCamera2);
    fprintf('Found the following rotation angles: %f, %f, %f\n', eul_1to2 * 180/pi);
    rot_2_to_middle = inv(eul2rotm(eul_1to2 / 2));
    trans_2_to_middle = stereoParams.TranslationOfCamera2 * (-1) / 2;
    
    fprintf(fout, '%s, %s, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n', string(from_serial), string(to_serial), stereoParams.RotationOfCamera2', stereoParams.TranslationOfCamera2/1000.0);
    fprintf(fout, '%s, %s, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n', string(to_serial), 'virtual_dev', rot_2_to_middle', trans_2_to_middle/1000.0);
    fclose(fout);
end

