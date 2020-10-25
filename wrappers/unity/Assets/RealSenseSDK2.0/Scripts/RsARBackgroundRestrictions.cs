using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RsARBackgroundRestrictions : MonoBehaviour {

#if UNITY_2020_1_OR_NEWER
    void Start ()
    {
        GameObject arPanel = GameObject.Find ("AR") as GameObject;
        if (arPanel != null)
            arPanel.SetActive(false);
    }
#endif

}
