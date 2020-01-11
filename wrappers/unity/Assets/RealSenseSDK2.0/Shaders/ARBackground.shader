Shader "Custom/ARBackground"
{
	Properties
	{
		[NoScaleOffset]
		_MainTex ("Texture", 2D) = "white" {}
		
		[NoScaleOffset]
		_DepthTex ("Depth", 2D) = "black" {}

		_BgColor ("Background", Color) = (0.003921569, 0.7098039, 0.9333333, 1)
		_MinRange("Min Range(m)", Range(0, 10)) = 0.15
		_MaxRange("Max Range(m)", Range(0, 20)) = 10.0

		[HideInInspector]
		_Colormaps ("Colormaps", 2D) = "" {}

		[KeywordEnum(Viridis,Plasma,Inferno,Jet,Rainbow,Coolwarm,Flag,Gray)]
		_Colormap("Colormap", Float) = 0
	}
	SubShader
	{
		Tags { "RenderType"="Opaque" }
		LOD 100

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
				float2 uv : TEXCOORD0;
				float2 uv1 : TEXCOORD1;
				float4 vertex : SV_POSITION;
			};

			sampler2D _MainTex;
			float4 _MainTex_TexelSize;
			
			sampler2D _DepthTex;

			sampler2D _Colormaps;
			float4 _Colormaps_TexelSize;
			
			float4 _BgColor;
			float _Colormap;
			float _MinRange;
			float _MaxRange;

			float _Blend;

			sampler2D _ShadowMapTexture;

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				
				o.uv = v.uv;
				o.uv.y = 1 - o.uv.y;
				
				// Scale and fix aspect ratio
				float sr = _ScreenParams.y / _ScreenParams.x;
				float tr = _MainTex_TexelSize.x * _MainTex_TexelSize.w;
				o.uv -= 0.5;
				o.uv.y *= sr;
				o.uv.x *= tr;
				o.uv /= tr;
				o.uv += 0.5;

				o.uv1 = v.uv;

				return o;
			}
			
			fixed4 frag (v2f i, out float depth : DEPTH) : SV_Target
			{
				depth = 0;
				if(i.uv.y < 0 || i.uv.y > 1) {
					depth = 1;
					return _BgColor;
				}

				float d = tex2D(_DepthTex, i.uv);
				if(d > 0) {
					d = d * 0xffff * 0.001;
					depth = LinearEyeDepth(d);
				}

				float4 col = tex2D(_MainTex, i.uv);

				float z = (d - _MinRange) / (_MaxRange - _MinRange);
				if(z > 0) {
					float4 c = tex2D(_Colormaps, float2(z, 1 - (_Colormap + 0.5) * _Colormaps_TexelSize.y));
					col = lerp(col, c, _Blend * 0.5);
				}
				
				col *= tex2D(_ShadowMapTexture, i.uv1);

				return col;
			}
			ENDCG
		}
	}
}
