using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class FloatToStringFormat : MonoBehaviour
{

    [System.Serializable]
    public class StringEvent : UnityEvent<string> { }

    public string format = "{0}";
    public StringEvent Binding;

    public float Value
    {
        set
        {
            Binding.Invoke(string.Format(format, value));
        }
    }

}
