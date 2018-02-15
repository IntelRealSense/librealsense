using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using Intel.RealSense;
using System;

public class SensorOptions : MonoBehaviour
{
    void Start()
    {
        if (RealSenseDevice.Instance.ActiveProfile != null)
        {
            onStartStreaming(RealSenseDevice.Instance.ActiveProfile);
        }
        else
        {
            RealSenseDevice.Instance.OnStart += onStartStreaming;
        }
    }

    private void onStartStreaming(PipelineProfile profile)
    {
        sensors.Clear();
        foreach (var s in profile.Device.Sensors)
        {
            sensors.Add(s.Info[CameraInfo.Name], s);
        }
    }

    void Update()
    {
    }

    public Dictionary<string, Sensor> sensors = new Dictionary<string, Sensor>();
}

public static class extensions
{
    public static bool IsCheckbox(this Sensor.CameraOption opt)
    {
        return opt.Max == 1.0f &&
            opt.Min == 0.0f &&
            opt.Step == 1.0f;
    }

    public static bool IsEnum(this Sensor.CameraOption opt, Sensor.SensorOptions s)
    {
        if (opt.Step < 0.001f) return false;

        for (float i = opt.Min; i <= opt.Max; i += opt.Step)
        {
            if (string.IsNullOrEmpty(s.OptionValueDescription(opt.Key, i)))
                return false;
        }
        return true;
    }

    public static bool IsIntegersOnly(this Intel.RealSense.Sensor.CameraOption opt)
    {
        Func<float, bool> is_integer = (v) => { return v == Math.Floor(v); };
        return is_integer(opt.Min) && is_integer(opt.Max) &&
                is_integer(opt.Default) && is_integer(opt.Step);
    }
}
[CustomEditor(typeof(SensorOptions))]
public class SensorControllerEditor : Editor
{
    public override void OnInspectorGUI()
    {
        SensorOptions sensorController = (SensorOptions)target;
        foreach (var kvp in sensorController.sensors)
        {
            string sensorName = kvp.Key;
            var sensor = kvp.Value;
            EditorGUILayout.Space();
            EditorGUILayout.LabelField(sensorName);

            foreach (var opt in sensor.Options)
            {
                if(opt.IsCheckbox())
                {
                    bool isChecked = Convert.ToBoolean(opt.Value);
                    if(isChecked != EditorGUILayout.Toggle(opt.Key.ToString(), isChecked))
                    {
                        opt.Value = Convert.ToSingle(!isChecked);
                    }
                }
                else if(!opt.IsEnum(sensor.Options))
                {
                    if(opt.ReadOnly)
                    {
                        EditorGUILayout.BeginHorizontal();
                        {
                            EditorGUILayout.LabelField(opt.Key.ToString(), GUILayout.Width(EditorGUIUtility.labelWidth - 4));
                            EditorGUILayout.SelectableLabel(opt.Value.ToString(), EditorStyles.textField, GUILayout.Height(EditorGUIUtility.singleLineHeight));
                        }
                        EditorGUILayout.EndHorizontal();
                    }
                    else if (opt.IsIntegersOnly())
                    {
                        var newVal = EditorGUILayout.IntSlider(opt.Key.ToString(), Convert.ToInt32(opt.Value), Convert.ToInt32(opt.Min), Convert.ToInt32(opt.Max));
                        if (newVal != Convert.ToInt32(opt.Value))
                            opt.Value = Convert.ToSingle(newVal);
                    }
                    else
                    {
                        float newVal = EditorGUILayout.Slider(opt.Key.ToString(), opt.Value, opt.Min, opt.Max);
                        if (opt.Value != newVal)
                            opt.Value = newVal;
                    }
                }
                else
                {
                    List<string> valuesStrings = new List<string>();
                    int selected = 0;
                    int counter = 0;
                    for (float i = opt.Min; i <= opt.Max; i += opt.Step, counter++)
                    {
                        if(Math.Abs(i - opt.Value) < 0.001)
                            selected = counter;

                        string label = sensor.Options.OptionValueDescription(opt.Key, i);
                        valuesStrings.Add(label);
                    }
                    var newSelection = EditorGUILayout.Popup(opt.Key.ToString(), selected, valuesStrings.ToArray());
                    if(newSelection != selected)
                    {
                        opt.Value = Convert.ToSingle(newSelection);
                    }
                }
                
            }
        }
    }
}