using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class GetMaterialFloat : MonoBehaviour
{

    [System.Serializable]
    public class FloatEvent : UnityEvent<float> { }

    public Material material;

    public string propertyName;
    public FloatEvent Binding;

    void Awake()
    {
        Binding.Invoke(material.GetFloat(propertyName));
    }
}
