#pragma once
#include "Window.h"
#include "Input.h"
#include "Typedef.h"
#include <dxgi1_4.h>
#include <memory>
#include <vector>
#include <map>

class Device
{
	//コマンド周りの構造体
	typedef struct
	{
		//デバイス
		ID3D12Device*				dev;
		//コマンドアロケータ
		ID3D12CommandAllocator*		allocator;
		//コマンドリスト
		ID3D12GraphicsCommandList*	list;
		//コマンドキュー
		ID3D12CommandQueue*			queue;
	}Command;

	//スワップチェイン周りの構造体
	typedef struct
	{
		//インターフェースファクトリー
		IDXGIFactory4*		factory;
		//スワップチェイン
		IDXGISwapChain3*	swapChain;
		//バックバッファ数
		UINT				bufferCnt;
	}Swap;

	//データ周りの構造体
	typedef struct
	{
		//ヒープ
		ID3D12DescriptorHeap*	heap;
		//リソース
		ID3D12Resource*			resource;
		//ヒープサイズ
		UINT					size;
	}View;

	//フェンス周りの構造体
	typedef struct
	{
		//フェンス
		ID3D12Fence*	fence;
		//フェンス値
		UINT64			fenceCnt;
		//フェンスイベント
		HANDLE			fenceEvent;
	}Fence;

	//ルートシグネチャ周りの構造体
	typedef struct
	{
		ID3DBlob*				signature;
		ID3DBlob*				error;
		ID3D12RootSignature*	rootSignature;
	}RootSignature;

	//パイプライン周りの構造体
	typedef struct
	{
		//頂点シェーダ
		ID3DBlob*				vertex;
		//ピクセルシェーダ
		ID3DBlob*				pixel;
		//パイプライン
		ID3D12PipelineState*	pipeline;
	}PipeLine;

	//頂点座標
	typedef struct
	{
		//頂点座標
		DirectX::XMFLOAT3	pos;
		//uv値
		DirectX::XMFLOAT2	uv;
	}Vertex;

	//WorldViewProjectionの構造体
	typedef struct
	{
		//ワールド行列
		DirectX::XMMATRIX	world;
		//ビュープロジェクション行列
		DirectX::XMMATRIX	viewProjection;
	}WVP;


public:
	// コンストラクタ
	Device(std::weak_ptr<Window>winAdr, std::weak_ptr<Input>inputAdr);
	// デストラクタ
	~Device();


	//=====WVP=====

	// ビュー行列のセット
	DirectX::XMMATRIX SetViewMatrix(void);
	// プロジェクション行列のセット
	DirectX::XMMATRIX SetProjectionMatrix(void);
	// WVPのセット
	void SetWVP(void);


	//=====コマンド=====

	// デバイスの生成
	HRESULT CreateDevice(void);
	// コマンド周りの生成
	HRESULT CreateCommand(void);


	//=====スワップチェイン=====

	// インターフェースファクトリーの生成
	HRESULT CreateFactory(void);
	// スワップチェインの生成
	HRESULT CreateSwapChain(void);


	//=====頂点ビュー・深度ビュー=====

	// レンダーターゲットの生成
	HRESULT CreateRenderTargetView(void);
	// 深度ステンシルの生成
	HRESULT CreateDepthStencil(void);


	// フェンスの生成
	HRESULT CreateFence(void);


	//=====ルートシグネチャ=====

	// ルートシグネチャのシリアライズ
	HRESULT SerializeRootSignature(void);
	// ルートシグネチャの生成
	HRESULT CreateRootSignature(void);


	//=====シェーダー=====

	// テクスチャ用シェーダーコンパイル
	HRESULT ShaderCompileTexture(void);
	// モデル用シェーダーコンパイル
	HRESULT ShaderCompileModel(void);


	// テクスチャ用パイプラインの生成
	HRESULT CreatePipeLine(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);


	//=====頂点バッファ=====

	// テクスチャ用頂点バッファの生成
	HRESULT CreateVertexBufferTexture(void);
	// モデル用頂点バッファの生成
	HRESULT CreateVertexBufferModel(void);


	// 定数バッファの生成
	HRESULT CreateConstantBuffer(void);


	// 初期処理
	void Init(void);


	//=====初期処理=====

	// テクスチャ用の初期処理
	void TextInit(void);
	// モデル用の初期化
	void ModelInit(void);


	// 待機処理
	void Wait(void);


	//=====セット=====

	// ビューポートのセット
	D3D12_VIEWPORT SetViewPort(void);
	// シザーのセット
	RECT SetScissor(void);

	// バリアの設置
	void Barrier(D3D12_RESOURCE_STATES befor, D3D12_RESOURCE_STATES affter);

	//=====セット=====
	// 定数バッファのセット
	void SetConstantBuffer(void);
	// レンダーターゲットのセット
	void SetRenderTarget(void);
	// 深度ステンシルビューのクリア
	void ClearDepthStencil(void);
	
	// 処理
	void UpData(void);

private:
	// ウィンドウクラス参照
	std::weak_ptr<Window>win;

	// インプットクラス参照
	std::weak_ptr<Input>input;

	// 機能レベル
	D3D_FEATURE_LEVEL level;

	// 参照結果
	HRESULT result;

	// 回転角度
	FLOAT angle[2];

	// 行列データ
	UINT8* data;

	// コマンド周りの構造体
	Command command;

	// スワップチェイン周りの構造体
	Swap swap;

	// フェンス周りの構造体
	Fence fence;

	// ルートシグネチャ周りの構造体
	RootSignature rootsignature;

	// パイプライン周りの構造体
	PipeLine pipeline;

	// 頂点バッファの設定用構造体
	D3D12_VERTEX_BUFFER_VIEW vertexView;

	// 頂点インデックスの設定用構造体
	D3D12_INDEX_BUFFER_VIEW indexView;

	// WVPの設定用構造体
	WVP wvp;

	// データ周りの構造体の配列
	std::map<std::string, View>view;

	// レンダーターゲット用リソース配列
	std::vector<ID3D12Resource*>renderTarget;
};