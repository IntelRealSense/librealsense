using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using UnityEditor;
using Intel.RealSense;
using System;

public class rsEditor : MonoBehaviour {

	// Use this for initialization
	void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}

/*
public class RealSenseDeviceInspectorEditor : Editor
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
    public override void OnInspectorGUI()
    {
        RealSenseDeviceInspector deviceInspector = (RealSenseDeviceInspector)target;
        if (!deviceInspector.streaming)
            return;

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
            EditorGUILayout.LabelField(sensorName);

            foreach (var opt in sensor.Options)
            {
                if (opt.IsCheckbox())
                {
                    bool isChecked = Convert.ToBoolean(opt.Value);
                    if (isChecked != EditorGUILayout.Toggle(opt.Key.ToString(), isChecked))
                    {
                        opt.Value = Convert.ToSingle(!isChecked);
                    }
                }
                else if (!opt.IsEnum(sensor.Options))
                {
                    if (opt.ReadOnly)
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
                        if (Math.Abs(i - opt.Value) < 0.001)
                            selected = counter;

                        string label = sensor.Options.OptionValueDescription(opt.Key, i);
                        valuesStrings.Add(label);
                    }
                    var newSelection = EditorGUILayout.Popup(opt.Key.ToString(), selected, valuesStrings.ToArray());
                    if (newSelection != selected)
                    {
                        opt.Value = Convert.ToSingle(newSelection);
                    }
                }

            }
        }
    }
}
*/