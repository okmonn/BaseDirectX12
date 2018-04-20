#include "PMD.h"
#include "BMP.h"
#include <tchar.h>
#include<sstream>
#include <algorithm>

#pragma comment (lib,"d3d12.lib")
#pragma comment (lib,"dxgi.lib")

//インスタンス変数の初期化
PMD* PMD::s_Instance = nullptr;

// コンストラクタ
PMD::PMD()
{
	//参照結果の初期化
	result = S_OK;

	//送信データの初期化
	data = nullptr;

	//ボーン用送信データ
	bornData = nullptr;

	//マテリアル構造体の初期化
	SecureZeroMemory(&mat, sizeof(mat));

	//データ周りの構造体の初期化
	SecureZeroMemory(&view, sizeof(view));
	
	//ボーン用データ周りの構造体の初期化
	SecureZeroMemory(&bornView, sizeof(bornView));

	//PMDヘッダー構造体の初期化
	SecureZeroMemory(&header, sizeof(header));

	//PMD頂点データ構造体配列の初期化
	vertex.clear();

	//PMDインデックスデータ配列の初期化
	index.clear();

	//PMDマテリアルデータ配列の初期化
	material.clear();

	//PMDボーンデータ配列の初期化
	born.clear();

	//ボーン座標配列の初期化
	pos.clear();

	//ボーンノード配列の初期化
	node.clear();

	//ボーン名配列の初期化
	map.clear();

	//ボーン行列配列の初期化
	matrix.clear();

	//BMPクラスのインスタンス
	if (BMP::GetInstance() == nullptr)
	{
		BMP::Create();
	}


	//VMDヘッダー構造体の初期化
	SecureZeroMemory(&vmd_header, sizeof(vmd_header));

	//VMDモーション構造体配列の初期化
	motion.clear();

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
PMD::~PMD()
{
	//データ
	if (view.heap != nullptr)
	{
		view.heap->Release();
	}
	if (view.resource != nullptr)
	{
		//アンマップ
		view.resource->Unmap(0, nullptr);

		view.resource->Release();
	}

	//ボーン用データ
	if (bornView.heap != nullptr)
	{
		bornView.heap->Release();
	}
	if (bornView.resource != nullptr)
	{
		//アンマップ
		bornView.resource->Unmap(0, nullptr);

		bornView.resource->Release();
	}

	//BMPクラス
	if (BMP::GetInstance() != nullptr)
	{
		BMP::Destroy();
	}
}

// インスタンス
void PMD::Create(void)
{
	if (s_Instance == nullptr)
	{
		s_Instance = new PMD;
	}
}

// 破棄
void PMD::Destroy(void)
{
	if (s_Instance != nullptr)
	{
		delete s_Instance;
	}

	s_Instance = nullptr;
}

// フォルダーとの連結
std::string PMD::FolderPath(std::string path, const char * textureName)
{
	//ダミー宣言
	int pathIndex1 = path.rfind('/');
	int pathIndex2 = path.rfind('\\');
	int pathIndex = max(pathIndex1, pathIndex2);

	std::string folderPath = path.substr(0, pathIndex);
	folderPath += "/";
	folderPath += textureName;

	return folderPath;
}

// ユニコード変換
std::wstring PMD::ChangeUnicode(const CHAR * str)
{
	//文字数の取得
	auto byteSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str, -1, nullptr, 0);

	std::wstring wstr;
	wstr.resize(byteSize);

	//変換
	byteSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str, -1, &wstr[0], byteSize);

	return wstr;
}

// PMD読み込み
HRESULT PMD::LoadPMD(std::string fileName, ID3D12Device * dev)
{
	//ファイル
	FILE *file;

	//ダミー宣言
	const char* path = fileName.c_str();

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

	//ヘッダーの読み込み
	fread(&header, sizeof(header), 1, file);

	//頂点データ配列のメモリサイズ確保
	vertex.resize(header.vertexNum);

	//頂点データの読み込み
	for (auto& v : vertex)
	{
		fread(&v.pos,        sizeof(v.pos),        1, file);
		fread(&v.normal,     sizeof(v.normal),     1, file);
		fread(&v.uv,         sizeof(v.uv),         1, file);
		fread(&v.bornNum,    sizeof(v.bornNum),    1, file);
		fread(&v.bornWeight, sizeof(v.bornWeight), 1, file);
		fread(&v.edge,       sizeof(v.edge),       1, file);
	}

	//インデックス数格納用
	UINT indexNum = 0;

	//インデックス数の読み込み
	fread(&indexNum, sizeof(UINT), 1, file);

	//インデックスデータ配列のメモリサイズ確保
	index.resize(indexNum);

	for (UINT i = 0; i < indexNum; i++)
	{
		//インデックスデータの読み込み
		fread(&index[i], sizeof(USHORT), 1, file);
	}

	//マテリアル数格納用
	UINT materialNum = 0;

	//マテリアル数の読み込み
	fread(&materialNum, sizeof(UINT), 1, file);

	//マテリアルデータ配列のメモリサイズ確保
	material.resize(materialNum);

	//マテリアルデータの読み込み
	fread(&material[0], sizeof(Material), materialNum, file);

	//ボーン数格納用
	UINT bornNum = 0;

	//ボーン数の読み込み
	fread(&bornNum, sizeof(USHORT), 1, file);

	//ボーンデータ配列のメモリサイズ確保
	born.resize(bornNum);

	//ボーンデータの読み込み
	for (auto& b : born)
	{
		fread(&b.name,                 sizeof(b.name), 1, file);
		fread(&b.parent_born_index,    sizeof(b.parent_born_index),    1, file);
		fread(&b.tail_born_index,      sizeof(b.tail_born_index),      1, file);
		fread(&b.type,                 sizeof(b.type),                 1, file);
		fread(&b.ik_parent_born_index, sizeof(b.ik_parent_born_index), 1, file);
		fread(&b.pos,                  sizeof(b.pos),                  1, file);
	}

	//ファイルを閉じる
	fclose(file);


	//SetTexture(dev);
	SetTextureWIC(dev);

	SetBorn();

	result = CreateConstantBuffer(dev);
	result = LoadVMD("MikuMikuDance/UserFile/Motion/pose.vmd");

	return result;
}

// テクスチャ読み込み
void PMD::SetTexture(ID3D12Device* dev)
{
	for (UINT i = 0; i < material.size(); i++)
	{
		if (material[i].textureFilePath[0] != '\0')
		{
			//読み込み
			result = BMP::GetInstance()->LoadBMP(i, FolderPath("MikuMikuDance/UserFile/Model/", material[i].textureFilePath), dev);
			if (FAILED(result))
			{
				OutputDebugString(_T("テクスチャの読み込み：失敗\n"));
			}
		}
	}
}

// WICテクスチャ読み込み
void PMD::SetTextureWIC(ID3D12Device * dev)
{
	for (UINT i = 0; i < material.size(); i++)
	{
		if (material[i].textureFilePath[0] != '\0')
		{
			result = BMP::GetInstance()->LoadTextureWIC(i, ChangeUnicode(FolderPath("MikuMikuDance/UserFile/Model/", material[i].textureFilePath).c_str()), dev);
			if (FAILED(result))
			{
				OutputDebugString(_T("テクスチャの読み込み：失敗\n"));
			}
		}
	}
}

// ボーンのセット
void PMD::SetBorn(void)
{
	//ボーン座標配列のメモリサイズ確保
	pos.resize(born.size());

	//ボーンノード配列のメモリサイズ確保
	node.resize(born.size());

	//ボーン行列配列のメモリサイズ確保
	matrix.resize(born.size());

	//ボーン行列の全配列の初期化
	std::fill(matrix.begin(), matrix.end(), DirectX::XMMatrixIdentity());

	for (UINT i = 0; i < born.size(); i++)
	{
		//ボーン名格納
		map[born[i].name] = i;

		if (born[i].tail_born_index != 0)
		{
			//先頭座標格納
			pos[i].head = born[i].pos;
			//末尾座標格納
			pos[i].tail = born[born[i].tail_born_index].pos;
		}

		if (born[i].parent_born_index != 0xffff)
		{
			//参照ボーン格納
			node[born[i].parent_born_index].index.push_back(i);
		}
	}
}

// ボーンの回転
void PMD::BornRotation(std::string name, const DirectX::XMMATRIX matrix)
{
	//ボーンノード配列に何もないとき
	if (node.empty())
	{
		return;
	}

	//ダミー宣言
	DirectX::XMVECTOR head = DirectX::XMLoadFloat3(&pos[map[name]].head);
	DirectX::XMVECTOR tmp = DirectX::XMVectorSet(-pos[map[name]].head.x, -pos[map[name]].head.y, -pos[map[name]].head.z, 0);
	DirectX::XMVECTOR tail = DirectX::XMLoadFloat3(&pos[map[name]].tail);
	DirectX::XMMATRIX mat = DirectX::XMMatrixTranslationFromVector(tmp);

	mat *= matrix;
	mat *= DirectX::XMMatrixTranslationFromVector(head);

	tail = DirectX::XMVector3Transform(tail, mat);

	DirectX::XMStoreFloat3(&pos[map[name]].tail, tail);

	this->matrix[map[name]] = mat;

	for (auto &child : node[map[name]].index)
	{
		MoveBorn(child, mat);
	}

	//コピー
	memcpy(bornData, &this->matrix[0], (sizeof(DirectX::XMMATRIX) * this->matrix.size() + 0xff) &~0xff);
}

// 子ボーンの回転
void PMD::MoveBorn(USHORT index, const DirectX::XMMATRIX & matrix)
{
	//ボーンノード配列に何もないとき
	if (node.empty() || index == 0)
	{
		return;
	}

	DirectX::XMVECTOR head = DirectX::XMLoadFloat3(&pos[index].head);
	head = DirectX::XMVector3Transform(head, matrix);
	DirectX::XMStoreFloat3(&pos[index].head, head);

	DirectX::XMVECTOR tail = DirectX::XMLoadFloat3(&pos[index].tail);
	tail = DirectX::XMVector3Transform(tail, matrix);
	DirectX::XMStoreFloat3(&pos[index].tail, tail);

	this->matrix[index] = matrix;

	for (auto child : node[index].index)
	{
		MoveBorn(child, matrix);
	}
}

// 定数バッファの生成
HRESULT PMD::CreateConstantBuffer(ID3D12Device * dev)
{
	//定数バッファ設定用構造体の設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.NumDescriptors			= 2 + material.size();
	heapDesc.Flags					= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type					= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//ヒープ生成
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&view.heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("モデル用ヒープの生成：失敗\n"));

		return result;
	}

	//ヒープサイズを取得
	view.size = dev->GetDescriptorHandleIncrementSize(heapDesc.Type);

	//ヒープ設定用構造体の設定
	D3D12_HEAP_PROPERTIES prop;
	SecureZeroMemory(&prop, sizeof(prop));
	prop.Type						= D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference		= D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask			= 1;
	prop.VisibleNodeMask			= 1;

	//リソース設定用構造体の設定
	D3D12_RESOURCE_DESC resourceDesc;
	SecureZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width				= ((sizeof(Mat) + 0xff) &~0xff) * ((sizeof(Mat) + 0xff) &~0xff);
	resourceDesc.Height				= 1;
	resourceDesc.DepthOrArraySize	= 1;
	resourceDesc.MipLevels			= 1;
	resourceDesc.Format				= DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count	= 1;
	resourceDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//リソース生成
	result = dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&view.resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("モデル用リソースの生成：失敗\n"));

		return result;
	}

	//定数バッファビュー設定用構造体の設定
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.SizeInBytes				= (sizeof(Mat) + 0xff) &~0xff;

	auto address = view.resource->GetGPUVirtualAddress();
	auto handle = view.heap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < material.size(); i++)
	{
		desc.BufferLocation			= address;

		//定数バッファビュー生成
		dev->CreateConstantBufferView(&desc, handle);

		address += desc.SizeInBytes;

		handle.ptr += view.size;
	}

	//送信範囲
	D3D12_RANGE range = { 0, 0 };

	//マッピング
	result = view.resource->Map(0, &range, (void**)(&data));
	if (FAILED(result))
	{
		OutputDebugString(_T("モデル用リソースのマッピング：失敗\n"));

		return result;
	}

	//コピー
	memcpy(data, &mat, sizeof(DirectX::XMMATRIX));


	//ボーン
	//定数バッファ設定用構造体の設定
	heapDesc.NumDescriptors			= 2;
	heapDesc.Flags					= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type					= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//ヒープ生成
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&bornView.heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("ボーン用ヒープの生成：失敗\n"));

		return result;
	}

	//ヒープサイズを取得
	bornView.size = dev->GetDescriptorHandleIncrementSize(heapDesc.Type);

	//ヒープ設定用構造体の設定
	prop.Type						= D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference		= D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask			= 1;
	prop.VisibleNodeMask			= 1;

	//リソース設定用構造体の設定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width				= ((sizeof(DirectX::XMMATRIX) * matrix.size() + 0xff) &~0xff);
	resourceDesc.Height				= 1;
	resourceDesc.DepthOrArraySize	= 1;
	resourceDesc.MipLevels			= 1;
	resourceDesc.Format				= DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count	= 1;
	resourceDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//リソース生成
	result = dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&bornView.resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("ボーン用リソースの生成：失敗\n"));

		return result;
	}

	//定数バッファビュー設定用構造体の設定
	desc.BufferLocation				= bornView.resource->GetGPUVirtualAddress();
	desc.SizeInBytes				= (sizeof(DirectX::XMMATRIX) * matrix.size() + 0xff) &~0xff;

	//定数バッファビュー生成
	dev->CreateConstantBufferView(&desc, bornView.heap->GetCPUDescriptorHandleForHeapStart());

	//マッピング
	result = bornView.resource->Map(0, &range, (void**)(&bornData));
	if (FAILED(result))
	{
		OutputDebugString(_T("ボーン用リソースのマッピング：失敗\n"));

		return result;
	}

	//コピー
	memcpy(bornData, &matrix[0], (sizeof(DirectX::XMMATRIX) * matrix.size() + 0xff) &~0xff);

	return result;
}

// 描画
void PMD::Draw(ID3D12GraphicsCommandList * list, D3D12_INDEX_BUFFER_VIEW indexView)
{
	//ヒープのセット
	list->SetDescriptorHeaps(1, &bornView.heap);

	//ディスクラプターテーブルのセット
	list->SetGraphicsRootDescriptorTable(3, bornView.heap->GetGPUDescriptorHandleForHeapStart());

	//頂点インデックスビューのセット
	list->IASetIndexBuffer(&indexView);

	//トポロジー設定
	list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//描画
	//オフセット
	UINT offset = 0;

	//送信用データ
	UINT8* dummy = data;

	//ヒープの先頭ハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE handle = view.heap->GetGPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < material.size(); i++)
	{
		//基本色格納
		mat.diffuse = material[i].diffuse;

		//テクスチャ対応チェック
		mat.existTexture = (material[i].textureFilePath[0] != '\0');

		if (mat.existTexture)
		{
			//テクスチャ適応
			//BMP::GetInstance()->SetDraw(i, list);
			BMP::GetInstance()->SetDrawWIC(i, list);
		}

		//ヒープのセット
		list->SetDescriptorHeaps(1, &view.heap);

		//ディスクラプターテーブルのセット
		list->SetGraphicsRootDescriptorTable(2, handle);

		//コピー
		memcpy(dummy, &mat, sizeof(Mat));

		//描画
		list->DrawIndexedInstanced(material[i].indexNum, 1, offset, 0, 0);

		//ハンドル更新
		handle.ptr += view.size;

		//データ更新
		dummy = (UINT8*)(((sizeof(Mat) + 0xff) &~0xff) + (CHAR*)(dummy));

		//オフセット更新
		offset += material[i].indexNum;
	}
}

// 頂点データの取得
std::vector<VERETX> PMD::GetVertex(void)
{
	return vertex;
}

// 頂点インデックスデータの取得
std::vector<USHORT> PMD::GetIndex(void)
{
	return index;
}

// VMD読み込み
HRESULT PMD::LoadVMD(std::string fileName)
{
	//ファイル
	FILE *file;

	//ダミー宣言
	const char* path = fileName.c_str();

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

	//ヘッダーの読み込み
	fread(&vmd_header, sizeof(vmd_header), 1, file);

	//モーションデータ数格納用
	DWORD motionNum = 0;

	//モーションデータ数の読み込み
	fread(&motionNum, sizeof(DWORD), 1, file);

	//モーションデータ配列のメモリ確保
	motion.resize(motionNum);

	for (auto& m : motion)
	{
		fread(&m.bornName, sizeof(m.bornName), 1, file);
		fread(&m.frameNo, sizeof(m.frameNo), 1, file);
		fread(&m.location, sizeof(m.location), 1, file);
		fread(&m.rotatation, sizeof(m.rotatation), 1, file);
		fread(&m.interpolation, sizeof(m.interpolation), 1, file);
	}

	//ファイルを閉じる
	fclose(file);

	//Motion();

	return S_OK;
}

// モーション
void PMD::Motion(void)
{
	for (auto& pose : motion)
	{
		DirectX::XMVECTOR vec = DirectX::XMLoadFloat4(&pose.rotatation);
		Motion(pose.bornName, vec);
	}
}

void PMD::Motion(std::string name, DirectX::XMVECTOR quaternion)
{
	//ダミー宣言
	DirectX::XMVECTOR head = DirectX::XMLoadFloat3(&pos[map[name]].head);
	DirectX::XMVECTOR tmp = DirectX::XMVectorSet(-pos[map[name]].head.x, -pos[map[name]].head.y, -pos[map[name]].head.z, 0);
	DirectX::XMVECTOR tail = DirectX::XMLoadFloat3(&pos[map[name]].tail);
	DirectX::XMMATRIX mat = DirectX::XMMatrixTranslationFromVector(tmp);

	mat *= DirectX::XMMatrixRotationQuaternion(quaternion);
	mat *= DirectX::XMMatrixTranslationFromVector(head);

	tail = DirectX::XMVector3Transform(tail, mat);

	DirectX::XMStoreFloat3(&pos[map[name]].tail, tail);

	this->matrix[map[name]] = mat;

	for (auto &child : node[map[name]].index)
	{
		MoveBorn(child, mat);
	}

	//コピー
	memcpy(bornData, &this->matrix[0], (sizeof(DirectX::XMMATRIX) * this->matrix.size() + 0xff) &~0xff);
}

void PMD::Motion(USHORT index, DirectX::XMMATRIX matrix)
{
	DirectX::XMVECTOR head = DirectX::XMLoadFloat3(&pos[index].head);
	head = DirectX::XMVector3Transform(head, matrix);
	DirectX::XMStoreFloat3(&pos[index].head, head);

	DirectX::XMVECTOR tail = DirectX::XMLoadFloat3(&pos[index].tail);
	tail = DirectX::XMVector3Transform(tail, matrix);
	DirectX::XMStoreFloat3(&pos[index].tail, tail);

	this->matrix[index] = matrix;

	for (auto child : node[index].index)
	{
		MoveBorn(child, matrix);
	}
}
