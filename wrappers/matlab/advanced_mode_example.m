function advanced_mode_example()
    % Make a context object
    ctx = realsense.context();
    % Get a device from context
    devs = ctx.query_devices();
    dev = devs{1};

    % Make sure the device supports advanced mode
    if ~dev.is('advanced_mode')
        error('Device doesn''t support advanced mode!');
    end
    
    % Make sure the device is in advanced mode
    adv = dev.as('advanced_mode');
    if ~adv.is_enabled()
        adv.toggle_advanced_mode(true);
        
        % Pause execution to give the device time to reconnect
        oldState = pause('on');
        pause(2)
        pause(oldState);

        % Grab the device
        devs = ctx.query_devices();
        dev = devs{1};
        adv = dev.as('advanced_mode');

        % Make sure the device supports advanced mode
        if ~adv.is_enabled()
            error('Device didn''t enter advanced mode');
        end
    end
    
    json_string = adv.serialize_json();
    json = jsondecode(json_string);

    % Modify json, jumping through a few hoops because of 
    % the interaction between how the json is defined and matlab
    json.controls_color_gain = string(str2double(json.controls_color_gain) - 4);
    json_string = jsonencode(json);
    % matlab replaces - with _ to create valid structure field names, we now need to switch them back
    json_string = strrep(json_string, '_', '-');

    % Upload new json
    adv.load_json(json_string);
end