#pragma once
#include "Typedef.h"
#include <dxgi1_4.h>
#include <string>
#include <vector>
#include <map>

class PMD
{
	//�}�e���A��
	typedef struct
	{
		//��{�F
		DirectX::XMFLOAT3	diffuse;
		//�e�N�X�`���K���t���O
		BOOL				existTexture;
	}Mat;

	//�f�[�^����̍\����
	typedef struct
	{
		//�q�[�v
		ID3D12DescriptorHeap*	heap;
		//���\�[�X
		ID3D12Resource*			resource;
		//�q�[�v�T�C�Y
		UINT					size;
	}Data;

#pragma pack(1)
	//PMD�w�b�_�[�p�\����
	typedef struct
	{
		//�^�C�v
		UCHAR	type[3];
		//�o�[�W����
		FLOAT	ver;
		//���O
		UCHAR	name[20];
		//�R�����g
		UCHAR	comment[256];
		//���_��
		UINT	vertexNum;
	}Header;
#pragma pack()

#pragma pack(1)
	//PMD�}�e���A���f�[�^�̍\����
	typedef struct
	{
		//��{�F
		DirectX::XMFLOAT3	diffuse;
		//�����x
		FLOAT				alpha;
		//���ˋ��x
		FLOAT				specularity;
		//���ːF
		DirectX::XMFLOAT3	specula;
		//���F
		DirectX::XMFLOAT3	mirror;
		//�g�D�[���ԍ�
		UCHAR				toonIndex;
		//�֊s���t���O
		UCHAR				edge;
		//�C���f�b�N�X��
		UINT				indexNum;
		//�e�N�X�`���p�X
		CHAR				textureFilePath[20];
	}Material;
#pragma pack()

	//PMD�{�[���f�[�^�̍\����
	typedef struct
	{
		//���O
		CHAR				name[20];
		//�e�{�[���ԍ�
		WORD				parent_born_index;
		//�q�{�[���ԍ�
		WORD				tail_born_index;
		//�^�C�v
		BYTE				type;
		//IK�e�{�[���ԍ�
		WORD				ik_parent_born_index;
		//���W
		DirectX::XMFLOAT3	pos;
	}Born;

	//�{�[�����W�̍\����
	typedef struct
	{
		//�擪���W
		DirectX::XMFLOAT3	head;
		//�������W
		DirectX::XMFLOAT3	tail;
	}Pos;

	//�{�[���m�[�h�̍\����
	typedef struct
	{
		std::vector<USHORT>	index;
	}Node;


	// VMD�w�b�_�[�p�\����
	typedef struct
	{
		CHAR data[30];
		CHAR name[20];
	}VMD_Header;

#pragma pack(1)
	// VMD���[�V�����f�[�^�\����
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
	// �f�X�g���N�^
	~PMD();

	// �C���X�^���X
	static void Create(void);
	// �j��
	static void Destroy(void);

	// �擾
	static PMD* GetInstance(void)
	{
		return s_Instance;
	}

	//=====�t�H���_�[=====
	// �t�H���_�[�Ƃ̘A��
	std::string FolderPath(std::string path, const char* textureName);
	// ���j�R�[�h�ϊ�
	std::wstring ChangeUnicode(const CHAR* str);

	// PMD�ǂݍ���
	HRESULT LoadPMD(std::string fileName, ID3D12Device* dev);

	//=====�e�N�X�`��=====
	// �e�N�X�`���ǂݍ���
	void SetTexture(ID3D12Device* dev);
	// WIC�e�N�X�`���ǂݍ���
	void SetTextureWIC(ID3D12Device* dev);

	// �{�[���̃Z�b�g
	void SetBorn(void);

	//=====�{�[��=====
	// �{�[���̉�]
	void BornRotation(std::string name, const DirectX::XMMATRIX matrix);
	// �q�{�[���̉�]
	void MoveBorn(USHORT index, const DirectX::XMMATRIX& matrix);

	// �萔�o�b�t�@�̐���
	HRESULT CreateConstantBuffer(ID3D12Device* dev);

	// �`��
	void Draw(ID3D12GraphicsCommandList* list, D3D12_INDEX_BUFFER_VIEW indexView);

	//=====�擾=====
	// ���_�f�[�^�̎擾
	std::vector<VERETX> GetVertex(void);
	// ���_�C���f�b�N�X�f�[�^�̎擾
	std::vector<USHORT>GetIndex(void);


	// VMD�ǂݍ���
	HRESULT LoadVMD(std::string fileName);

	// ���[�V����
	void Motion(void);
	void Motion(std::string name, DirectX::XMVECTOR quaternion);
	void Motion(USHORT index, DirectX::XMMATRIX matrix);



private:
	// �R���X�g���N�^
	PMD();

	// �C���X�^���X�ϐ�
	static PMD* s_Instance;

	// �Q�ƌ���
	HRESULT result;

	// ���M�f�[�^
	UINT8* data;
	
	// �{�[���p���M�f�[�^
	UINT* bornData;

	// �}�e���A���\����
	Mat mat;

	// �f�[�^����̍\����
	Data view;

	// �{�[���p�̃f�[�^����̍\����
	Data bornView;

	// PMD�w�b�_�[�\����
	Header header;

	// PMD���_�f�[�^�\���̂̔z��
	std::vector<VERETX>vertex;

	// PMD�C���f�b�N�X�f�[�^�̔z��
	std::vector<USHORT>index;

	// PMD�}�e���A���f�[�^�\���̂̔z��
	std::vector<Material>material;

	// PMD�{�[���f�[�^�\���̂̔z��
	std::vector<Born>born;

	// �{�[�����W�\���̂̔z��
	std::vector<Pos>pos;

	// �{�[���m�[�h�\���̂̔z��
	std::vector<Node>node;

	// �{�[�����z��
	std::map<std::string, UINT>map;

	// �{�[���s��̔z��
	std::vector<DirectX::XMMATRIX>matrix;



	// VMD�w�b�_�[�\����
	VMD_Header vmd_header;

	// VMD���[�V�����\���̂̔z��
	std::vector<VMD_Motion>motion;
};