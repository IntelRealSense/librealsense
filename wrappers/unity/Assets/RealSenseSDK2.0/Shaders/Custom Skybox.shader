Shader "Skybox/Custom Skybox"
{
    Properties
    {
        _TopColor("Top Color", Color) = (0, 0, 0, 1)
        _BottomColor("Bottom Color", Color) = (0, 0.674, 0.878, 1)
    }

    CGINCLUDE

    #include "UnityCG.cginc"

    float4 _TopColor;
    float4 _BottomColor;

    struct appdata
    {
        float4 position : POSITION;
        float3 texcoord : TEXCOORD0;
    };
    
    struct v2f
    {
        float4 position : SV_POSITION;
        float3 texcoord : TEXCOORD0;
        float4 scrPos : TEXCOORD1;
    };
    
    
    v2f vert (appdata v)
    {
        v2f o;
        o.position = UnityObjectToClipPos (v.position);
        o.scrPos = o.position;
        o.texcoord = v.texcoord;
        return o;
    }
    
    half4 frag (v2f i) : COLOR
    {
        float4 col = lerp(_TopColor, _BottomColor, saturate(i.scrPos.y / i.scrPos.w * 0.5 + 0.5));
        return col;

        #define PI 3.141592653589793
        float3 p = normalize(i.texcoord);


        float th = acos(p.y);
        float th0 = th - PI/2;
        float phi = atan2(p.x, p.z) + PI;

        th = fmod(th, PI/16);
        phi = fmod(phi, PI/16);

        th = abs(th - PI/32);
        phi = abs(phi - PI/32);

        th = saturate(lerp(1, 0, th*200));
        phi = saturate(lerp(1, 0, phi*200));


        float g = th + phi;
        float f = 1 - 0.5*th0*th0;
        g = lerp(1 - g*f, f, 0.85);
        return float4(g.xxx, 1) * col;

        return float4(p.xz, 0, 1);
    }

    ENDCG

    SubShader
    {
        Tags { "RenderType"="Background" "Queue"="Background" }
        Pass
        {
            ZWrite Off
            Cull Off
            Fog { Mode Off }
            CGPROGRAM
            #pragma fragmentoption ARB_precision_hint_fastest
            #pragma vertex vert
            #pragma fragment frag
            ENDCG
        }
    } 
}
