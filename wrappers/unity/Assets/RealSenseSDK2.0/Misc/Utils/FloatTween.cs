using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class FloatTween : MonoBehaviour
{

    public bool RunOnAwake = true;

    public float StartDelay = 0.1f;
    public float fromValue = 0f;
    public float toValue = 1f;
    public float Speed = 1f;

    [System.Serializable]
    public class FloatEvent : UnityEvent<float> { }

    public FloatEvent OnChange;

    public UnityEvent OnComplete;

    void Awake()
    {
        if (RunOnAwake)
            Tween();
    }

    public void Tween()
    {
        StartCoroutine(Tween(fromValue, toValue));
    }

    IEnumerator Tween(float from, float to)
    {
        OnChange.Invoke(from);
        yield return new WaitForSeconds(StartDelay);

        var value = from;
        while (!Mathf.Approximately(value, to))
        {
            value = Mathf.MoveTowards(value, to, Time.deltaTime * Speed);
            OnChange.Invoke(value);
            yield return null;
        }

        OnComplete.Invoke();
    }

}
