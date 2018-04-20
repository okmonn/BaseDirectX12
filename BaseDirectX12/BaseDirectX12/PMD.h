#pragma once
#include "Typedef.h"
#include <dxgi1_4.h>
#include <string>
#include <vector>
#include <map>

class PMD
{
	//マテリアル
	typedef struct
	{
		//基本色
		DirectX::XMFLOAT3	diffuse;
		//テクスチャ適応フラグ
		BOOL				existTexture;
	}Mat;

	//データ周りの構造体
	typedef struct
	{
		//ヒープ
		ID3D12DescriptorHeap*	heap;
		//リソース
		ID3D12Resource*			resource;
		//ヒープサイズ
		UINT					size;
	}Data;

#pragma pack(1)
	//PMDヘッダー用構造体
	typedef struct
	{
		//タイプ
		UCHAR	type[3];
		//バージョン
		FLOAT	ver;
		//名前
		UCHAR	name[20];
		//コメント
		UCHAR	comment[256];
		//頂点数
		UINT	vertexNum;
	}Header;
#pragma pack()

#pragma pack(1)
	//PMDマテリアルデータの構造体
	typedef struct
	{
		//基本色
		DirectX::XMFLOAT3	diffuse;
		//透明度
		FLOAT				alpha;
		//反射強度
		FLOAT				specularity;
		//反射色
		DirectX::XMFLOAT3	specula;
		//環境色
		DirectX::XMFLOAT3	mirror;
		//トゥーン番号
		UCHAR				toonIndex;
		//輪郭線フラグ
		UCHAR				edge;
		//インデックス数
		UINT				indexNum;
		//テクスチャパス
		CHAR				textureFilePath[20];
	}Material;
#pragma pack()

	//PMDボーンデータの構造体
	typedef struct
	{
		//名前
		CHAR				name[20];
		//親ボーン番号
		WORD				parent_born_index;
		//子ボーン番号
		WORD				tail_born_index;
		//タイプ
		BYTE				type;
		//IK親ボーン番号
		WORD				ik_parent_born_index;
		//座標
		DirectX::XMFLOAT3	pos;
	}Born;

	//ボーン座標の構造体
	typedef struct
	{
		//先頭座標
		DirectX::XMFLOAT3	head;
		//末尾座標
		DirectX::XMFLOAT3	tail;
	}Pos;

	//ボーンノードの構造体
	typedef struct
	{
		std::vector<USHORT>	index;
	}Node;


	// VMDヘッダー用構造体
	typedef struct
	{
		CHAR data[30];
		CHAR name[20];
	}VMD_Header;

#pragma pack(1)
	// VMDモーションデータ構造体
	typedef struct
	{
		CHAR bornName[15];
		UINT frameNo;
		DirectX::XMFLOAT3 location;
		DirectX::XMFLOAT4 rotatation;
		UCHAR interpolation[64];
	}VMD_Motion;
#pragma pack()

public:
	// デストラクタ
	~PMD();

	// インスタンス
	static void Create(void);
	// 破棄
	static void Destroy(void);

	// 取得
	static PMD* GetInstance(void)
	{
		return s_Instance;
	}

	//=====フォルダー=====
	// フォルダーとの連結
	std::string FolderPath(std::string path, const char* textureName);
	// ユニコード変換
	std::wstring ChangeUnicode(const CHAR* str);

	// PMD読み込み
	HRESULT LoadPMD(std::string fileName, ID3D12Device* dev);

	//=====テクスチャ=====
	// テクスチャ読み込み
	void SetTexture(ID3D12Device* dev);
	// WICテクスチャ読み込み
	void SetTextureWIC(ID3D12Device* dev);

	// ボーンのセット
	void SetBorn(void);

	//=====ボーン=====
	// ボーンの回転
	void BornRotation(std::string name, const DirectX::XMMATRIX matrix);
	// 子ボーンの回転
	void MoveBorn(USHORT index, const DirectX::XMMATRIX& matrix);

	// 定数バッファの生成
	HRESULT CreateConstantBuffer(ID3D12Device* dev);

	// 描画
	void Draw(ID3D12GraphicsCommandList* list, D3D12_INDEX_BUFFER_VIEW indexView);

	//=====取得=====
	// 頂点データの取得
	std::vector<VERETX> GetVertex(void);
	// 頂点インデックスデータの取得
	std::vector<USHORT>GetIndex(void);


	// VMD読み込み
	HRESULT LoadVMD(std::string fileName);

	// モーション
	void Motion(void);
	void Motion(std::string name, DirectX::XMVECTOR quaternion);
	void Motion(USHORT index, DirectX::XMMATRIX matrix);



private:
	// コンストラクタ
	PMD();

	// インスタンス変数
	static PMD* s_Instance;

	// 参照結果
	HRESULT result;

	// 送信データ
	UINT8* data;
	
	// ボーン用送信データ
	UINT* bornData;

	// マテリアル構造体
	Mat mat;

	// データ周りの構造体
	Data view;

	// ボーン用のデータ周りの構造体
	Data bornView;

	// PMDヘッダー構造体
	Header header;

	// PMD頂点データ構造体の配列
	std::vector<VERETX>vertex;

	// PMDインデックスデータの配列
	std::vector<USHORT>index;

	// PMDマテリアルデータ構造体の配列
	std::vector<Material>material;

	// PMDボーンデータ構造体の配列
	std::vector<Born>born;

	// ボーン座標構造体の配列
	std::vector<Pos>pos;

	// ボーンノード構造体の配列
	std::vector<Node>node;

	// ボーン名配列
	std::map<std::string, UINT>map;

	// ボーン行列の配列
	std::vector<DirectX::XMMATRIX>matrix;



	// VMDヘッダー構造体
	VMD_Header vmd_header;

	// VMDモーション構造体の配列
	std::vector<VMD_Motion>motion;
};