using UnityEngine;
using UnityEngine.UI;

public class MinMaxSlider : MonoBehaviour
{

    public Slider minSlider;
    public Slider maxSlider;
    public RectTransform fillRect;

    void Awake()
    {
        minSlider.onValueChanged.AddListener(OnValueChanged);
        maxSlider.onValueChanged.AddListener(OnValueChanged);
        
        OnValueChanged(0);
    }

    void OnValueChanged(float v)
    {
        float min = minSlider.value, max = maxSlider.value;
        minSlider.value = Mathf.Min(min, max);
        maxSlider.value = Mathf.Max(min, max);
        
        fillRect.anchorMin = Vector2.right * minSlider.normalizedValue;
        fillRect.anchorMax = Vector2.right * maxSlider.normalizedValue;

    }
}
