Shader "Custom/PointCloud" {
	Properties {
		_MainTex ("Texture", 2D) = "white" {}
		_UVMap ("UV", 2D) = "white" {}
		_PointSize("Point Size (GL only)", Float) = 4.0
		_Color ("PointCloud Color", Color) = (1, 1, 1, 1)
	}

	SubShader
	{
		Pass 
		{
		
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag

			#include "UnityCG.cginc"

			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
			};

			struct v2f
			{
				float4 vertex : SV_POSITION;
				float size : PSIZE;
				float2 uv : TEXCOORD0;
			};

			float _PointSize;
			fixed4 _Color;

			sampler2D _MainTex;
			float4 _MainTex_TexelSize;

			sampler2D _UVMap;

			v2f vert (appdata v)
			{
				v2f o;
				v.vertex.y = -v.vertex.y;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.size = _PointSize; // GL only
				o.uv = v.uv;
				return o;
			}

			fixed4 frag (v2f i) : SV_Target
			{
				float2 uv = tex2D(_UVMap, i.uv);
				if(any(uv <= 0 || uv >= 1))
					discard;
				// offset to pixel center
				uv += 0.5 * _MainTex_TexelSize.xy;
				return tex2D(_MainTex, uv) * _Color;
			}
			ENDCG
		}
	}
}
