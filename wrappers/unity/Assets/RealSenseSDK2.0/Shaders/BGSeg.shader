// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "Custom/BGSeg" {
	Properties
	{
		_MainTex("MainTex", 2D) = "white" {}
		_ColorTex("Base (RGB)", 2D) = "white" {}
        _MinRange("Min Range(m)", float) = 0.5
        _MaxRange("Max Range(m)", float) = 2.0
        _DepthScale("Depth Multiplyer Factor to Meters", float) = 1
		[Toggle] _HasColor ("Has Color?", Float) = 0
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

			half4 frag(v2f_img pix) : SV_Target
			{
                float r = tex2D(_MainTex, pix.uv).r; //r is unscaled depth, normalized to [0-1]
                float distMeters = r * 65536 * _DepthScale; //resotring to uint16 and multiplying by depth scale to get depth in meters

				#if _HASCOLOR_ON
					float3 Y = pow(tex2D(_ColorTex, pix.uv).rgb, 0.4);
				#else
					float Y = pow(tex2D(_ColorTex, pix.uv).aaa, 0.4);
				#endif

				float alpha = 1;
                if (distMeters == 0 || distMeters < _MinRange)
                {
                    alpha = 0;
                }
                else if(distMeters > _MaxRange) //Background or no depth data
				{
                    alpha = (distMeters - _MaxRange) / distMeters;
				}
				else
				{
                    alpha = 1; //Withing the range
				}

                if (alpha < 1)
                {
                    Y = dot(Y, float3(0.15, 0.3, 0.05)); //To Grayscale(0.3, 0.59, 0.11)
                    alpha = 0.7;
                }
				#if _HASCOLOR_ON
					return float4(Y, alpha);
				#else
					return float4(Y, Y, Y, alpha);
				#endif
			}
			ENDCG
			}
	}
}