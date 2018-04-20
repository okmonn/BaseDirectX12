Texture2D<float4> tex : register(t0);
Texture2D<float4> spa : register(t1);
SamplerState      smp : register(s0);

//ワールドビュープロジェクション
cbuffer wvp : register(b0)
{
    //ワールド行列
    float4x4 world;
    //ビュープロジェクション行列
    float4x4 viewProjection;
}

//マテリアル
cbuffer mat : register(b1)
{
    //基本色
    float3 diffuse;
    //テクスチャ対応フラグ
    bool existTexture;
}

//ボーン
cbuffer born : register(b2)
{
    //ボーン
    matrix borns[256];
}

//出力
struct Out
{
	//システム座標
    float4 svpos  : SV_POSITION;
	//座標
    float4 pos    : POSITION;
    //uv値
    float2 uv     : TEXCOORD;
    //法線
    float3 normal : NORMAL;
    //色
    float4 color  : COLOR;
};

//入力
struct VSInput
{
    //座標
    float4 pos        : POSITION;
    //法線
    float4 normal     : NORMAL;
    //uv
    float2 uv         : TEXCOORD;
    //ボーン
    min16uint2 bornID : BORN;
    //ウェイト
    min16uint weight  : WEIGHT;
};

//テクスチャ用頂点シェーダ
Out TextureVS(VSInput input)
{
    /*座標補正
	pos.xy = float2(-1, 1) + (pos.xy / float2((640 / 2), -(480 / 2)));*/

    input.pos = mul(mul(viewProjection, world), input.pos);
    
    Out o;
    o.svpos     = input.pos;
    o.pos       = input.pos;
    o.uv        = input.normal;
    o.normal    = input.normal;
    o.color     = float4(1, 0, 0, 1);

    return o;
}

//テクスチャ用ピクセルシェーダ
float4 TexturePS(Out o) : SV_TARGET
{
    //return float4(tex.Sample(smp, o.uv).abg, 1);
    return float4(0, 0, 1, 0);
}


//モデル用頂点シェーダー
Out ModelVS(VSInput input)
{
    float w1 = (float) (input.weight) / 100.0f;
    float w2 = (float) (1.0f - w1);
    matrix m = borns[input.bornID[0]] * w1 + borns[input.bornID[1]] * w2;

    m = mul(world, m);

    input.pos = mul(mul(viewProjection, m), input.pos);
    
    Out o;
    o.svpos     = input.pos;
    o.pos       = input.pos;
    o.uv        = input.uv;
    o.normal    = mul(world, input.normal);
    if (input.bornID[0] == 19 || input.bornID[1] == 19)
    {
        o.color = float4(1, 0, 0, 1) * (float(input.weight) / 100.0);
    }

    return o;
}

//モデル用ピクセルシェーダー
float4 ModelPS(Out o) : SV_TARGET
{
    //光源へのベクトル
    float3 light       = normalize(float3(-1, 1, -1));
    //内積
    float brightness    = dot(o.normal, light);
    //色
    float3 color        = (existTexture == true ? tex.Sample(smp, o.uv).rgb : diffuse);

    return float4(color * brightness, 1);
}