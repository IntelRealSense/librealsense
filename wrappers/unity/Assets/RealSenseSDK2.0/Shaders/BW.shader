Shader "Custom/BW" {
	Properties {
		_MainTex ("MainTex", 2D) = "white" {}
		_Gamma ("Gamma", float) = 0.45
	}
	SubShader {
		Tags { "QUEUE"="Transparent" "IGNOREPROJECTOR"="true" "RenderType"="Transparent" "PreviewType"="Plane" }
		Pass {		
			ZWrite Off
			Cull Off
			Fog { Mode Off }
			
			ColorMask RGB

			CGPROGRAM
			#pragma vertex vert_img
	        #pragma fragment frag
	        #pragma target 3.0
	        #pragma glsl
	        
			#include "UnityCG.cginc"

	        sampler2D _MainTex;
			float _Gamma;

	        half4 frag (v2f_img pix) : SV_Target
	        {
				float Y = pow(tex2D(_MainTex, pix.uv).a, _Gamma);
				return float4(Y.rrr, 1);
	        }
	        ENDCG

  		 }
	} 
	FallBack Off
}
