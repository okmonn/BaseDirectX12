#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

class BMP
{
	//�摜�T�C�Y�̍\����
	typedef struct
	{
		LONG	width;
		LONG	height;
	}IMAGE_SIZE;

	//BMP�f�[�^�̍\����
	typedef struct
	{
		//�摜�T�C�Y
		IMAGE_SIZE				size;
		//bmp�f�[�^
		std::vector<UCHAR>		bmp;
		//�q�[�v
		ID3D12DescriptorHeap*	heap;
		//���\�[�X
		ID3D12Resource*			resource;
	}Data;

public:
	// �f�X�g���N�^
	~BMP();

	// �C���X�^���X
	static void Create(void);
	// �j��
	static void Destroy(void);

	// �擾
	static BMP* GetInstance(void)
	{
		return s_Instance;
	}

	//=====�ǂݍ���=====
	// BMP�f�[�^�̓ǂݍ���
	HRESULT LoadBMP(USHORT index, std::string fileName, ID3D12Device* dev);
	// WIC�e�N�X�`���̓ǂݍ���
	HRESULT LoadTextureWIC(USHORT index, std::wstring fileName, ID3D12Device* dev);

	//=====����=====
	// �q�[�v�̐���
	HRESULT CreateHeap(USHORT index, ID3D12Device* dev);
	// ���\�[�X�̐���
	HRESULT CreateResource(USHORT index, ID3D12Device* dev);

	//=====�`�揀��=====
	// �`�揀��
	void SetDraw(USHORT index, ID3D12GraphicsCommandList* list);
	// WIC�p�`�揀��
	void SetDrawWIC(USHORT index, ID3D12GraphicsCommandList* list);

	// �`��
	void Draw(USHORT index, ID3D12GraphicsCommandList* list);

private:
	// �R���X�g���N�^
	BMP();

	// �C���X�^���X�ϐ�
	static BMP* s_Instance;

	// �Q�ƌ���
	HRESULT result;

	// BMP�f�[�^�̍\���̂̔z��
	std::map<USHORT, Data>data;

	// �f�R�[�h�f�[�^�̔z��
	std::map<USHORT, std::unique_ptr<uint8_t[]>>decode;

	// �T�u���\�[�X�̔z��
	std::map<USHORT, D3D12_SUBRESOURCE_DATA>sub;
};

