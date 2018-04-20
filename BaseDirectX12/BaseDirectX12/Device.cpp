#include "Device.h"
#include "BMP.h"
#include "PMD.h"
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <tchar.h>
#include<sstream>


#pragma comment (lib,"d3d12.lib")
#pragma comment (lib,"dxgi.lib")
#pragma comment (lib,"d3dcompiler.lib")


// 機能レベルの一覧
D3D_FEATURE_LEVEL levels[] =
{
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_0,
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0
};

// コンストラクタ
Device::Device(std::weak_ptr<Window>winAdr, std::weak_ptr<Input>inputAdr)
{
	//ウィンドウクラス参照
	win = winAdr;

	//インプットクラス参照
	input = inputAdr;

	//機能レベルの初期化
	level = D3D_FEATURE_LEVEL_11_0;

	//参照結果の初期化
	result = S_OK;

	//回転角度の初期化
	for (UINT i = 0; i < 2; i++)
	{
		angle[i] = 0.0f;
	}

	//行列データ
	data = nullptr;

	//コマンド周りの構造体の初期化
	SecureZeroMemory(&command, sizeof(command));

	//スワップチェイン周りの構造体の初期化
	SecureZeroMemory(&swap, sizeof(swap));

	//フェンス周りの構造体の初期化
	SecureZeroMemory(&fence, sizeof(fence));

	//ルートシグネチャ周りの構造体の初期化
	SecureZeroMemory(&rootsignature, sizeof(rootsignature));

	//パイプライン周りの構造体の初期化
	SecureZeroMemory(&pipeline, sizeof(pipeline));

	//頂点バッファ設定用構造体の初期化
	SecureZeroMemory(&vertexView, sizeof(vertexView));

	//頂点インデックスビュー設定用構造体の初期化
	SecureZeroMemory(&indexView, sizeof(indexView));

	//WVP設定用構造体の初期化
	SecureZeroMemory(&wvp, sizeof(wvp));

	//データ周りの構造体配列の初期化
	view.clear();

	//レンダーターゲット用リソース配列の初期化
	renderTarget.clear();


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


	Init();

}

// デストラクタ
Device::~Device()
{
	//定数バッファのアンマップ
	view[mode[2]].resource->Unmap(0, nullptr);

	//デバイス
	if (command.dev != nullptr)
	{
		command.dev->Release();
	}

	//コマンドアロケータ
	if (command.allocator != nullptr)
	{
		command.allocator->Release();
	}

	//コマンドリスト
	if (command.list != nullptr)
	{
		command.list->Release();
	}

	//コマンドキュー
	if (command.queue != nullptr)
	{
		command.queue->Release();
	}

	//インターフェースファクトリー
	if (swap.factory != nullptr)
	{
		swap.factory->Release();
	}

	//スワップチェイン
	if (swap.swapChain != nullptr)
	{
		swap.swapChain->Release();
	}

	//フェンス
	if (fence.fence != nullptr)
	{
		fence.fence->Release();
	}

	//シリアライズメッセージ
	if (rootsignature.signature != nullptr)
	{
		rootsignature.signature->Release();
	}

	//シリアライズエラーメッセージ
	if (rootsignature.error != nullptr)
	{
		rootsignature.error->Release();
	}

	//ルートシグネチャ
	if (rootsignature.rootSignature != nullptr)
	{
		rootsignature.rootSignature->Release();
	}

	//頂点情報
	if (pipeline.vertex != nullptr)
	{
		pipeline.vertex->Release();
	}

	//ピクセル情報
	if (pipeline.pixel != nullptr)
	{
		pipeline.pixel->Release();
	}

	//パイプライン
	if (pipeline.pipeline != nullptr)
	{
		pipeline.pipeline->Release();
	}

	//ビュー
	for (auto itr = view.begin(); itr != view.end(); ++itr)
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

	//レンダーターゲット
	for (UINT i = 0; i < renderTarget.size(); i++)
	{
		if (renderTarget[i] != nullptr)
		{
			renderTarget[i]->Release();
		}
	}

	//PMDクラス
	if (PMD::GetInstance() != nullptr)
	{
		PMD::Destroy();
	}

	//BMPクラス
	if (BMP::GetInstance() != nullptr)
	{
		BMP::Destroy();
	}
}

// ビュー行列のセット
DirectX::XMMATRIX Device::SetViewMatrix(void)
{
	//ダミー宣言
	FLOAT pos = .0f;
	DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
	//カメラの位置
	DirectX::XMVECTOR eye		= { 0, pos,  -1 };
	//カメラの焦点
	DirectX::XMVECTOR target	= { 0, pos,   0 };
	//カメラの上方向
	DirectX::XMVECTOR upper		= { 0, 1,     0 };

	view = DirectX::XMMatrixLookAtLH(eye, target, upper);

	return view;
}

// プロジェクション行列のセット
DirectX::XMMATRIX Device::SetProjectionMatrix(void)
{
	//ダミー宣言
	DirectX::XMMATRIX projection = DirectX::XMMatrixIdentity();

	projection = DirectX::XMMatrixPerspectiveFovLH(RAD(90), ((static_cast<FLOAT>(WINDOW_X) / static_cast<FLOAT>(WINDOW_Y))), 0.5f, 500.0f);
	
	return projection;
}

// WVPのセット
void Device::SetWVP(void)
{
	wvp.world = DirectX::XMMatrixIdentity();
	wvp.viewProjection = SetViewMatrix() * SetProjectionMatrix();
}

// デバイスの生成
HRESULT Device::CreateDevice(void)
{
	for (auto& i : levels)
	{
		//デバイス生成
		result = D3D12CreateDevice(nullptr, i, IID_PPV_ARGS(&command.dev));
		if (result == S_OK)
		{
			//機能レベルの決定
			level = i;
			break;
		}
	}

	return result;
}

// コマンド周りの生成
HRESULT Device::CreateCommand(void)
{
	if (CreateDevice() == S_OK)
	{
		//コマンドアロケータ生成
		result = command.dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command.allocator));
		if (FAILED(result))
		{
			OutputDebugString(_T("コマンドアロケータの生成：失敗\n"));

			return result;
		}

		//コマンドリスト生成
		result = command.dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command.allocator, nullptr, IID_PPV_ARGS(&command.list));
		if (FAILED(result))
		{
			OutputDebugString(_T("コマンドリストの生成：失敗\n"));

			return result;
		}

		//コマンドリストを閉じる
		command.list->Close();

		//コマンドキュー設定用構造体の設定
		D3D12_COMMAND_QUEUE_DESC desc;
		SecureZeroMemory(&desc, sizeof(desc));
		desc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask	= 0;
		desc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Type		= D3D12_COMMAND_LIST_TYPE_DIRECT;

		//コマンドキュー生成
		result = command.dev->CreateCommandQueue(&desc, IID_PPV_ARGS(&command.queue));
		if (FAILED(result))
		{
			OutputDebugString(_T("コマンドキューの生成：失敗\n"));

			return result;
		}
	}
	else
	{
		OutputDebugString(_T("デバイスの生成：失敗\n"));

		return S_FALSE;
	}
	
	return result;
}

// インターフェースファクトリーの生成
HRESULT Device::CreateFactory(void)
{
	//インターフェースファクトリー生成
	result = CreateDXGIFactory1(IID_PPV_ARGS(&swap.factory));

	return result;
}

// スワップチェインの生成
HRESULT Device::CreateSwapChain(void)
{
	if (CreateFactory() == S_OK)
	{
		//スワップチェイン設定用構造体の設定
		DXGI_SWAP_CHAIN_DESC1 desc;
		SecureZeroMemory(&desc, sizeof(desc));
		desc.AlphaMode		= DXGI_ALPHA_MODE_UNSPECIFIED;
		desc.BufferCount	= 2;
		desc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.Flags			= 0;
		desc.Format			= DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Height			= WINDOW_Y;
		desc.SampleDesc		= { 1, 0 };
		desc.Scaling		= DXGI_SCALING_STRETCH;
		desc.Stereo			= false;
		desc.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.Width			= WINDOW_X;

		//スワップチェイン生成
		result = swap.factory->CreateSwapChainForHwnd(command.queue, win.lock()->GetWindowHandle(), &desc, nullptr, nullptr, (IDXGISwapChain1**)(&swap.swapChain));
		if (FAILED(result))
		{
			OutputDebugString(_T("スワップチェインの生成：失敗\n"));

			return result;
		}

		//バックバッファ数保存
		swap.bufferCnt = desc.BufferCount;
	}
	else
	{
		OutputDebugString(_T("ファクトリーの生成：失敗\n"));

		return S_FALSE;
	}

	return result;
}

// レンダーターゲットの生成
HRESULT Device::CreateRenderTargetView(void)
{
	//ヒープ設定用構造体の設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask					= 0;
	heapDesc.NumDescriptors				= swap.bufferCnt;
	heapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	//レンダーターゲット(頂点)用ヒープ生成
	result = command.dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&view[mode[0]].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("レンダーターゲット用ヒープの生成：失敗\n"));

		return result;
	}

	//ヒープサイズを取得
	view[mode[0]].size = command.dev->GetDescriptorHandleIncrementSize(heapDesc.Type);

	//レンダーターゲット(頂点)用ヒープの先頭を取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle = view[mode[0]].heap->GetCPUDescriptorHandleForHeapStart();

	//レンダーターゲット設定用構造体の設定
	D3D12_RENDER_TARGET_VIEW_DESC renderDesc;
	SecureZeroMemory(&renderDesc, sizeof(renderDesc));
	renderDesc.Format					= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	renderDesc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
	renderDesc.Texture2D.MipSlice		= 0;
	renderDesc.Texture2D.PlaneSlice		= 0;

	//レンダーターゲット分のメモリ確保
	renderTarget.resize(swap.bufferCnt);

	for (UINT i = 0; i < renderTarget.size(); i++)
	{
		//バッファの取得
		result = swap.swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTarget[i]));
		if (FAILED(result))
		{
			continue;
		}

		//レンダーターゲット生成
		command.dev->CreateRenderTargetView(renderTarget[i], &renderDesc, handle);

		//ハンドルの位置を移動
		handle.ptr += view[mode[0]].size;
	}

	return result;
}

// 深度ステンシルの生成
HRESULT Device::CreateDepthStencil(void)
{
	//ヒープ設定用構造体の設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask					= 0;
	heapDesc.NumDescriptors				= swap.bufferCnt;
	heapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	//深度ステンシル用ヒープ生成
	result = command.dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&view[mode[1]].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("深度ステンシル用ヒープの生成：失敗\n"));

		return result;
	}

	//ヒープサイズを取得
	view[mode[1]].size = command.dev->GetDescriptorHandleIncrementSize(heapDesc.Type);

	//ヒーププロパティ設定用構造体の設定
	D3D12_HEAP_PROPERTIES prop;
	SecureZeroMemory(&prop, sizeof(prop));
	prop.CPUPageProperty				= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.CreationNodeMask				= 1;
	prop.MemoryPoolPreference			= D3D12_MEMORY_POOL_UNKNOWN;
	prop.Type							= D3D12_HEAP_TYPE_DEFAULT;
	prop.VisibleNodeMask				= 1;

	//リソース設定用構造体の設定
	D3D12_RESOURCE_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.Dimension						= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment						= 0;
	desc.Width							= WINDOW_X;
	desc.Height							= WINDOW_Y;
	desc.DepthOrArraySize				= 1;
	desc.MipLevels						= 0;
	desc.Format							= DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count				= 1;
	desc.SampleDesc.Quality				= 0;
	desc.Flags							= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	desc.Layout							= D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//クリア値設定用構造体の設定
	D3D12_CLEAR_VALUE clearValue;
	SecureZeroMemory(&clearValue, sizeof(clearValue));
	clearValue.Format					= DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth		= 1.0f;
	clearValue.DepthStencil.Stencil		= 0;

	//深度ステンシル用リソース生成
	result = command.dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&view[mode[1]].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("深度ステンシル用リソースの生成：失敗\n"));

		return result;
	}

	//深度ステンシルビュー設定用構造体の設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	SecureZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format						= DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension				= D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags						= D3D12_DSV_FLAG_NONE;

	//深度ステンシルビュー生成
	command.dev->CreateDepthStencilView(view[mode[1]].resource,&dsvDesc, view[mode[1]].heap->GetCPUDescriptorHandleForHeapStart());

	return result;
}

// フェンスの生成
HRESULT Device::CreateFence(void)
{
	if (command.dev == nullptr)
	{
		return S_FALSE;
	}

	//フェンス生成
	result = command.dev->CreateFence(fence.fenceCnt, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence.fence));
	if (FAILED(result))
	{
		OutputDebugString(_T("フェンスの生成：失敗\n"));

		return result;
	}

	//フェンス値の更新
	fence.fenceCnt = 1;

	//フェンスイベント生成
	fence.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (fence.fenceEvent == nullptr)
	{
		OutputDebugString(_T("フェンスイベントの生成：失敗\n"));

		return S_FALSE;
	}

	return result;
}

// ルートシグネチャのシリアライズ
HRESULT Device::SerializeRootSignature(void)
{
	// ディスクリプタレンジの設定.
	D3D12_DESCRIPTOR_RANGE range[4];
	SecureZeroMemory(&range, sizeof(range));

	//ルートパラメータの設定.
	D3D12_ROOT_PARAMETER param[4];
	SecureZeroMemory(&param, sizeof(param));

	//定数バッファ用
	range[0].RangeType								= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[0].NumDescriptors							= 1;
	range[0].BaseShaderRegister						= 0;
	range[0].RegisterSpace							= 0;
	range[0].OffsetInDescriptorsFromTableStart		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	param[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[0].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	param[0].DescriptorTable.NumDescriptorRanges	= 1;
	param[0].DescriptorTable.pDescriptorRanges		= &range[0];

	//テクスチャ用
	range[1].RangeType								= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[1].NumDescriptors							= 1;
	range[1].BaseShaderRegister						= 0;
	range[1].RegisterSpace							= 0;
	range[1].OffsetInDescriptorsFromTableStart		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	param[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;
	param[1].DescriptorTable.NumDescriptorRanges	= 1;
	param[1].DescriptorTable.pDescriptorRanges		= &range[1];

	//マテリアル用
	range[2].RangeType								= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[2].NumDescriptors							= 1;
	range[2].BaseShaderRegister						= 1;
	range[2].RegisterSpace							= 0;
	range[2].OffsetInDescriptorsFromTableStart		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	param[2].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[2].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	param[2].DescriptorTable.NumDescriptorRanges	= 1;
	param[2].DescriptorTable.pDescriptorRanges		= &range[2];

	//ボーン用
	range[3].RangeType								= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[3].NumDescriptors							= 1;
	range[3].BaseShaderRegister						= 2;
	range[3].RegisterSpace							= 0;
	range[3].OffsetInDescriptorsFromTableStart		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	param[3].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[3].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	param[3].DescriptorTable.NumDescriptorRanges	= 1;
	param[3].DescriptorTable.pDescriptorRanges		= &range[3];

	//静的サンプラーの設定
	D3D12_STATIC_SAMPLER_DESC sampler;
	SecureZeroMemory(&sampler, sizeof(sampler));
	sampler.Filter									= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU								= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV								= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW								= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias								= 0;
	sampler.MaxAnisotropy							= 0;
	sampler.ComparisonFunc							= D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor								= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD									= 0.0f;
	sampler.MaxLOD									= D3D12_FLOAT32_MAX;
	sampler.ShaderRegister							= 0;
	sampler.RegisterSpace							= 0;
	sampler.ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;

	//ルートシグネチャ設定用構造体の設定
	D3D12_ROOT_SIGNATURE_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.NumParameters								= _countof(param);
	desc.pParameters								= param;
	desc.NumStaticSamplers							= 1;
	desc.pStaticSamplers							= &sampler;
	desc.Flags										= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//ルートシグネチャのシリアライズ化
	result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootsignature.signature, &rootsignature.error);
	
	return result;
}

// ルートシグネチャの生成
HRESULT Device::CreateRootSignature(void)
{
	if (SerializeRootSignature() == S_OK)
	{
		//ルートシグネチャ生成
		result = command.dev->CreateRootSignature(0, rootsignature.signature->GetBufferPointer(), rootsignature.signature->GetBufferSize(), IID_PPV_ARGS(&rootsignature.rootSignature));
		if (FAILED(result))
		{
			OutputDebugString(_T("ルートシグネチャの生成：失敗\n"));

			return result;
		}
	}
	else
	{
		OutputDebugString(_T("シリアライズ化：失敗\n"));

		return S_FALSE;
	}

	return result;
}

// テクスチャ用シェーダーコンパイル
HRESULT Device::ShaderCompileTexture(void)
{
	//頂点シェーダのコンパイル
	result = D3DCompileFromFile(_T("shader.hlsl"), nullptr, nullptr, "TextureVS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pipeline.vertex, &rootsignature.error);
	if (FAILED(result))
	{
		OutputDebugString(_T("頂点シェーダコンパイル：失敗\n"));

		return result;
	}

	//ピクセルシェーダのコンパイル
	result = D3DCompileFromFile(_T("shader.hlsl"), nullptr, nullptr, "TexturePS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pipeline.pixel, &rootsignature.error);
	if (FAILED(result))
	{
		OutputDebugString(_T("ピクセルシェーダコンパイル：失敗\n"));

		return result;
	}

	return result;
}

// モデル用シェーダーコンパイル
HRESULT Device::ShaderCompileModel(void)
{
	//頂点シェーダのコンパイル
	result = D3DCompileFromFile(_T("shader.hlsl"), nullptr, nullptr, "ModelVS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pipeline.vertex, &rootsignature.error);
	if (FAILED(result))
	{
		OutputDebugString(_T("頂点シェーダコンパイル：失敗\n"));

		return result;
	}

	//ピクセルシェーダのコンパイル
	result = D3DCompileFromFile(_T("shader.hlsl"), nullptr, nullptr, "ModelPS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pipeline.pixel, &rootsignature.error);
	if (FAILED(result))
	{
		OutputDebugString(_T("ピクセルシェーダコンパイル：失敗\n"));

		return result;
	}

	return result;
}

// パイプラインの生成
HRESULT Device::CreatePipeLine(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
	//頂点レイアウト設定用構造体の設定
	D3D12_INPUT_ELEMENT_DESC input[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BORN",     0, DXGI_FORMAT_R16G16_UINT,     0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHT",   0, DXGI_FORMAT_R8_UINT,         0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	//ラスタライザーステート設定用構造体の設定
	D3D12_RASTERIZER_DESC rasterizer;
	SecureZeroMemory(&rasterizer, sizeof(rasterizer));
	rasterizer.FillMode						= D3D12_FILL_MODE_SOLID;
	rasterizer.CullMode						= D3D12_CULL_MODE_NONE;
	rasterizer.FrontCounterClockwise		= FALSE;
	rasterizer.DepthBias					= D3D12_DEFAULT_DEPTH_BIAS;
	rasterizer.DepthBiasClamp				= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizer.SlopeScaledDepthBias			= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizer.DepthClipEnable				= TRUE;
	rasterizer.MultisampleEnable			= FALSE;
	rasterizer.AntialiasedLineEnable		= FALSE;
	rasterizer.ForcedSampleCount			= 0;
	rasterizer.ConservativeRaster			= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	//レンダーターゲットブレンド設定用構造体
	D3D12_RENDER_TARGET_BLEND_DESC renderBlend;
	SecureZeroMemory(&renderBlend, sizeof(renderBlend));
	renderBlend.BlendEnable					= FALSE;
	renderBlend.BlendOp						= D3D12_BLEND_OP_ADD;
	renderBlend.BlendOpAlpha				= D3D12_BLEND_OP_ADD;
	renderBlend.DestBlend					= D3D12_BLEND_ZERO;
	renderBlend.DestBlendAlpha				= D3D12_BLEND_ZERO;
	renderBlend.LogicOp						= D3D12_LOGIC_OP_NOOP;
	renderBlend.LogicOpEnable				= FALSE;
	renderBlend.RenderTargetWriteMask		= D3D12_COLOR_WRITE_ENABLE_ALL;
	renderBlend.SrcBlend					= D3D12_BLEND_ONE;
	renderBlend.SrcBlendAlpha				= D3D12_BLEND_ONE;

	//ブレンドステート設定用構造体
	D3D12_BLEND_DESC descBS;
	descBS.AlphaToCoverageEnable			= FALSE;
	descBS.IndependentBlendEnable			= FALSE;
	for (UINT i = 0; i < swap.bufferCnt; i++)
	{
		descBS.RenderTarget[i]				= renderBlend;
	}

	//パイプラインステート設定用構造体
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.InputLayout						= { input, _countof(input) };
	desc.PrimitiveTopologyType				= type;
	desc.pRootSignature						= rootsignature.rootSignature;
	desc.VS									= CD3DX12_SHADER_BYTECODE(pipeline.vertex);
	desc.PS									= CD3DX12_SHADER_BYTECODE(pipeline.pixel);
	desc.RasterizerState					= rasterizer;
	desc.BlendState							= descBS;
	desc.DepthStencilState.DepthEnable		= true;
	desc.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthStencilState.DepthFunc		= D3D12_COMPARISON_FUNC_LESS;
	desc.DepthStencilState.StencilEnable	= FALSE;
	desc.SampleMask							= UINT_MAX;
	desc.NumRenderTargets					= 1;
	desc.RTVFormats[0]						= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.DSVFormat							= DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count					= 1;

	//パイプライン生成
	result = command.dev->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline.pipeline));
	if (FAILED(result))
	{
		OutputDebugString(_T("パイプラインの生成：失敗\n"));

		return result;
	}

	return result;
}

// テクスチャ用頂点バッファの生成
HRESULT Device::CreateVertexBufferTexture(void)
{
	//三角形の頂点座標(上から時計回り)
	Vertex tran[] =
	{
		{ { -1.0f / 2.0f,  1.0f / 2.0f,	0.0f }, {0, 0} },	//左上
		{ {  1.0f / 2.0f,  1.0f / 2.0f,	0.0f }, {1, 0} },	//右上
		{ {  1.0f / 2.0f, -1.0f / 2.0f,	0.0f }, {1, 1} },	//右下

		{ {  1.0f / 2.0f, -1.0f / 2.0f,	0.0f }, {1, 1} },	//右下
		{ { -1.0f / 2.0f, -1.0f / 2.0f, 0.0f }, {0, 1} },	//左下
		{ { -1.0f / 2.0f,  1.0f / 2.0f,	0.0f }, {0, 0} }	//左上
	};

	//頂点用リソース生成
	result = command.dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(tran)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&view[mode[0]].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("頂点バッファ用リソースの生成：失敗\n"));

		return result;
	}

	//送信用データ
	UCHAR* data = nullptr;

	//送信範囲
	D3D12_RANGE range = { 0,0 };

	//マッピング
	result = view[mode[0]].resource->Map(0, &range, reinterpret_cast<void**>(&data));
	if (FAILED(result))
	{
		OutputDebugString(_T("頂点用リソースのマッピング：失敗\n"));

		return result;
	}

	//頂点データのコピー
	memcpy(data, &tran, (sizeof(tran)));

	//アンマッピング
	view[mode[0]].resource->Unmap(0, nullptr);

	//頂点バッファ設定用構造体の設定
	vertexView.BufferLocation	= view[mode[0]].resource->GetGPUVirtualAddress();
	vertexView.SizeInBytes		= sizeof(tran);
	vertexView.StrideInBytes	= sizeof(Vertex);

	return result;
}

// モデル用頂点バッファの生成
HRESULT Device::CreateVertexBufferModel(void)
{
	//頂点用リソース生成
	result = command.dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(VERETX) * PMD::GetInstance()->GetVertex().size()), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&view[mode[0]].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("頂点バッファ用リソースの生成：失敗\n"));

		return result;
	}

	//送信用データ
	UCHAR* data = nullptr;

	//送信範囲
	D3D12_RANGE range = { 0,0 };

	//マッピング
	result = view[mode[0]].resource->Map(0, &range, reinterpret_cast<void**>(&data));
	if (FAILED(result))
	{
		OutputDebugString(_T("頂点バッファ用リソースのマッピング：失敗\n"));

		return result;
	}

	//頂点データのコピー
	memcpy(data, &PMD::GetInstance()->GetVertex()[0], (sizeof(VERETX) * PMD::GetInstance()->GetVertex().size()));

	//アンマッピング
	view[mode[0]].resource->Unmap(0, nullptr);

	//頂点バッファ設定用構造体の設定
	vertexView.BufferLocation	= view[mode[0]].resource->GetGPUVirtualAddress();
	vertexView.SizeInBytes		= sizeof(VERETX) * PMD::GetInstance()->GetVertex().size();
	vertexView.StrideInBytes	= sizeof(VERETX);

	//インデックス
	result = command.dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(USHORT) * PMD::GetInstance()->GetIndex().size()), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&view[mode[0]].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("インデックスバッファ用リソースの生成：失敗\n"));

		return result;
	}

	//送信用データ
	UCHAR* idata = nullptr;

	//マッピング
	result = view[mode[0]].resource->Map(0, &range, reinterpret_cast<void**>(&data));
	if (FAILED(result))
	{
		OutputDebugString(_T("インデックスバッファ用リソースのマッピング：失敗\n"));

		return result;
	}

	//コピー
	memcpy(data, &PMD::GetInstance()->GetIndex()[0], sizeof(USHORT) * PMD::GetInstance()->GetIndex().size());

	//アンマッピング
	view[mode[0]].resource->Unmap(0, nullptr);

	//頂点インデックスビュー設定用構造体の設定
	indexView.BufferLocation	= view[mode[0]].resource->GetGPUVirtualAddress();
	indexView.SizeInBytes		= sizeof(USHORT) * PMD::GetInstance()->GetIndex().size();
	indexView.Format			= DXGI_FORMAT_R16_UINT;

	return result;
}

// 定数バッファの生成
HRESULT Device::CreateConstantBuffer(void)
{
	//定数バッファ設定用構造体の設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.NumDescriptors					= 2;
	heapDesc.Flags							= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type							= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	
	//ヒープ生成
	result = command.dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&view[mode[2]].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("定数バッファ用ヒープの生成：失敗\n"));

		return S_FALSE;
	}

	//ヒープサイズを取得
	view[mode[2]].size = command.dev->GetDescriptorHandleIncrementSize(heapDesc.Type);

	//ヒープ設定用構造体の設定
	D3D12_HEAP_PROPERTIES prop;
	SecureZeroMemory(&prop, sizeof(prop));
	prop.Type								= D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty					= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference				= D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask					= 1;
	prop.VisibleNodeMask					= 1;

	//リソース設定用構造体の設定
	D3D12_RESOURCE_DESC resourceDesc;
	SecureZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Dimension					= D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width						= ((sizeof(WVP) + 0xff) &~0xff);
	resourceDesc.Height						= 1;
	resourceDesc.DepthOrArraySize			= 1;
	resourceDesc.MipLevels					= 1;
	resourceDesc.Format						= DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count			= 1;
	resourceDesc.Flags						= D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Layout						= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//リソース生成
	result = command.dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&view[mode[2]].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("定数バッファ用リソースの生成：失敗\n"));

		return result;
	}

	//定数バッファビュー設定用構造体の設定
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.BufferLocation						= view[mode[2]].resource->GetGPUVirtualAddress();
	desc.SizeInBytes						= (sizeof(WVP) + 0xff) &~0xff;

	//定数バッファビュー生成
	command.dev->CreateConstantBufferView(&desc, view[mode[2]].heap->GetCPUDescriptorHandleForHeapStart());

	//送信範囲
	D3D12_RANGE range = { 0, 0 };
	
	//マッピング
	result = view[mode[2]].resource->Map(0, &range, (void**)(&data));
	if (FAILED(result))
	{
		OutputDebugString(_T("定数バッファ用リソースのマッピング：失敗\n"));

		return result;
	}

	//コピー
	memcpy(data, &wvp, sizeof(DirectX::XMMATRIX));

	return result;
}

// 初期処理
void Device::Init(void)
{
	//WVP
	SetWVP();

	//コマンド
	result = CreateCommand();
	if (FAILED(result))
	{
		return;
	}

	//スワップチェイン
	result = CreateSwapChain();
	if (FAILED(result))
	{
		return;
	}

	//レンダーターゲット
	result = CreateRenderTargetView();
	if (FAILED(result))
	{
		return;
	}

	//深度ステンシル
	result = CreateDepthStencil();
	if (FAILED(result))
	{
		return;
	}

	//フェンス
	result = CreateFence();
	if (FAILED(result))
	{
		return;
	}

	//ルートシグネチャ
	result = CreateRootSignature();
	if (FAILED(result))
	{
		return;
	}

	//定数バッファ
	result = CreateConstantBuffer();
	if (FAILED(result))
	{
		return;
	}


	TextInit();
	//ModelInit();
}

// テクスチャ用の初期処理
void Device::TextInit(void)
{
	/*if (BMP::GetInstance() == nullptr)
	{
		//BMPクラスのインスタンス
		BMP::Create();
	}

	//BMP読み込み
	result = BMP::GetInstance()->LoadBMP(0, "sample/texturesample24bit.bmp", command.dev);
	if (FAILED(result))
	{
		return;
	}*/

	//シェーダー
	result = ShaderCompileTexture();
	if (FAILED(result))
	{
		return;
	}

	//パイプライン
	result = CreatePipeLine(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	if (FAILED(result))
	{
		return;
	}

	//頂点データ
	result = CreateVertexBufferTexture();
	if (FAILED(result))
	{
		return;
	}
}

// モデル用の初期化
void Device::ModelInit(void)
{
	if (PMD::GetInstance() == nullptr)
	{
		//PMDクラスのインスタンスインスタンス
		PMD::Create();
	}

	//PMD読み込み
	result = PMD::GetInstance()->LoadPMD("MikuMikuDance/UserFile/Model/初音ミク.pmd", command.dev);
	if (FAILED(result))
	{
		return;
	}

	//ボーンの回転
	//PMD::GetInstance()->BornRotation("左肩", DirectX::XMMatrixRotationZ(RAD(60)));

	//シェーダー
	result = ShaderCompileModel();
	if (FAILED(result))
	{
		return;
	}

	//パイプライン
	result = CreatePipeLine(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	//result = CreatePipeLine(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
	if (FAILED(result))
	{
		return;
	}

	//頂点データ
	result = CreateVertexBufferModel();
	if (FAILED(result))
	{
		return;
	}
}

// 待機処理
void Device::Wait(void)
{
	//フェンス値更新
	fence.fenceCnt++;

	//フェンス値を変更
	result = command.queue->Signal(fence.fence, fence.fenceCnt);
	if (FAILED(result))
	{
		OutputDebugString(_T("フェンス値の更新：失敗\n"));

		return;
	}

	//完了を待機(ポーリング)
	while (fence.fence->GetCompletedValue() != fence.fenceCnt)
	{
		/*auto a = fence.fenceCnt;
		std::stringstream s;
		s << a;
		OutputDebugStringA(s.str().c_str());*/

		//フェンスイベントのセット
		result = fence.fence->SetEventOnCompletion(fence.fenceCnt, fence.fenceEvent);
		if (FAILED(result))
		{
			OutputDebugString(_T("フェンスイベントのセット：失敗\n"));

			return;
		}

		//フェンスイベントの待機
		WaitForSingleObject(fence.fenceEvent, INFINITE);
	}
}

// ビューポートのセット
D3D12_VIEWPORT Device::SetViewPort(void)
{
	//ビューポート設定用構造体の設定
	D3D12_VIEWPORT viewPort;
	SecureZeroMemory(&viewPort, sizeof(viewPort));
	viewPort.TopLeftX	= 0;
	viewPort.TopLeftY	= 0;
	viewPort.Width		= WINDOW_X;
	viewPort.Height		= WINDOW_Y;
	viewPort.MinDepth	= 0;
	viewPort.MaxDepth	= 1;

	return viewPort;
}

// シザーのセット
RECT Device::SetScissor(void)
{
	//シザー設定用構造体の設定
	RECT scissor;
	SecureZeroMemory(&scissor, sizeof(scissor));
	scissor.left	= 0;
	scissor.right	= WINDOW_X;
	scissor.top		= 0;
	scissor.bottom	= WINDOW_Y;

	return scissor;
}

// バリアの設置
void Device::Barrier(D3D12_RESOURCE_STATES befor, D3D12_RESOURCE_STATES affter)
{
	//バリア設定用構造体の設定
	D3D12_RESOURCE_BARRIER barrier;
	ZeroMemory(&barrier, sizeof(barrier));
	barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource	= renderTarget[swap.swapChain->GetCurrentBackBufferIndex()];
	barrier.Transition.StateBefore	= befor;
	barrier.Transition.StateAfter	= affter;
	barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_FLAG_NONE;

	//バリア設置
	command.list->ResourceBarrier(1, &barrier);
}

// 定数バッファのセット
void Device::SetConstantBuffer(void)
{
	//定数バッファヒープの先頭ハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE handle = view[mode[2]].heap->GetGPUDescriptorHandleForHeapStart();

	//定数バッファヒープのセット
	command.list->SetDescriptorHeaps(1, &view[mode[2]].heap);

	//定数バッファディスクラプターテーブルのセット
	command.list->SetGraphicsRootDescriptorTable(0, handle);
}

// レンダーターゲットのセット
void Device::SetRenderTarget(void)
{
	//頂点ヒープの先頭ハンドルの取得
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(view[mode[0]].heap->GetCPUDescriptorHandleForHeapStart(), swap.swapChain->GetCurrentBackBufferIndex(), view[mode[0]].size);

	//レンダーターゲットのセット
	command.list->OMSetRenderTargets(1, &handle, false, &view[mode[1]].heap->GetCPUDescriptorHandleForHeapStart());

	//クリアカラーの指定
	const FLOAT clearColor[] = { 1.0f,0.0f,1.0f,1.0f };
	//レンダーターゲットのクリア
	command.list->ClearRenderTargetView(handle, clearColor, 0, nullptr);
}

// 深度ステンシルビューのクリア
void Device::ClearDepthStencil(void)
{
	//深度ステンシルヒープの先頭ハンドルの取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle_DSV = view[mode[1]].heap->GetCPUDescriptorHandleForHeapStart();

	//深度ステンシルビューのクリア
	command.list->ClearDepthStencilView(handle_DSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

// 処理
void Device::UpData(void)
{
	if (input.lock()->InputKey(DIK_RIGHT) == TRUE)
	{
		//回転
		angle[0]++;
		//行列更新
		wvp.world = DirectX::XMMatrixRotationY(RAD(angle[0]));
	}
	else if (input.lock()->InputKey(DIK_LEFT) == TRUE)
	{
		//回転
		angle[0]--;
		//行列更新
		wvp.world = DirectX::XMMatrixRotationY(RAD(angle[0]));
	}
	
	//行列データ更新
	memcpy(data, &wvp, sizeof(WVP));
	

	//コマンドアロケータのリセット
	command.allocator->Reset();
	//リストのリセット
	command.list->Reset(command.allocator, pipeline.pipeline);

	//ルートシグネチャのセット
	command.list->SetGraphicsRootSignature(rootsignature.rootSignature);

	//パイプラインのセット
	command.list->SetPipelineState(pipeline.pipeline);

	SetConstantBuffer();

	//ビューのセット
	command.list->RSSetViewports(1, &SetViewPort());

	//シザーのセット
	command.list->RSSetScissorRects(1, &SetScissor());

	// Present ---> RenderTarget
	Barrier(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	SetRenderTarget();

	ClearDepthStencil();
	
	//頂点バッファビューのセット
	command.list->IASetVertexBuffers(0, 1, &vertexView);

	// 描画
	command.list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	command.list->DrawInstanced(6, 1, 0, 0);
	
	// RenderTarget ---> Present
	Barrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	//コマンドリストの記録終了
	command.list->Close();

	//リストの配列
	ID3D12CommandList *commandList[] = { command.list };
	//配列でない場合：queue->ExecuteCommandLists(1, (ID3D12CommandList*const*)&list);
	command.queue->ExecuteCommandLists(_countof(commandList), commandList);

	//裏、表画面を反転
	swap.swapChain->Present(1, 0);

	Wait();
}
