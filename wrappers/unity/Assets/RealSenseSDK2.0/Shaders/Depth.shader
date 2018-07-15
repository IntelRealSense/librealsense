// Colormaps texture generated using this python script:

// from PIL import Image
// import matplotlib.cm as cm
// import numpy as np
// names = ['viridis', 'plasma', 'inferno', 'jet', 'rainbow', 'coolwarm', 'flag', 'gray']
// def cm_array(m, size=256):
//     return cm.ScalarMappable(cmap=getattr(cm, m)).to_rgba(range(size), bytes=True).reshape(1, size, 4)
// Image.fromarray(np.vstack(map(cm_array, names)), mode='RGBA').save('colormaps.png')
// print ','.join(map(lambda x: x.title(), names))

Shader "Custom/Depth" {
	Properties {
		
		[PerRendererData]
		_MainTex ("MainTex", 2D) = "black" {}
		
		[HideInInspector]
		_Colormaps ("Colormaps", 2D) = "" {}

		_DepthScale("Depth Multiplier Factor to Meters", float) = 0.001 
		
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
				if(z <= 0)
					return 0;
				return tex2D(_Colormaps, float2(z, 1 - (_Colormap + 0.5) * _Colormaps_TexelSize.y));
			}
			ENDCG
		}
	}
	FallBack Off
}
