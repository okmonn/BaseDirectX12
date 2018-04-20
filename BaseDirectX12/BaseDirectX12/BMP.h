#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

class BMP
{
	//画像サイズの構造体
	typedef struct
	{
		LONG	width;
		LONG	height;
	}IMAGE_SIZE;

	//BMPデータの構造体
	typedef struct
	{
		//画像サイズ
		IMAGE_SIZE				size;
		//bmpデータ
		std::vector<UCHAR>		bmp;
		//ヒープ
		ID3D12DescriptorHeap*	heap;
		//リソース
		ID3D12Resource*			resource;
	}Data;

public:
	// デストラクタ
	~BMP();

	// インスタンス
	static void Create(void);
	// 破棄
	static void Destroy(void);

	// 取得
	static BMP* GetInstance(void)
	{
		return s_Instance;
	}

	//=====読み込み=====
	// BMPデータの読み込み
	HRESULT LoadBMP(USHORT index, std::string fileName, ID3D12Device* dev);
	// WICテクスチャの読み込み
	HRESULT LoadTextureWIC(USHORT index, std::wstring fileName, ID3D12Device* dev);

	//=====生成=====
	// ヒープの生成
	HRESULT CreateHeap(USHORT index, ID3D12Device* dev);
	// リソースの生成
	HRESULT CreateResource(USHORT index, ID3D12Device* dev);

	//=====描画準備=====
	// 描画準備
	void SetDraw(USHORT index, ID3D12GraphicsCommandList* list);
	// WIC用描画準備
	void SetDrawWIC(USHORT index, ID3D12GraphicsCommandList* list);

	// 描画
	void Draw(USHORT index, ID3D12GraphicsCommandList* list);

private:
	// コンストラクタ
	BMP();

	// インスタンス変数
	static BMP* s_Instance;

	// 参照結果
	HRESULT result;

	// BMPデータの構造体の配列
	std::map<USHORT, Data>data;

	// デコードデータの配列
	std::map<USHORT, std::unique_ptr<uint8_t[]>>decode;

	// サブリソースの配列
	std::map<USHORT, D3D12_SUBRESOURCE_DATA>sub;
};

