using Intel.RealSense;
using UnityEditor;
using UnityEngine;

[CustomEditor(typeof(RsDevice))]
public class RsDeviceEditor : Editor
{
    private SerializedProperty config;
    private SerializedProperty mode;

    /// <summary>
    /// This function is called when the object becomes enabled and active.
    /// </summary>
    void OnEnable()
    {
        config = serializedObject.FindProperty("DeviceConfiguration");
        mode = config.FindPropertyRelative("mode");
    }
    // string[] names = { };
    // string[] serials;
    // int selectedSerial = 0;


    // private void PopuplateDevices()
    // {
    //     using (var ctx = new Context())
    //     using (var device_list = ctx.QueryDevices())
    //     {
    //         serials = device_list.Select(d => d.Info[CameraInfo.SerialNumber]).ToArray();
    //         var _names = device_list.Select(d => d.Info[CameraInfo.Name]);
    //         var m = _names.Max(d => d.Length);
    //         names = _names.Select((n, i) => string.Format("{0}    SN: {1}", n.PadRight(m), serials[i])).ToArray();
    //     }
    // }

    public override void OnInspectorGUI()
    {
        serializedObject.Update();

        // EditorGUILayout.PropertyField(serializedObject.FindProperty("m_Script"), true);

        var device = target as RsDevice;
        // bool isStreaming = device.ActiveProfile != null;
        bool isStreaming = device.isActiveAndEnabled && device.ActiveProfile != null;

        EditorGUI.BeginChangeCheck();

        EditorGUILayout.Space();
        EditorGUI.BeginDisabledGroup(isStreaming);
        mode.enumValueIndex = GUILayout.Toolbar(mode.enumValueIndex, mode.enumDisplayNames);

        EditorGUILayout.Space();
        EditorGUILayout.PropertyField(serializedObject.FindProperty("processMode"));
        EditorGUILayout.Space();
        EditorGUI.EndDisabledGroup();


        // EditorGUILayout.PropertyField(serializedObject.FindProperty("DeviceConfiguration"), true);

        switch ((RsConfiguration.Mode)mode.enumValueIndex)
        {
            case RsConfiguration.Mode.Live:
                // EditorGUILayout.BeginHorizontal();
                // var sn = config.FindPropertyRelative("RequestedSerialNumber");
                // selectedSerial = EditorGUILayout.Popup(selectedSerial, names);
                // if (serials != null && serials.Length != 0)
                //     sn.stringValue = serials[selectedSerial];

                // if (GUILayout.Button("Update", EditorStyles.miniButton, GUILayout.ExpandWidth(false)))
                // {
                //     PopuplateDevices();
                // }
                // EditorGUILayout.EndHorizontal();

                EditorGUI.BeginDisabledGroup(isStreaming);
                EditorGUILayout.PropertyField(config.FindPropertyRelative("RequestedSerialNumber"));

                EditorGUILayout.Space();
                EditorGUILayout.PropertyField(config.FindPropertyRelative("Profiles"), true);
                // EditorGUILayout.Space();
                // EditorGUILayout.PropertyField(serializedObject.FindProperty("_processingPipe"), false);
                EditorGUILayout.Space();
                EditorGUI.EndDisabledGroup();
                break;

            case RsConfiguration.Mode.Playback:
                {
                    EditorGUI.BeginDisabledGroup(isStreaming);
                    EditorGUILayout.BeginHorizontal();
                    var prop = config.FindPropertyRelative("PlaybackFile");
                    EditorGUILayout.PropertyField(prop);
                    if (GUILayout.Button("Open", EditorStyles.miniButton, GUILayout.ExpandWidth(false)))
                    {
                        var path = EditorUtility.OpenFilePanel("Recorded sequence", "", "bag");
                        if (path.Length != 0)
                        {
                            prop.stringValue = path;
                        }
                    }
                    EditorGUILayout.EndHorizontal();
                    // EditorGUILayout.Space();
                    // EditorGUILayout.PropertyField(serializedObject.FindProperty("_processingPipe"), false);
                    EditorGUILayout.Space();
                    EditorGUI.EndDisabledGroup();

                    if (isStreaming)
                    {
                        using (var playback = PlaybackDevice.FromDevice(device.ActiveProfile.Device))
                        {
                            bool isPlaying = playback.Status == PlaybackStatus.Playing;

                            //TODO: cache icons (in OnEnable)
                            var playBtnStyle = EditorGUIUtility.IconContent("PlayButton", "|Play");
                            // var playBtnStyle = EditorGUIUtility.IconContent("Animation.Play");

                            var pauseBtnStyle = EditorGUIUtility.IconContent("PauseButton", "|Pause");

                            // var stepBtnStyle = EditorGUIUtility.IconContent("StepButton", "|Step");

                            // var rewindBtnStyle = EditorGUIUtility.IconContent("Animation.PrevKey");
                            var rewindBtnStyle = EditorGUIUtility.IconContent("animation.firstkey.png");

                            GUILayout.BeginHorizontal();

                            if (GUILayout.Button(rewindBtnStyle, "CommandLeft"))
                                playback.Position = 0;

                            if (GUILayout.Button(isPlaying ? pauseBtnStyle : playBtnStyle, "CommandRight"))
                            {
                                if (isPlaying)
                                    playback.Pause();
                                else
                                    playback.Resume();
                            }

                            // bool play = GUILayout.Toggle(isPlaying, playBtnStyle, "CommandMid");
                            // if (play && !isPlaying)
                            //     playback.Resume();

                            // bool pause = GUILayout.Toggle(!isPlaying, pauseBtnStyle, "CommandRight");
                            // if (pause && isPlaying)
                            //     playback.Pause();

                            // if (GUILayout.Button(stepBtnStyle, "CommandRight"))
                            // {
                            // }

                            //TODO: no getter...
                            // playback.Speed = EditorGUILayout.FloatField(playback.Speed);

                            GUILayout.EndHorizontal();

                            if (!isPlaying)
                            {
                                // var t = TimeSpan.FromMilliseconds(playback.Position * 1e-6);
                                // playback.Position = (ulong)EditorGUILayout.Slider(t.ToString(), playback.Position, 0, playback.Duration);
                                playback.Position = (ulong)EditorGUILayout.Slider(playback.Position, 0, playback.Duration);
                            }


                            EditorGUI.BeginDisabledGroup(true);
                            EditorGUILayout.Space();
                            EditorGUILayout.PropertyField(config.FindPropertyRelative("Profiles"), true);
                            EditorGUI.EndDisabledGroup();
                        }
                    }
                }
                break;

            case RsConfiguration.Mode.Record:
                {
                    EditorGUILayout.BeginHorizontal();
                    var prop = config.FindPropertyRelative("RecordPath");
                    EditorGUILayout.PropertyField(prop);
                    if (GUILayout.Button("Choose", EditorStyles.miniButton, GUILayout.ExpandWidth(false)))
                    {
                        var path = EditorUtility.SaveFilePanel("Recorded sequence", "", System.DateTime.Now.ToString("yyyyMMdd_hhmmss"), "bag");
                        if (path.Length != 0)
                        {
                            prop.stringValue = path;
                        }
                    }
                    EditorGUILayout.EndHorizontal();

                    EditorGUILayout.Space();
                    EditorGUILayout.PropertyField(config.FindPropertyRelative("Profiles"), true);
                    // EditorGUILayout.Space();
                    // EditorGUILayout.PropertyField(serializedObject.FindProperty("_processingPipe"), false);
                    EditorGUILayout.Space();
                    EditorGUI.EndDisabledGroup();
                }
                break;

        }


        serializedObject.ApplyModifiedProperties();

        EditorGUI.EndChangeCheck();
    }

}
