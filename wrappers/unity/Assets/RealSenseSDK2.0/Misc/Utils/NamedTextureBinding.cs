using UnityEngine;
using System.Collections;
using UnityEngine.Events;

public class NamedTextureBinding : MonoBehaviour {
    public Texture Texture
    {
        set
        {
            textureBinding.Invoke(textureName, value);
        }
    }

    [System.Serializable]
    public class NamedTextureEvent : UnityEvent<string, Texture> { }

    public string textureName;
    public NamedTextureEvent textureBinding;
}
