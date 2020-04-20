using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class BoolInvert : MonoBehaviour
{
    [Serializable]
    public class BooleanEvent : UnityEvent<bool> { }

    public BooleanEvent @event;

    public bool Value
    {
        set
        {
            @event.Invoke(!value);
        }
    }
}
