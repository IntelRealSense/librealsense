using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using Intel.RealSense;
using System;
using System.Linq;


public static class CameraOptionExtensions
{
    public static bool IsCheckbox(this Sensor.CameraOption opt)
    {
        return opt.Max == 1.0f &&
        opt.Min == 0.0f &&
        opt.Step == 1.0f;
    }

    public static bool IsEnum(this Sensor.CameraOption opt, Sensor.SensorOptions s)
    {
        if (opt.Step < 0.001f)
            return false;

        for (float i = opt.Min; i <= opt.Max; i += opt.Step)
        {
            if (string.IsNullOrEmpty(s.OptionValueDescription(opt.Key, i)))
                return false;
        }
        return true;
    }

    public static bool IsIntegersOnly(this Sensor.CameraOption opt)
    {
        Func<float, bool> is_integer = (v) => v == Math.Floor(v);
        return is_integer(opt.Min) && is_integer(opt.Max) &&
        is_integer(opt.Default) && is_integer(opt.Step);
    }
}


[CustomEditor(typeof(RsDeviceInspector))]
public class RsDeviceInspectorEditor : Editor
{
    public static void DrawHorizontal(string field, string value)
    {
        EditorGUILayout.BeginHorizontal();
        {
            EditorGUILayout.LabelField(field, GUILayout.Width(EditorGUIUtility.labelWidth - 4));
            EditorGUILayout.SelectableLabel(value, EditorStyles.textField, GUILayout.Height(EditorGUIUtility.singleLineHeight));
        }
        EditorGUILayout.EndHorizontal();
    }

    protected override bool ShouldHideOpenButton()
    {
        return true;
    }

    public override void OnInspectorGUI()
    {
        var deviceInspector = target as RsDeviceInspector;
        if (!deviceInspector.streaming)
        {
            EditorGUILayout.HelpBox("Start streaming to see device settings", MessageType.Info);
            cachedValue.Clear();
            return;
        }

        EditorGUILayout.Space();
        var devName = deviceInspector.device.Info[CameraInfo.Name];
        var devSerial = deviceInspector.device.Info[CameraInfo.SerialNumber];
        DrawHorizontal("Device", devName);
        DrawHorizontal("Device S/N", devSerial);
        EditorGUILayout.Space();

        foreach (var kvp in deviceInspector.sensors)
        {
            string sensorName = kvp.Key;
            var sensor = kvp.Value;

            EditorGUILayout.Space();
            EditorGUILayout.LabelField(sensorName, EditorStyles.boldLabel);

            EditorGUI.indentLevel++;
            deviceInspector.sensorOptions[sensorName].ForEach(opt => DrawOption(sensor, opt));
            EditorGUI.indentLevel--;
        }
    }

    readonly Dictionary<Sensor.CameraOption, float> cachedValue = new Dictionary<Sensor.CameraOption, float>();

    void DrawOption(Sensor sensor, Sensor.CameraOption opt)
    {
        if (!cachedValue.ContainsKey(opt))
            cachedValue[opt] = opt.Value;

        string k = opt.Key.ToString();
        float v = cachedValue[opt];

        if (opt.ReadOnly)
        {
            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField(k, GUILayout.Width(EditorGUIUtility.labelWidth - 4));
            EditorGUILayout.SelectableLabel(v.ToString(), EditorStyles.textField, GUILayout.Height(EditorGUIUtility.singleLineHeight));
            EditorGUILayout.EndHorizontal();
        }
        else if (opt.IsCheckbox())
        {
            bool isChecked = Convert.ToBoolean(v);
            if (isChecked != EditorGUILayout.Toggle(k, isChecked))
            {
                cachedValue[opt] = opt.Value = Convert.ToSingle(!isChecked);
            }
        }
        else if (opt.IsEnum(sensor.Options))
        {
            var valuesStrings = new List<string>();
            int selected = 0;
            int counter = 0;
            for (float i = opt.Min; i <= opt.Max; i += opt.Step, counter++)
            {
                if (Math.Abs(i - v) < 0.001)
                    selected = counter;
                valuesStrings.Add(sensor.Options.OptionValueDescription(opt.Key, i));
            }
            var newSelection = EditorGUILayout.Popup(k, selected, valuesStrings.ToArray());
            if (newSelection != selected)
            {
                cachedValue[opt] = opt.Value = Convert.ToSingle(newSelection);
            }
        }
        else if (opt.IsIntegersOnly())
        {
            var newVal = EditorGUILayout.IntSlider(k, (int)v, (int)opt.Min, (int)opt.Max);
            if (newVal != Convert.ToInt32(v))
                cachedValue[opt] = opt.Value = Convert.ToSingle(newVal);
        }
        else
        {
            float s = EditorGUILayout.Slider(k, v, opt.Min, opt.Max);
            if (!Mathf.Approximately(s, v))
            {
                cachedValue[opt] = opt.Value = s;
            }
        }
    }
}
