using System.Collections;
using System.Collections.Generic;
using Intel.RealSense;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.Rendering;
using UnityEngine.XR;

public class RsARBackgroundRenderer : MonoBehaviour
{
    public RsFrameProvider Source;
    public Material material;
    private Camera cam;
    private ARBackgroundRenderer bg;
    private Intrinsics intrinsics;
    private RenderTexture rt;

    Vector2Int screenSize;

    IEnumerator Start()
    {
        yield return new WaitUntil(() => Source && Source.Streaming);

        using (var profile = Source.ActiveProfile.GetStream<VideoStreamProfile>(Stream.Color))
        {
            intrinsics = profile.GetIntrinsics();
        }

        cam = GetComponent<Camera>();

        bg = new ARBackgroundRenderer()
        {
            backgroundMaterial = material,
            mode = ARRenderMode.MaterialAsBackground,
            backgroundTexture = material.mainTexture
        };

        cam.depthTextureMode |= DepthTextureMode.Depth;

        // Uses the same material as above to update camera's depth texture
        // Unity will use it when calculating shadows
        var updateCamDepthTexture = new CommandBuffer() { name = "UpdateDepthTexture" };
        updateCamDepthTexture.Blit(BuiltinRenderTextureType.None, BuiltinRenderTextureType.CurrentActive, material);
        updateCamDepthTexture.SetGlobalTexture("_ShadowMapTexture", Texture2D.whiteTexture);
        cam.AddCommandBuffer(CameraEvent.AfterDepthTexture, updateCamDepthTexture);

        // assume single directional light
        var light = FindObjectOfType<Light>();

        // Copy resulting screenspace shadow map, ARBackgroundRenderer's material will multiply it over color image
        var copyScreenSpaceShadow = new CommandBuffer { name = "CopyScreenSpaceShadow" };
        int shadowCopyId = Shader.PropertyToID("_ShadowMapTexture");
        copyScreenSpaceShadow.GetTemporaryRT(shadowCopyId, -1, -1, 0);
        copyScreenSpaceShadow.CopyTexture(BuiltinRenderTextureType.CurrentActive, shadowCopyId);
        copyScreenSpaceShadow.SetGlobalTexture(shadowCopyId, shadowCopyId);
        light.AddCommandBuffer(LightEvent.AfterScreenspaceMask, copyScreenSpaceShadow);
    }

    void OnEnable()
    {
        if (bg != null)
            bg.mode = ARRenderMode.MaterialAsBackground;
    }

    void OnDisable()
    {
        if (bg != null)
            bg.mode = ARRenderMode.StandardBackground;
    }

    void Update()
    {
        if (cam == null)
            return;

        var s = new Vector2Int(Screen.width, Screen.height);
        if (screenSize != s)
        {
            screenSize = s;

            var projectionMatrix = new Matrix4x4
            {
                m00 = intrinsics.fx,
                m11 = -intrinsics.fy,
                m03 = intrinsics.ppx / intrinsics.width,
                m13 = intrinsics.ppy / intrinsics.height,
                m22 = (cam.nearClipPlane + cam.farClipPlane) * 0.5f,
                m23 = cam.nearClipPlane * cam.farClipPlane,
            };
            float r = (float)intrinsics.width / Screen.width;
            projectionMatrix = Matrix4x4.Ortho(0, Screen.width * r, Screen.height * r, 0, cam.nearClipPlane, cam.farClipPlane) * projectionMatrix;
            projectionMatrix.m32 = -1;

            cam.projectionMatrix = projectionMatrix;
        }

        // if (Input.GetMouseButtonDown(0))
        // Debug.Log(GetImagePoint(Input.mousePosition));
    }

    public Vector2Int GetImagePoint(Vector2 screenPos)
    {
        var vp = (Vector2)Camera.main.ScreenToViewportPoint(screenPos);
        vp.y = 1f - vp.y;

        float sr = (float)Screen.width / Screen.height;
        float tr = (float)intrinsics.height / intrinsics.width;
        float sh = sr * tr;
        vp -= 0.5f * Vector2.one;
        vp.y /= sh;
        vp += 0.5f * Vector2.one;

        return new Vector2Int((int)(vp.x * intrinsics.width), (int)(vp.y * intrinsics.height));
    }
}
