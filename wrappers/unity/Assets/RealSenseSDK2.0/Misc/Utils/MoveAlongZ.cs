using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class MoveAlongZ : MonoBehaviour
{

    public float PositionZ
    {
        set
        {
            transform.position = Vector3.forward * value;
        }
    }
}
