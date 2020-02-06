package com.intel.realsense.camera;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Switch;
import android.widget.Toast;

import com.intel.realsense.librealsense.DebugProtocol;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.RsContext;

import java.io.File;

public class TerminalActivity extends AppCompatActivity {

    private static final String TAG = "librs camera terminal";
    private static final int OPEN_FILE_REQUEST_CODE = 0;

    private Switch mAutoComplete;
    private Switch mStreaming;

    private Button mSendButton;
    private Button mClearButton;
    private AutoCompleteTextView mInputText;
    private EditText mOutputText;
    private String mFilePath;

    private Thread mStreamingThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_terminal);

        mSendButton = findViewById(R.id.terminal_send_button);
        mSendButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                send();
            }
        });

        mClearButton = findViewById(R.id.terminal_clear_button);
        mClearButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mInputText.setText("");
            }
        });
        mInputText = findViewById(R.id.terminal_input_edit_text);
        mInputText.setOnKeyListener(new View.OnKeyListener() {
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                if ((event.getAction() == KeyEvent.ACTION_DOWN) &&
                        (keyCode == KeyEvent.KEYCODE_ENTER)) {
                    send();
                    return true;
                }
                return false;
            }
        });
        mOutputText = findViewById(R.id.terminal_output_edit_text);
        mAutoComplete = findViewById(R.id.terminal_input_auto_complete);
        mAutoComplete.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putBoolean(getString(R.string.terminal_auto_complete), isChecked);
                editor.commit();
                setupAutoCompleteTextView();
            }
        });

        mStreaming = findViewById(R.id.terminal_stream);
        mStreaming.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked)
                    startStreaming();
                else
                    stopStreaming();
            }
        });
        init();
    }

    private void stopStreaming() {
        mStreamingThread.interrupt();
        try {
            mStreamingThread.join(1000);
        } catch (InterruptedException e) {
            Log.e(TAG, "stopStreaming, error: " + e.getMessage());
        }
    }

    @Override
    protected void onPause() {
        super.onPause();

        if(mStreaming.isChecked()){
            stopStreaming();
        }
    }

    private void startStreaming() {
        mStreamingThread = new Thread(new Runnable() {
            @Override
            public void run() {
                try(Pipeline pipe = new Pipeline()){
                    pipe.start();
                    while(!mStreamingThread.isInterrupted()){
                        try(FrameSet frames = pipe.waitForFrames()){}
                    }
                    pipe.stop();
                } catch (Exception e) {
                    mOutputText.setText("streaming error: " + e.getMessage());
                }
            }
        });
        mStreamingThread.start();
    }

    private void send() {
        RsContext mRsContext = new RsContext();
        try(DeviceList devices = mRsContext.queryDevices()){
            if(devices.getDeviceCount() == 0)
            {
                Log.e(TAG, "getDeviceCount returned zero");
                Toast.makeText(this, "Connect a camera", Toast.LENGTH_LONG).show();
                return;
            }
            try(Device device = devices.createDevice(0)){
                try(DebugProtocol dp = device.as(Extension.DEBUG)){
                    String content = mInputText.getText().toString();
                    String res = dp.SendAndReceiveRawData(mFilePath, content);
                    mOutputText.setText("command: " + mInputText.getText() + "\n\n" + res);
                    mInputText.setText("");
                }
            }
            catch(Exception e){
                mOutputText.setText("Error: " + e.getMessage());
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    private void setupAutoCompleteTextView(){
        try{
            if(mAutoComplete.isChecked()) {
                String[] spArray = DebugProtocol.getCommands(mFilePath);
                ArrayAdapter<String> adapter = new ArrayAdapter<>(this,
                        android.R.layout.simple_dropdown_item_1line, spArray);
                mInputText.setAdapter(adapter);
            }else{
                mInputText.setAdapter(null);
            }
        }catch(Exception e){
            Log.e(TAG, e.getMessage());
            mOutputText.setText("Error: " + e.getMessage());
        }
    }

    private void init(){
        SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
        mFilePath = sharedPref.getString(getString(R.string.terminal_commands_file), "");
        mAutoComplete.setChecked(sharedPref.getBoolean(getString(R.string.terminal_auto_complete), false));

        if(mFilePath == null || !(new File(mFilePath).exists())){
            Intent intent = new Intent(this, FileBrowserActivity.class);
            intent.putExtra(getString(R.string.browse_folder), getString(R.string.realsense_folder) + File.separator + "hw");
            startActivityForResult(intent, OPEN_FILE_REQUEST_CODE);
        }
        else
            setupAutoCompleteTextView();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == OPEN_FILE_REQUEST_CODE && resultCode == RESULT_OK) {
            if (data != null) {
                mFilePath = data.getStringExtra(getString(R.string.intent_extra_file_path));
                SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putString(getString(R.string.terminal_commands_file), mFilePath);
                editor.commit();
                setupAutoCompleteTextView();
            }
        }
        else{
            Intent intent = new Intent();
            setResult(RESULT_OK, intent);
            finish();
        }
    }
}
