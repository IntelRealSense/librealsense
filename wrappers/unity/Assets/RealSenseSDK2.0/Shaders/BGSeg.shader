// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "Custom/BGSeg" {
	Properties
	{
		[NoScaleOffset][PerRendererData]_MainTex("MainTex", 2D) = "black" {}
		[NoScaleOffset]_ColorTex("Base (RGB)", 2D) = "white" {}
		_MinRange("Min Range(m)", Float) = 0.5
		_MaxRange("Max Range(m)", Float) = 2.0
		_Feather("Feather Range(m)", Float) = 0.25
		_DepthScale("Depth Multiplyer Factor to Meters", float) = 0.001
		_Gamma ("Gamma", float) = 1
		[Toggle] _HasColor ("Has Color?", Float) = 1
	}
	SubShader
	{
		Tags { "QUEUE" = "Transparent" "IGNOREPROJECTOR" = "true" "RenderType" = "Transparent" "PreviewType" = "Plane" }
		Pass 
		{
			ZWrite Off
			Cull Off
			Blend SrcAlpha OneMinusSrcAlpha
			Fog { Mode Off }

			CGPROGRAM
			#pragma vertex vert_img
			#pragma fragment frag
			#pragma target 3.0
			#pragma glsl
			#pragma multi_compile _HASCOLOR_OFF _HASCOLOR_ON

			#include "UnityCG.cginc"

			sampler2D _MainTex;
			sampler2D _ColorTex;
			float _MinRange;
			float _MaxRange;
			float _DepthScale;
			float _Feather;
			float _Gamma;

			half4 frag(v2f_img pix) : SV_Target
			{
				float r = tex2D(_MainTex, pix.uv).r; //r is unscaled depth, normalized to [0-1]
				float distMeters = r * 65536 * _DepthScale; //resotring to uint16 and multiplying by depth scale to get depth in meters

				#if _HASCOLOR_ON
					float3 Y = pow(tex2D(_ColorTex, pix.uv).rgb, _Gamma);
				#else
					float3 Y = pow(tex2D(_ColorTex, pix.uv).aaa, _Gamma);
				#endif

				float mask = smoothstep(_MinRange - _Feather * 0.5, _MinRange + _Feather * 0.5, distMeters) * 
							 smoothstep(distMeters - _Feather, distMeters + _Feather, _MaxRange);
				
				return float4(Y, mask);
			}
			ENDCG
			}
	}
}