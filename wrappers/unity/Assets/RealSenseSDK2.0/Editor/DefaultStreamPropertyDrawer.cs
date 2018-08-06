using System;
using System.Collections;
using System.Collections.Generic;
using Intel.RealSense;
using UnityEditor;
using UnityEngine;

[CustomEditor(typeof(RsStreamTextureRenderer), true)]
public class DefaultStreamPropertyDrawer : Editor
{
    bool hasDefault = false;

    private SerializedProperty stream;
    private SerializedProperty format;

    void OnEnable()
    {
        stream = serializedObject.FindProperty("sourceStreamType");
        format = serializedObject.FindProperty("textureFormat");
        hasDefault = false;

        var t = target.GetType();
        var a = t.GetCustomAttributes(typeof(DefaultStreamAttribute), false);
        if (a.Length > 0)
        {
            hasDefault = true;
            var d = a[0] as DefaultStreamAttribute;
            stream.enumValueIndex = (int)d.stream;
            format.intValue = (int)d.format;
            serializedObject.ApplyModifiedProperties();
        }
    }

    public override void OnInspectorGUI()
    {
        EditorGUI.BeginChangeCheck();
        serializedObject.Update();
        SerializedProperty iterator = serializedObject.GetIterator();
        bool enterChildren = true;
        while (iterator.NextVisible(enterChildren))
        {
            if (hasDefault)
            {
                if (iterator.propertyPath == stream.propertyPath || iterator.propertyPath == format.propertyPath)
                    continue;
            }
            using (new EditorGUI.DisabledScope("m_Script" == iterator.propertyPath))
            {
                EditorGUILayout.PropertyField(iterator, true, new GUILayoutOption[0]);
            }
            enterChildren = false;
        }
        serializedObject.ApplyModifiedProperties();
        EditorGUI.EndChangeCheck();

    }

}
