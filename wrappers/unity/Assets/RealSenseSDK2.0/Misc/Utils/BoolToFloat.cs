using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class BoolToFloat : MonoBehaviour
{
    [Serializable]
    public class FloatEvent : UnityEvent<float> { }

    public FloatEvent OnValueChange;

    public bool Value
    {
        set
        {
            OnValueChange.Invoke(Convert.ToSingle(value));
        }
    }
}
