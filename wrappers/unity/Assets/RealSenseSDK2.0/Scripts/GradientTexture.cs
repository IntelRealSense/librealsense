using System;
using UnityEngine;
using UnityEngine.Events;

public class GradientTexture : MonoBehaviour
{
    public UnityEngine.Gradient gradient;
    private Texture2D tex;

    [System.Serializable]
    public class TextureEvent : UnityEvent<Texture> { }
    public TextureEvent TextureBinding;

    const int numSamples = 1024;

    public void Start()
    {
        tex = new Texture2D(numSamples, 1);
        for (int i = 0; i < tex.width; i++)
        {
            float t = (float)i / (float)tex.width;
            tex.SetPixel(i, 0, gradient.Evaluate(t));
        }
        tex.Apply();
        TextureBinding.Invoke(tex);
    }
}
