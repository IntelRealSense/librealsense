using UnityEngine;
using UnityEditor;
using UnityEditorInternal;
using System;
using System.Linq;
using System.Collections.Generic;

[CustomEditor(typeof(RsProcessingProfile))]
public class RsProcessingPipeEditor : Editor
{
    private SerializedProperty _property;
    private ReorderableList _list;

    private void OnEnable()
    {
        _property = serializedObject.FindProperty("_processingBlocks");
        _list = new ReorderableList(serializedObject, _property, true, true, true, true)
        {
            drawHeaderCallback = DrawListHeader,
            drawElementCallback = DrawListElement,
            drawElementBackgroundCallback = DrawElementBackground,
            elementHeightCallback = GetElementHeightCallback,
            onAddDropdownCallback = AddDropdownCallback,
            onRemoveCallback = RemoveCallback,
#if UNITY_2018_1_OR_NEWER
            //onReorderCallbackWithDetails = ReorderCallbackWithDetails,
#endif
            onSelectCallback = SelectElement,
        };
    }

    private void SelectElement(ReorderableList list)
    {
        var element = list.serializedProperty.GetArrayElementAtIndex(list.index);
        EditorGUIUtility.PingObject(element.objectReferenceValue);
    }

    private void ReorderCallbackWithDetails(ReorderableList list, int oldIndex, int newIndex)
    {
        var e0 = list.serializedProperty.GetArrayElementAtIndex(oldIndex).isExpanded;
        var e = list.serializedProperty.GetArrayElementAtIndex(newIndex).isExpanded;
        list.serializedProperty.GetArrayElementAtIndex(newIndex).isExpanded = e0;
        list.serializedProperty.GetArrayElementAtIndex(oldIndex).isExpanded = e;
    }


    private void DrawElementBackground(Rect rect, int index, bool isActive, bool isFocused)
    {
        if (Event.current.type == EventType.Repaint)
        {
            rect.height = GetElementHeightCallback(index);
            // rect.height += 3f;
            ReorderableList.defaultBehaviours.DrawElementBackground(rect, index, isActive, isFocused, true);

            // Color color = (!EditorGUIUtility.isProSkin) ? new Color(0.6f, 0.6f, 0.6f, 0.333f) : new Color(0.12f, 0.12f, 0.12f, 0.333f);
            // if (index < _list.count - 1)
            //     EditorGUI.DrawRect(new Rect(rect.x, rect.y + rect.height - 1f, rect.width, 1f), color);
            // EditorGUI.DrawRect(new Rect(rect.x + 20f, rect.y, 1f, rect.height), color);

            // var s = new GUIStyle("IN Title");
            // s.Draw(new Rect(rect.x + 1f, rect.y, 19f, rect.height), false, false, true, false);

        }
    }

    private void RemoveCallback(ReorderableList list)
    {
        var element = list.serializedProperty.GetArrayElementAtIndex(list.index);

        ScriptableObject.DestroyImmediate(element.objectReferenceValue, true);
        AssetDatabase.SaveAssets();


        element.objectReferenceValue = null;
        list.serializedProperty.DeleteArrayElementAtIndex(list.index);
        if (list.index >= list.serializedProperty.arraySize - 1)
        {
            list.index = list.serializedProperty.arraySize - 1;
        }
    }

    private void AddDropdownCallback(Rect buttonRect, ReorderableList list)
    {
        var menu = new GenericMenu();

        // var blocks = AppDomain.CurrentDomain.GetAssemblies()
        //     .SelectMany(a => a.GetTypes())
        //     .Where(t => t.IsSubclassOf(typeof(Intel.RealSense.ProcessingBlock)))
        //     .ToArray();

        var blocks = AppDomain.CurrentDomain.GetAssemblies()
            .SelectMany(a => a.GetTypes())
            .Select(t => new
            {
                Type = t,
                Attribute = t.GetCustomAttributes(typeof(ProcessingBlockDataAttribute), false).FirstOrDefault() as ProcessingBlockDataAttribute
            }).Where(a => a.Attribute != null)
            ;

        foreach (var b in blocks)
        {
            var t = b.Attribute.blockClass.ToString();
            t = t.Replace('.', '/');
            menu.AddItem(new GUIContent(t), false, data =>
            {
                serializedObject.UpdateIfRequiredOrScript();

                var block = (data as object[])[0] as Type;
                var index = list.serializedProperty.arraySize;
                list.serializedProperty.arraySize++;
                list.index = index;
                var element = list.serializedProperty.GetArrayElementAtIndex(index);

                var obj = ScriptableObject.CreateInstance(block);
                // UnityEditor.ObjectNames.GetUniqueName(names.ToArray(), block.Name);
                obj.name = block.Name;
                AssetDatabase.AddObjectToAsset(obj, target);
                element.objectReferenceValue = obj;

                serializedObject.ApplyModifiedProperties();
                AssetDatabase.SaveAssets();

            }, new object[] { b.Type, b.Attribute });
        }

        menu.ShowAsContext();
    }

    private float GetElementHeightCallback(int index)
    {
        if (index < 0)
            return 0;

        var item = _property.GetArrayElementAtIndex(index);

        if (item == null || item.objectReferenceValue == null)
            return 0;

        if (!item.isExpanded)
            return EditorGUIUtility.singleLineHeight * 1.5f;

        var obj = new SerializedObject(item.objectReferenceValue);

        var h = EditorGUIUtility.singleLineHeight * 1.5f;

        SerializedProperty iterator = obj.GetIterator();
        bool enterChildren = true;
        while (iterator.NextVisible(enterChildren))
        {
            if (iterator.name == "m_Script")
                continue;
            if (iterator.name == "enabled")
                continue;
            h += EditorGUI.GetPropertyHeight(iterator, GUIContent.none, iterator.isExpanded);
            h += 4f;
            enterChildren = false;
        }

        iterator.Dispose();
        obj.Dispose();

        return h;
    }

    private void DrawListHeader(Rect rect)
    {
        GUI.Label(rect, "Processing Blocks");
    }

    private void DrawListElement(Rect rect, int index, bool isActive, bool isFocused)
    {
        var item = _property.GetArrayElementAtIndex(index);

        if (item.objectReferenceValue == null)
        {
            return;
        }

        var pb = item.objectReferenceValue;
        var obj = new SerializedObject(pb);
        obj.Update();

        var r = new Rect(rect.x, rect.y, rect.width, EditorGUIUtility.singleLineHeight);

        var enabled = obj.FindProperty("enabled");
        enabled.boolValue = EditorUtility.GetObjectEnabled(obj.targetObject) == 1;
        item.isExpanded = EditorGUI.InspectorTitlebar(r, item.isExpanded, obj.targetObject, true);

        if (!item.isExpanded)
        {
            obj.ApplyModifiedProperties();
            obj.Dispose();
            return;
        }

        r.y += EditorGUIUtility.singleLineHeight * 1.5f;

        SerializedProperty iterator = obj.GetIterator();
        bool enterChildren = true;
        while (iterator.NextVisible(enterChildren))
        {
            if (iterator.name == "m_Script")
                continue;

            if (iterator.name == "enabled")
                continue;

            r.height = EditorGUI.GetPropertyHeight(iterator, GUIContent.none, iterator.isExpanded);
            EditorGUI.PropertyField(r, iterator, true);
            enterChildren = false;
            r.y += r.height + 4f;
        }
        obj.ApplyModifiedProperties();
        obj.Dispose();
    }

    protected override bool ShouldHideOpenButton()
    {
        return true;
    }

    public override void OnInspectorGUI()
    {
        serializedObject.Update();
        EditorGUILayout.Space();
        using (new EditorGUI.DisabledScope(true))
            EditorGUILayout.PropertyField(serializedObject.FindProperty("m_Script"));
        EditorGUILayout.Space();
        _list.DoLayoutList();
        EditorGUILayout.Space();
        serializedObject.ApplyModifiedProperties();

        EditorUtility.SetDirty(target);
    }
}