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
	//�R�}���h����̍\����
	typedef struct
	{
		//�f�o�C�X
		ID3D12Device*				dev;
		//�R�}���h�A���P�[�^
		ID3D12CommandAllocator*		allocator;
		//�R�}���h���X�g
		ID3D12GraphicsCommandList*	list;
		//�R�}���h�L���[
		ID3D12CommandQueue*			queue;
	}Command;

	//�X���b�v�`�F�C������̍\����
	typedef struct
	{
		//�C���^�[�t�F�[�X�t�@�N�g���[
		IDXGIFactory4*		factory;
		//�X���b�v�`�F�C��
		IDXGISwapChain3*	swapChain;
		//�o�b�N�o�b�t�@��
		UINT				bufferCnt;
	}Swap;

	//�f�[�^����̍\����
	typedef struct
	{
		//�q�[�v
		ID3D12DescriptorHeap*	heap;
		//���\�[�X
		ID3D12Resource*			resource;
		//�q�[�v�T�C�Y
		UINT					size;
	}View;

	//�t�F���X����̍\����
	typedef struct
	{
		//�t�F���X
		ID3D12Fence*	fence;
		//�t�F���X�l
		UINT64			fenceCnt;
		//�t�F���X�C�x���g
		HANDLE			fenceEvent;
	}Fence;

	//���[�g�V�O�l�`������̍\����
	typedef struct
	{
		ID3DBlob*				signature;
		ID3DBlob*				error;
		ID3D12RootSignature*	rootSignature;
	}RootSignature;

	//�p�C�v���C������̍\����
	typedef struct
	{
		//���_�V�F�[�_
		ID3DBlob*				vertex;
		//�s�N�Z���V�F�[�_
		ID3DBlob*				pixel;
		//�p�C�v���C��
		ID3D12PipelineState*	pipeline;
	}PipeLine;

	//���_���W
	typedef struct
	{
		//���_���W
		DirectX::XMFLOAT3	pos;
		//uv�l
		DirectX::XMFLOAT2	uv;
	}Vertex;

	//WorldViewProjection�̍\����
	typedef struct
	{
		//���[���h�s��
		DirectX::XMMATRIX	world;
		//�r���[�v���W�F�N�V�����s��
		DirectX::XMMATRIX	viewProjection;
	}WVP;


public:
	// �R���X�g���N�^
	Device(std::weak_ptr<Window>winAdr, std::weak_ptr<Input>inputAdr);
	// �f�X�g���N�^
	~Device();


	//=====WVP=====

	// �r���[�s��̃Z�b�g
	DirectX::XMMATRIX SetViewMatrix(void);
	// �v���W�F�N�V�����s��̃Z�b�g
	DirectX::XMMATRIX SetProjectionMatrix(void);
	// WVP�̃Z�b�g
	void SetWVP(void);


	//=====�R�}���h=====

	// �f�o�C�X�̐���
	HRESULT CreateDevice(void);
	// �R�}���h����̐���
	HRESULT CreateCommand(void);


	//=====�X���b�v�`�F�C��=====

	// �C���^�[�t�F�[�X�t�@�N�g���[�̐���
	HRESULT CreateFactory(void);
	// �X���b�v�`�F�C���̐���
	HRESULT CreateSwapChain(void);


	//=====���_�r���[�E�[�x�r���[=====

	// �����_�[�^�[�Q�b�g�̐���
	HRESULT CreateRenderTargetView(void);
	// �[�x�X�e���V���̐���
	HRESULT CreateDepthStencil(void);


	// �t�F���X�̐���
	HRESULT CreateFence(void);


	//=====���[�g�V�O�l�`��=====

	// ���[�g�V�O�l�`���̃V���A���C�Y
	HRESULT SerializeRootSignature(void);
	// ���[�g�V�O�l�`���̐���
	HRESULT CreateRootSignature(void);


	//=====�V�F�[�_�[=====

	// �e�N�X�`���p�V�F�[�_�[�R���p�C��
	HRESULT ShaderCompileTexture(void);
	// ���f���p�V�F�[�_�[�R���p�C��
	HRESULT ShaderCompileModel(void);


	// �e�N�X�`���p�p�C�v���C���̐���
	HRESULT CreatePipeLine(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);


	//=====���_�o�b�t�@=====

	// �e�N�X�`���p���_�o�b�t�@�̐���
	HRESULT CreateVertexBufferTexture(void);
	// ���f���p���_�o�b�t�@�̐���
	HRESULT CreateVertexBufferModel(void);


	// �萔�o�b�t�@�̐���
	HRESULT CreateConstantBuffer(void);


	// ��������
	void Init(void);


	//=====��������=====

	// �e�N�X�`���p�̏�������
	void TextInit(void);
	// ���f���p�̏�����
	void ModelInit(void);


	// �ҋ@����
	void Wait(void);


	//=====�Z�b�g=====

	// �r���[�|�[�g�̃Z�b�g
	D3D12_VIEWPORT SetViewPort(void);
	// �V�U�[�̃Z�b�g
	RECT SetScissor(void);

	// �o���A�̐ݒu
	void Barrier(D3D12_RESOURCE_STATES befor, D3D12_RESOURCE_STATES affter);

	//=====�Z�b�g=====
	// �萔�o�b�t�@�̃Z�b�g
	void SetConstantBuffer(void);
	// �����_�[�^�[�Q�b�g�̃Z�b�g
	void SetRenderTarget(void);
	// �[�x�X�e���V���r���[�̃N���A
	void ClearDepthStencil(void);
	
	// ����
	void UpData(void);

private:
	// �E�B���h�E�N���X�Q��
	std::weak_ptr<Window>win;

	// �C���v�b�g�N���X�Q��
	std::weak_ptr<Input>input;

	// �@�\���x��
	D3D_FEATURE_LEVEL level;

	// �Q�ƌ���
	HRESULT result;

	// ��]�p�x
	FLOAT angle[2];

	// �s��f�[�^
	UINT8* data;

	// �R�}���h����̍\����
	Command command;

	// �X���b�v�`�F�C������̍\����
	Swap swap;

	// �t�F���X����̍\����
	Fence fence;

	// ���[�g�V�O�l�`������̍\����
	RootSignature rootsignature;

	// �p�C�v���C������̍\����
	PipeLine pipeline;

	// ���_�o�b�t�@�̐ݒ�p�\����
	D3D12_VERTEX_BUFFER_VIEW vertexView;

	// ���_�C���f�b�N�X�̐ݒ�p�\����
	D3D12_INDEX_BUFFER_VIEW indexView;

	// WVP�̐ݒ�p�\����
	WVP wvp;

	// �f�[�^����̍\���̂̔z��
	std::map<std::string, View>view;

	// �����_�[�^�[�Q�b�g�p���\�[�X�z��
	std::vector<ID3D12Resource*>renderTarget;
};