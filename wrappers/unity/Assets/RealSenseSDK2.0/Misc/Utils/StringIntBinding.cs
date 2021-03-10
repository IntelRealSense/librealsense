using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class StringIntBinding : MonoBehaviour
{
    public string propertyName;

    [System.Serializable]
    public class StringIntEvent : UnityEvent<string, int> { }

    public StringIntEvent Binding;

    public int Value
    {
        set
        {
            Binding.Invoke(propertyName, value);
        }
    }

}
