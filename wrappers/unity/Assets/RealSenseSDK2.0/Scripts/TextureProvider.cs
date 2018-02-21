using UnityEngine;
using UnityEngine.Events;

public abstract class TextureProvider : MonoBehaviour
{
    [System.Serializable]
    public class TextureEvent : UnityEvent<Texture> { }
    public TextureEvent TextureBinding;

    [System.Serializable]
    public class FloatEvent : UnityEvent<float> { }
    public FloatEvent AspectRatioBinding;

    public enum TextureType { Color, DepthOrIr, Custom };
    public TextureType texturetype = TextureType.Custom;

    public Texture2D texture;

    public delegate void TextureUpdateDelegate();
    public TextureUpdateDelegate onTextureUpdate;

    public TextureFormat texFormat = TextureFormat.ARGB32;
    public FilterMode filterMode = FilterMode.Point;
    public TextureWrapMode wrapMode = TextureWrapMode.Clamp;

    public virtual void Start()
    {
        switch (texturetype)
        {
            case TextureType.Color:
                texFormat = TextureFormat.RGBA32;
                break;
            case TextureType.DepthOrIr:
                texFormat = TextureFormat.R16;
                break;
        }
    }
}
