Shader "Custom/Depth" {
	Properties {
		_MainTex ("MainTex", 2D) = "white" {}
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
	        #pragma target 3.0
	        #pragma glsl
	        
			#include "UnityCG.cginc"

	        sampler2D _MainTex;

	        half4 frag (v2f_img pix) : SV_Target
	        {
				float Y = pow(tex2D(_MainTex, pix.uv).r, 0.4);
				return float4(Y, Y, Y, 1);
	        }
	        ENDCG

  		 }
	} 
	FallBack Off
}
