#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <vector>
#include <map>
#include <string>

class Texture
{
	// �񎟌����W
	struct Vector2
	{
		float x;
		float y;
	};

	// �摜�T�C�Y
	struct Size
	{
		LONG	width;
		LONG	height;
	};

	// �f�[�^
	struct Descriptor
	{
		//�q�[�v
		ID3D12DescriptorHeap* heap;
		//���\�[�X
		ID3D12Resource* resorce;
	};

	//���_���W
	typedef struct
	{
		//���_���W
		DirectX::XMFLOAT3	pos;
		//uv�l
		DirectX::XMFLOAT2	uv;
	}Vertex;

	// BMP�f�[�^�̉�
	struct Data
	{
		//�摜�T�C�Y
		Size		size;
		//bmp�f�[�^
		std::vector<UCHAR>bmp;
		//�f�B�X�N���v�^�[
		Descriptor texture[2];
		//���_���
		std::vector<Vertex>vertex;
	};

public:
	// �f�X�g���N�^
	~Texture();

	// �C���X�^���X��
	static void Create(void);
	// �j��
	static void Destroy(void);

	// �C���X�^���X�ϐ��擾
	static Texture* GetInstance()
	{
		return s_Instance;
	}

	// �摜�f�[�^�̓ǂݍ���
	HRESULT LoadBMP(USHORT index, std::string fileName, UINT swapBufferNum, ID3D12Device* dev);

	// �`��
	HRESULT Draw(USHORT index, Vector2 pos, Vector2 size, Vector2 wSize, ID3D12GraphicsCommandList* list);

private:
	// �R���X�g���N�^
	Texture();

	// ���_�q�[�v�̐���
	HRESULT CreateVertexHeap(USHORT index, UINT swapBufferNum, ID3D12Device* dev);
	// ���_�o�b�t�@�̐���
	HRESULT CreateVertexBuffer(USHORT index, ID3D12Device* dev);

	// �萔�q�[�v�̐���
	HRESULT CreateConstantHeap(USHORT index, ID3D12Device* dev);
	// �萔�o�b�t�@�̐���
	HRESULT CreateConstantBuffer(USHORT index, ID3D12Device* dev);

	// ���_�ݒ�
	HRESULT SetVertex(USHORT index, Vector2 pos, Vector2 size, Vector2 wSize);

	// �C���X�^���X�ϐ�
	static Texture* s_Instance;
	UCHAR* d = nullptr;
	// �Q�ƌ���
	HRESULT result;

	// �f�[�^
	std::map<USHORT, Data>data;

	// ���_�o�b�t�@�r���[�ݒ�p�\����
	D3D12_VERTEX_BUFFER_VIEW view;
};

