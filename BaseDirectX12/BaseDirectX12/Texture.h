#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <vector>
#include <map>
#include <string>

class Texture
{
	// 二次元座標
	struct Vector2
	{
		float x;
		float y;
	};

	// 画像サイズ
	struct Size
	{
		LONG	width;
		LONG	height;
	};

	// データ
	struct Descriptor
	{
		//ヒープ
		ID3D12DescriptorHeap* heap;
		//リソース
		ID3D12Resource* resorce;
	};

	//頂点座標
	typedef struct
	{
		//頂点座標
		DirectX::XMFLOAT3	pos;
		//uv値
		DirectX::XMFLOAT2	uv;
	}Vertex;

	// BMPデータの塊
	struct Data
	{
		//画像サイズ
		Size		size;
		//bmpデータ
		std::vector<UCHAR>bmp;
		//ディスクリプター
		Descriptor texture[2];
		//頂点情報
		std::vector<Vertex>vertex;
	};

public:
	// デストラクタ
	~Texture();

	// インスタンス化
	static void Create(void);
	// 破棄
	static void Destroy(void);

	// インスタンス変数取得
	static Texture* GetInstance()
	{
		return s_Instance;
	}

	// 画像データの読み込み
	HRESULT LoadBMP(USHORT index, std::string fileName, UINT swapBufferNum, ID3D12Device* dev);

	// 描画
	HRESULT Draw(USHORT index, Vector2 pos, Vector2 size, Vector2 wSize, ID3D12GraphicsCommandList* list);

private:
	// コンストラクタ
	Texture();

	// 頂点ヒープの生成
	HRESULT CreateVertexHeap(USHORT index, UINT swapBufferNum, ID3D12Device* dev);
	// 頂点バッファの生成
	HRESULT CreateVertexBuffer(USHORT index, ID3D12Device* dev);

	// 定数ヒープの生成
	HRESULT CreateConstantHeap(USHORT index, ID3D12Device* dev);
	// 定数バッファの生成
	HRESULT CreateConstantBuffer(USHORT index, ID3D12Device* dev);

	// 頂点設定
	HRESULT SetVertex(USHORT index, Vector2 pos, Vector2 size, Vector2 wSize);

	// インスタンス変数
	static Texture* s_Instance;
	UCHAR* d = nullptr;
	// 参照結果
	HRESULT result;

	// データ
	std::map<USHORT, Data>data;

	// 頂点バッファビュー設定用構造体
	D3D12_VERTEX_BUFFER_VIEW view;
};

