#include "BMP.h"
#include "WICTextureLoader12.h"
#include <tchar.h>
#include<sstream>

#pragma comment (lib,"d3d12.lib")
#pragma comment (lib,"dxgi.lib")

//インスタンス変数の初期化
BMP* BMP::s_Instance = nullptr;

// コンストラクタ
BMP::BMP()
{
	//参照結果の初期化
	result = S_OK;

	//BMPデータ構造体配列の初期化
	data.clear();

	//WICの初期処理
	CoInitialize(nullptr);

	//デコードデータ配列の初期化
	decode.clear();

	//サブリソース配列の初期化
	sub.clear();


	//エラーを出力に表示させる
#ifdef _DEBUG
	ID3D12Debug *debug = nullptr;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (FAILED(result))
		int i = 0;
	debug->EnableDebugLayer();
	debug->Release();
	debug = nullptr;
#endif
}

// デストラクタ
BMP::~BMP()
{
	//データ
	for (auto itr = data.begin(); itr != data.end(); ++itr)
	{
		if (itr->second.heap != nullptr)
		{
			itr->second.heap->Release();
		}
		if (itr->second.resource != nullptr)
		{
			itr->second.resource->Release();
		}
	}

	//デコードデータ
	for (auto itr = decode.begin(); itr != decode.end(); ++itr)
	{
		if (itr->second != nullptr)
		{
			itr->second.release();
		}
	}
}

// インスタンス
void BMP::Create(void)
{
	if (s_Instance == nullptr)
	{
		s_Instance = new BMP;
	}
}

// 破棄
void BMP::Destroy(void)
{
	if (s_Instance != nullptr)
	{
		delete s_Instance;
	}

	s_Instance = nullptr;
}

// BMPデータの読み込み
HRESULT BMP::LoadBMP(USHORT index, std::string fileName, ID3D12Device* dev)
{
	//BMPヘッダー構造体
	BITMAPINFOHEADER header;
	SecureZeroMemory(&header, sizeof(header));

	//BMPファイルヘッダー
	BITMAPFILEHEADER fileheader;
	SecureZeroMemory(&fileheader, sizeof(fileheader));

	//ダミー宣言
	const char* path = fileName.c_str();

	//ファイル
	FILE *file;

	//ファイル開らく
	if ((fopen_s(&file, path, "rb")) != 0)
	{
		//エラーナンバー確認
		auto a = (fopen_s(&file, path, "rb"));
		std::stringstream s;
		s << a;
		OutputDebugString(_T("ファイルを開けませんでした：失敗\n"));
		OutputDebugStringA(s.str().c_str());
		return S_FALSE;
	}

	//BMPファイルヘッダー読み込み
	fread(&fileheader, sizeof(fileheader), 1, file);

	//BMPヘッダー読み込み
	fread(&header, sizeof(header), 1, file);

	//画像の幅と高さの保存
	data[index].size = { header.biWidth ,header.biHeight };

	//データサイズ分のメモリ確保(ビットの深さが32bitの場合)
	//bmp.resize(header.biSizeImage);
	//データサイズ分のメモリ確保(ビットの深さが24bitの場合)
	data[index].bmp.resize(header.biWidth * header.biHeight * 4);

	UCHAR* ptr = &data[index].bmp[0];

	for (int line = header.biHeight - 1; line >= 0; --line)
	{
		for (int count = 0; count < header.biWidth * 4; count += 4)
		{
			//一番左の配列番号
			UINT address = line * header.biWidth * 4;
			data[index].bmp[address + count] = 0;
			fread(&data[index].bmp[address + count + 1], sizeof(UCHAR), 3, file);
		}
	}

	//ファイルを閉じる
	fclose(file);

	if (CreateHeap(index, dev) == S_OK)
	{
		result = CreateResource(index, dev);
	}

	return result;
}

// WICテクスチャの読み込み
HRESULT BMP::LoadTextureWIC(USHORT index, std::wstring fileName, ID3D12Device * dev)
{
	//読み込み
	result = DirectX::LoadWICTextureFromFile(dev, fileName.c_str(), &data[index].resource, decode[index], sub[index]);
	if (FAILED(result))
	{
		OutputDebugString(_T("WICテクスチャの読み込み：失敗\n"));

		return result;
	}

	result = CreateHeap(index, dev);
	if (FAILED(result))
	{
		return result;
	}

	//シェーダリソースビュー設定用構造体の設定
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderDesc;
	SecureZeroMemory(&shaderDesc, sizeof(shaderDesc));
	shaderDesc.Format						= DXGI_FORMAT_R8G8B8A8_UNORM;
	shaderDesc.ViewDimension				= D3D12_SRV_DIMENSION_TEXTURE2D;
	shaderDesc.Texture2D.MipLevels			= 1;
	shaderDesc.Texture2D.MostDetailedMip	= 0;
	shaderDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//ヒープの先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle = data[index].heap->GetCPUDescriptorHandleForHeapStart();

	//シェーダーリソースビューの生成
	dev->CreateShaderResourceView(data[index].resource, &shaderDesc, handle);

	return result;
}

// ヒープの生成
HRESULT BMP::CreateHeap(USHORT index, ID3D12Device* dev)
{
	//ヒープ設定用構造体の設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask			= 0;
	heapDesc.NumDescriptors		= 2;
	heapDesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//ヒープ生成
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&data[index].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("テクスチャ用ヒープの生成：失敗\n"));

		return result;
	}

	return result;
}

// リソースの生成
HRESULT BMP::CreateResource(USHORT index, ID3D12Device * dev)
{
	//ヒープステート設定用構造体の設定
	D3D12_HEAP_PROPERTIES prop;
	SecureZeroMemory(&prop, sizeof(prop));
	prop.CPUPageProperty					= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	prop.CreationNodeMask					= 1;
	prop.MemoryPoolPreference				= D3D12_MEMORY_POOL_L0;
	prop.Type								= D3D12_HEAP_TYPE_CUSTOM;
	prop.VisibleNodeMask					= 1;

	//リソース設定用構造体の設定
	D3D12_RESOURCE_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.Dimension							= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width								= data[index].size.width;
	desc.Height								= data[index].size.height;
	desc.DepthOrArraySize					= 1;
	desc.Format								= DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count					= 1;
	desc.Flags								= D3D12_RESOURCE_FLAG_NONE;
	desc.Layout								= D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//リソース生成
	result = dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&data[index].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("テクスチャ用リソースの生成：失敗\n"));

		return result;
	}

	//シェーダリソースビュー設定用構造体の設定
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderDesc;
	SecureZeroMemory(&shaderDesc, sizeof(shaderDesc));
	shaderDesc.Format						= DXGI_FORMAT_R8G8B8A8_UNORM;
	shaderDesc.ViewDimension				= D3D12_SRV_DIMENSION_TEXTURE2D;
	shaderDesc.Texture2D.MipLevels			= 1;
	shaderDesc.Texture2D.MostDetailedMip	= 0;
	shaderDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//ヒープの先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle = data[index].heap->GetCPUDescriptorHandleForHeapStart();
	
	//シェーダーリソースビューの生成
	dev->CreateShaderResourceView(data[index].resource, &shaderDesc, handle);

	return result;
}

// 描画準備
void BMP::SetDraw(USHORT index, ID3D12GraphicsCommandList * list)
{
	//ボックス設定用構造体の設定
	D3D12_BOX box;
	SecureZeroMemory(&box, sizeof(box));
	box.back	= 1;
	box.bottom	= data[index].size.height;
	box.front	= 0;
	box.left	= 0;
	box.right	= data[index].size.width;
	box.top		= 0;

	//サブリソースに書き込み
	result = data[index].resource->WriteToSubresource(0, &box, &data[index].bmp[0], (box.right * 4), (box.bottom * 4));
	if (FAILED(result))
	{
		OutputDebugString(_T("サブリソースへの書き込み：失敗\n"));

		return;
	}

	//ヒープの先頭ハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE handle = data[index].heap->GetGPUDescriptorHandleForHeapStart();

	//ヒープのセット
	list->SetDescriptorHeaps(1, &data[index].heap);

	//ディスクラプターテーブルのセット
	list->SetGraphicsRootDescriptorTable(1, handle);
}

// WIC用描画準備
void BMP::SetDrawWIC(USHORT index, ID3D12GraphicsCommandList * list)
{
	//リソース設定用構造体
	D3D12_RESOURCE_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));

	desc = data[index].resource->GetDesc();

	//ボックス設定用構造体の設定
	D3D12_BOX box;
	SecureZeroMemory(&box, sizeof(box));
	box.back	= 1;
	box.bottom	= desc.Height;
	box.front	= 0;
	box.left	= 0;
	box.right	= (UINT)desc.Width;
	box.top		= 0;

	//サブリソースに書き込み
	result = data[index].resource->WriteToSubresource(0, &box, decode[index].get(), sub[index].RowPitch, sub[index].SlicePitch);
	if (FAILED(result))
	{
		OutputDebugString(_T("WICサブリソースへの書き込み：失敗\n"));

		return;
	}

	//ヒープの先頭ハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE handle = data[index].heap->GetGPUDescriptorHandleForHeapStart();

	//ヒープのセット
	list->SetDescriptorHeaps(1, &data[index].heap);

	//ディスクラプターテーブルのセット
	list->SetGraphicsRootDescriptorTable(1, handle);
}

// 描画
void BMP::Draw(USHORT index, ID3D12GraphicsCommandList * list)
{
	//トポロジー設定
	list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SetDraw(index, list);

	//描画
	list->DrawInstanced(6, 1, 0, 0);
}