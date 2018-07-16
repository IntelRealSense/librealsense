using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class GetMaterialInt : MonoBehaviour
{

    [System.Serializable]
    public class IntEvent : UnityEvent<int> { }

    public Material material;

    public string propertyName;
    public IntEvent Binding;

    void Awake()
    {
        Binding.Invoke(material.GetInt(propertyName));
    }
}
