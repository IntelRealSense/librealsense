Shader "Custom/Depth" {
	Properties {
		
		[PerRendererData]
		_MainTex ("MainTex", 2D) = "white" {}
		
		[HideInInspector]
		_Colormaps ("Colormaps", 2D) = "" {}

		_DepthScale("Depth Multiplyer Factor to Meters", float) = 0.001 
		
		_MinRange("Min Range(m)", Range(0, 10)) = 0.15
		_MaxRange("Max Range(m)", Range(0, 20)) = 10.0

		[KeywordEnum(Viridis,Plasma,Inferno,Jet,Rainbow,Coolwarm,Flag,Gray)]
		_Colormap("Colormap", Float) = 0

	}
	SubShader {
		Tags { "QUEUE"="Transparent" "IGNOREPROJECTOR"="true" "RenderType"="Transparent" "PreviewType"="Plane" }
		Pass {
			ZWrite Off
			Cull Off
			Fog { Mode Off }

			CGPROGRAM
			#pragma vertex vert_img
			#pragma fragment frag
			#pragma multi_compile _COLORMAP_VIRIDIS _COLORMAP_PLASMA

			#include "UnityCG.cginc"

			sampler2D _MainTex;
			sampler2D _Colormaps;
			float4 _Colormaps_TexelSize;
			
			float _Colormap;
			float _MinRange;
			float _MaxRange;
			float _DepthScale;

			half4 frag (v2f_img pix) : SV_Target
			{
				// [0..1] -> ushort -> meters
				float z = tex2D(_MainTex, pix.uv).r * 0xffff * _DepthScale;
				z = (z - _MinRange) / (_MaxRange - _MinRange);
				if(z <= 0 || z >= 1)
					return 0;
				return tex2D(_Colormaps, float2(z, 1 - (_Colormap + 0.5) * _Colormaps_TexelSize.y));
			}
			ENDCG
		}
	}
	FallBack Off
}
