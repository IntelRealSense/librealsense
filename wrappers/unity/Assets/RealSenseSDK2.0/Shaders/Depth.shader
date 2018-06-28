Shader "Custom/Depth" {
	Properties {
		_MainTex ("MainTex", 2D) = "white" {}
		_DepthScale("Depth Multiplyer Factor to Meters", float) = 0.001
	    _MinRange("Min Range(m)", Range(0, 10)) = 0
		_MaxRange("Max Range(m)", Range(0, 10.0)) = 5.0
		[Toggle] _GrayScale("Gray scale", Float) = 0
		_Red("Red", Range(0, 1)) = 0.2
		_Green("Green", Range(0, 1)) = 0.2
		_Blue("Blue", Range(0, 1)) = 0.2
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
			float _DepthScale;
			float _MinRange;
			float _MaxRange;
			bool _GrayScale;
			float _Red;
			float _Green;
			float _Blue;

	        half4 frag (v2f_img pix) : SV_Target
	        {
				float maxGray16 = 0xffff;
				float z = tex2D(_MainTex, pix.uv).r; //r is unscaled depth, normalized to [0-1]
				float distMeters = z * maxGray16 * _DepthScale; //resotring to uint16 and multiplying by depth scale to get depth in meters

				if (distMeters < _MinRange || distMeters > _MaxRange || distMeters == 0)
					return float4(0, 0, 0, 1);

				float r, g, b;

				float norm = (distMeters - _MinRange) / (_MaxRange - _MinRange);

				if (_GrayScale)
				{
					return float4(norm, norm, norm, 1);
				}

				_Red = _Red < 0 ? 0 : (_Red > 1 ? 1 : _Red);
				_Green = _Green < 0 ? 0 : (_Green > 1 ? 1 : _Green);
				_Blue = _Blue < 0 ? 0 : (_Blue > 1 ? 1 : _Blue);

				_Green *= 15;
				r = pow(_Red, 1 - norm);
				g = _Green * (-pow(norm, 2) + norm);
				b = pow(_Blue, norm);

				return float4(r, g, b, 1);
	        }
	        ENDCG

  		 }
	} 
	FallBack Off
}
