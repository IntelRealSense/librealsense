using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[CreateAssetMenu]
public class RsProcessingProfile : ScriptableObject, IEnumerable<RsProcessingBlock>
{
    // [HideInInspector]
    [SerializeField]
    public List<RsProcessingBlock> _processingBlocks;

    public IEnumerator<RsProcessingBlock> GetEnumerator()
    {
        return _processingBlocks.GetEnumerator() as IEnumerator<RsProcessingBlock>;
    }

    IEnumerator IEnumerable.GetEnumerator()
    {
        return _processingBlocks.GetEnumerator();
    }


#if UNITY_EDITOR
    void Reset()
    {

        var obj = new UnityEditor.SerializedObject(this);
        obj.Update();

        var blocks = obj.FindProperty("_processingBlocks");
        blocks.ClearArray();

        var p = UnityEditor.AssetDatabase.GetAssetPath(this);
        var bl = UnityEditor.AssetDatabase.LoadAllAssetsAtPath(p);
        foreach (var a in bl)
        {
            if (a == this)
                continue;
            // Debug.Log(a, a);
            // DestroyImmediate(a, true);
            int i = blocks.arraySize++;
            var e = blocks.GetArrayElementAtIndex(i);
            e.objectReferenceValue = a;
        }

        obj.ApplyModifiedProperties();
        // UnityEditor.EditorUtility.SetDirty(this);
        UnityEditor.AssetDatabase.SaveAssets();

    }
#endif
}
