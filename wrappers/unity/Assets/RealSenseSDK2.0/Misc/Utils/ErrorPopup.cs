using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class ErrorPopup : MonoBehaviour
{

    public GameObject PopupPrefab;

    void Awake()
    {
        Application.logMessageReceived += OnLogMessageRecevied;
    }

    void OnDestroy()
    {
        Application.logMessageReceived -= OnLogMessageRecevied;
    }

    private void OnLogMessageRecevied(string logString, string stackTrace, LogType type)
    {
        StartCoroutine(Popup(logString));
    }

    IEnumerator Popup(string text)
    {
        yield return null;

        var go = Instantiate(PopupPrefab) as GameObject;
        go.GetComponent<RectTransform>().SetParent(GetComponent<RectTransform>());
        go.GetComponent<RectTransform>().SetAsFirstSibling();

        var txt = go.GetComponentInChildren<Text>();
        txt.text = string.Format("<color=#00000040>[{0}]</color> {1}", DateTime.Now.ToString(), text);

        var btn = go.GetComponentInChildren<Button>();
        // btn.onClick.AddListener(() => Destroy(go));
        btn.onClick.AddListener(() =>
        {
            var fade = go.GetComponent<FloatTween>();
            fade.fromValue = 1;
            fade.toValue = 0;
            fade.OnComplete.AddListener(() => Destroy(go));
            fade.Tween();
        });
    }
}
