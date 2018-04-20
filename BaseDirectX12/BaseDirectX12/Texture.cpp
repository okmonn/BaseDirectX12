#include "Texture.h"
#include "d3dx12.h"
#include<sstream>
#include <tchar.h>

#pragma comment (lib,"d3d12.lib")
#pragma comment (lib,"dxgi.lib")

//インスタンス変数の初期化
Texture* Texture::s_Instance = nullptr;

// コンストラクタ
Texture::Texture()
{
	//参照結果
	result = S_OK;

	//データの初期化
	data.clear();

	//頂点バッファビュー構造体の初期化
	SecureZeroMemory(&view, sizeof(view));


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
Texture::~Texture()
{
	//データ
	for (auto itr = data.begin(); itr != data.end(); ++itr)
	{
		if (itr->second.texture->resorce != nullptr)
		{
			itr->second.texture->resorce->Release();
		}
		if (itr->second.texture->heap != nullptr)
		{
			itr->second.texture->heap->Release();
		}
	}
}

// インスタンス化
void Texture::Create(void)
{
	if (s_Instance == nullptr)
	{
		s_Instance = new Texture;
	}
}

// 破棄
void Texture::Destroy(void)
{
	if (s_Instance != nullptr)
	{
		delete s_Instance;
	}

	s_Instance = nullptr;
}

// 画像データの読み込み
HRESULT Texture::LoadBMP(USHORT index, std::string fileName, UINT swapBufferNum, ID3D12Device* dev)
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
	data[index].size = { header.biWidth, header.biHeight };

	//データサイズ分のメモリ確保(ビットの深さが32bitの場合)
	//bmp.resize(header.biSizeImage);
	//データサイズ分のメモリ確保(ビットの深さが24bitの場合)
	data[index].bmp.resize(header.biWidth * header.biHeight * 4);

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

	if (CreateVertexHeap(index, swapBufferNum, dev) == S_OK)
	{
		result = CreateVertexBuffer(index, dev);
	}

	if (CreateConstantHeap(index, dev) == S_OK)
	{
		result = CreateConstantBuffer(index, dev);
	}

	return result;
}

// 頂点ヒープの生成
HRESULT Texture::CreateVertexHeap(USHORT index, UINT swapBufferNum, ID3D12Device* dev)
{
	//ヒープ設定用構造体の設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = swapBufferNum;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	//レンダーターゲット(頂点)用ヒープ生成
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&data[index].texture[0].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("頂点用ヒープの生成：失敗\n"));

		return result;
	}

	return result;
}

// 頂点バッファの生成
HRESULT Texture::CreateVertexBuffer(USHORT index, ID3D12Device* dev)
{
	//配列のメモリ確保
	data[index].vertex.resize(6);

	//頂点用リソース生成
	result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * data[index].vertex.size()), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&data[index].texture[0].resorce));
	if (FAILED(result))
	{
		OutputDebugString(_T("頂点バッファ用リソースの生成：失敗\n"));

		return result;
	}

	//送信用データ
	

	//送信範囲
	D3D12_RANGE range = { 0,0 };

	//マッピング
	result = data[index].texture[0].resorce->Map(0, &range, reinterpret_cast<void**>(&d));
	if (FAILED(result))
	{
		OutputDebugString(_T("頂点用リソースのマッピング：失敗\n"));

		return result;
	}

	//頂点データのコピー
	memcpy(d, &data[index].vertex, (sizeof(Vertex) * data[index].vertex.size()));

	//アンマッピング
	data[index].texture[0].resorce->Unmap(0, nullptr);

	//頂点バッファ設定用構造体の設定
	view.BufferLocation = data[index].texture[0].resorce->GetGPUVirtualAddress();
	view.SizeInBytes = sizeof(Vertex) * data[index].vertex.size();
	view.StrideInBytes = sizeof(Vertex);

	return result;
}

// 定数ヒープの生成
HRESULT Texture::CreateConstantHeap(USHORT index, ID3D12Device * dev)
{//ヒープ設定用構造体の設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//ヒープ生成
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&data[index].texture[1].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("テクスチャ用ヒープの生成：失敗\n"));

		return result;
	}

	return result;
}

// 定数バッファの生成
HRESULT Texture::CreateConstantBuffer(USHORT index, ID3D12Device * dev)
{
	//ヒープステート設定用構造体の設定
	D3D12_HEAP_PROPERTIES prop;
	SecureZeroMemory(&prop, sizeof(prop));
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	prop.CreationNodeMask = 1;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	prop.Type = D3D12_HEAP_TYPE_CUSTOM;
	prop.VisibleNodeMask = 1;

	//リソース設定用構造体の設定
	D3D12_RESOURCE_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = data[index].size.width;
	desc.Height = data[index].size.height;
	desc.DepthOrArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//リソース生成
	result = dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&data[index].texture[1].resorce));
	if (FAILED(result))
	{
		OutputDebugString(_T("テクスチャ用リソースの生成：失敗\n"));

		return result;
	}

	//シェーダリソースビュー設定用構造体の設定
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderDesc;
	SecureZeroMemory(&shaderDesc, sizeof(shaderDesc));
	shaderDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	shaderDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	shaderDesc.Texture2D.MipLevels = 1;
	shaderDesc.Texture2D.MostDetailedMip = 0;
	shaderDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//ヒープの先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle = data[index].texture[1].heap->GetCPUDescriptorHandleForHeapStart();

	//シェーダーリソースビューの生成
	dev->CreateShaderResourceView(data[index].texture[1].resorce, &shaderDesc, handle);

	return result;
}

// 頂点設定
HRESULT Texture::SetVertex(USHORT index, Vector2 pos, Vector2 size, Vector2 wSize)
{
	//頂点設定
	data[index].vertex[0] = { { -(pos.x / (wSize.x / 2)),             (pos.y / (wSize.y / 2)),	          0.0f },{ 0.0f, 0.0f } };	//左上
	data[index].vertex[1] = { {  ((pos.x + size.x) / (wSize.x )),     (pos.y / (wSize.y / 2)),	          0.0f },{ 1.0f, 0.0f } };	//右上
	data[index].vertex[2] = { {  ((pos.x + size.x) / (wSize.x )),    -((pos.y + size.y) / (wSize.y )), 0.0f },{ 1.0f, 1.0f } };	//右下
	data[index].vertex[3] = { {  ((pos.x + size.x) / (wSize.x )),    -((pos.y + size.y) / (wSize.y )), 0.0f },{ 1.0f, 1.0f } };	//右下
	data[index].vertex[4] = { { -(pos.x / (wSize.x / 2))           , -((pos.y + size.y) / (wSize.y)), 0.0f },{ 0.0f, 1.0f } };	//左下
	data[index].vertex[5] = { { -(pos.x / (wSize.x / 2))           ,  (pos.y / (wSize.y / 2)),	          0.0f },{ 0.0f, 0.0f } };  //左上

	//送信用データ
	

	//送信範囲
	D3D12_RANGE range = { 0,0 };

	//マッピング
	result = data[index].texture[0].resorce->Map(0, &range, reinterpret_cast<void**>(&d));
	if (FAILED(result))
	{
		OutputDebugString(_T("頂点用リソースのマッピング：失敗\n"));

		return result;
	}

	//頂点データのコピー
	memcpy(d, &data[index].vertex, (sizeof(Vertex) * data[index].vertex.size()));

	//アンマッピング
	data[index].texture[0].resorce->Unmap(0, nullptr);

	//頂点バッファ設定用構造体の設定
	view.BufferLocation = data[index].texture[0].resorce->GetGPUVirtualAddress();
	view.SizeInBytes = sizeof(Vertex) * data[index].vertex.size();
	view.StrideInBytes = sizeof(Vertex);

	return result;
}

// 描画
HRESULT Texture::Draw(USHORT index, Vector2 pos, Vector2 size, Vector2 wSize, ID3D12GraphicsCommandList * list)
{
	result = SetVertex(index, pos, size, wSize);
	if (FAILED(result))
	{
		OutputDebugString(_T("頂点情報のセット：失敗\n"));

		return result;
	}

	//頂点バッファビューのセット
	list->IASetVertexBuffers(0, 1, &view);

	//トポロジー設定
	list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//ボックス設定用構造体の設定
	D3D12_BOX box;
	SecureZeroMemory(&box, sizeof(box));
	box.back = 1;
	box.bottom = data[index].size.height;
	box.front = 0;
	box.left = 0;
	box.right = data[index].size.width;
	box.top = 0;

	//サブリソースに書き込み
	result = data[index].texture[1].resorce->WriteToSubresource(0, &box, &data[index].bmp[0], (box.right * 4), (box.bottom * 4));
	if (FAILED(result))
	{
		OutputDebugString(_T("サブリソースへの書き込み：失敗\n"));

		return result;
	}

	//ヒープの先頭ハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE handle = data[index].texture[1].heap->GetGPUDescriptorHandleForHeapStart();

	//ヒープのセット
	list->SetDescriptorHeaps(1, &data[index].texture[1].heap);

	//ディスクラプターテーブルのセット
	list->SetGraphicsRootDescriptorTable(1, handle);

	//描画
	list->DrawInstanced(6, 1, 0, 0);

	return result;
}
