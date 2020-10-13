using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class StringFloatBinding : MonoBehaviour
{

    [System.Serializable]
    public class StringFloatEvent : UnityEvent<string, float> { }

    public string Name;

    public float Value
    {
        set
        {
            Binding.Invoke(Name, value);
        }
    }
    public StringFloatEvent Binding;
}
