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


// �@�\���x���̈ꗗ
D3D_FEATURE_LEVEL levels[] =
{
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_0,
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0
};

// �R���X�g���N�^
Device::Device(std::weak_ptr<Window>winAdr, std::weak_ptr<Input>inputAdr)
{
	//�E�B���h�E�N���X�Q��
	win = winAdr;

	//�C���v�b�g�N���X�Q��
	input = inputAdr;

	//�@�\���x���̏�����
	level = D3D_FEATURE_LEVEL_11_0;

	//�Q�ƌ��ʂ̏�����
	result = S_OK;

	//��]�p�x�̏�����
	for (UINT i = 0; i < 2; i++)
	{
		angle[i] = 0.0f;
	}

	//�s��f�[�^
	data = nullptr;

	//�R�}���h����̍\���̂̏�����
	SecureZeroMemory(&command, sizeof(command));

	//�X���b�v�`�F�C������̍\���̂̏�����
	SecureZeroMemory(&swap, sizeof(swap));

	//�t�F���X����̍\���̂̏�����
	SecureZeroMemory(&fence, sizeof(fence));

	//���[�g�V�O�l�`������̍\���̂̏�����
	SecureZeroMemory(&rootsignature, sizeof(rootsignature));

	//�p�C�v���C������̍\���̂̏�����
	SecureZeroMemory(&pipeline, sizeof(pipeline));

	//���_�o�b�t�@�ݒ�p�\���̂̏�����
	SecureZeroMemory(&vertexView, sizeof(vertexView));

	//���_�C���f�b�N�X�r���[�ݒ�p�\���̂̏�����
	SecureZeroMemory(&indexView, sizeof(indexView));

	//WVP�ݒ�p�\���̂̏�����
	SecureZeroMemory(&wvp, sizeof(wvp));

	//�f�[�^����̍\���̔z��̏�����
	view.clear();

	//�����_�[�^�[�Q�b�g�p���\�[�X�z��̏�����
	renderTarget.clear();


	//�G���[���o�͂ɕ\��������
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

// �f�X�g���N�^
Device::~Device()
{
	//�萔�o�b�t�@�̃A���}�b�v
	view[mode[2]].resource->Unmap(0, nullptr);

	//�f�o�C�X
	if (command.dev != nullptr)
	{
		command.dev->Release();
	}

	//�R�}���h�A���P�[�^
	if (command.allocator != nullptr)
	{
		command.allocator->Release();
	}

	//�R�}���h���X�g
	if (command.list != nullptr)
	{
		command.list->Release();
	}

	//�R�}���h�L���[
	if (command.queue != nullptr)
	{
		command.queue->Release();
	}

	//�C���^�[�t�F�[�X�t�@�N�g���[
	if (swap.factory != nullptr)
	{
		swap.factory->Release();
	}

	//�X���b�v�`�F�C��
	if (swap.swapChain != nullptr)
	{
		swap.swapChain->Release();
	}

	//�t�F���X
	if (fence.fence != nullptr)
	{
		fence.fence->Release();
	}

	//�V���A���C�Y���b�Z�[�W
	if (rootsignature.signature != nullptr)
	{
		rootsignature.signature->Release();
	}

	//�V���A���C�Y�G���[���b�Z�[�W
	if (rootsignature.error != nullptr)
	{
		rootsignature.error->Release();
	}

	//���[�g�V�O�l�`��
	if (rootsignature.rootSignature != nullptr)
	{
		rootsignature.rootSignature->Release();
	}

	//���_���
	if (pipeline.vertex != nullptr)
	{
		pipeline.vertex->Release();
	}

	//�s�N�Z�����
	if (pipeline.pixel != nullptr)
	{
		pipeline.pixel->Release();
	}

	//�p�C�v���C��
	if (pipeline.pipeline != nullptr)
	{
		pipeline.pipeline->Release();
	}

	//�r���[
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

	//�����_�[�^�[�Q�b�g
	for (UINT i = 0; i < renderTarget.size(); i++)
	{
		if (renderTarget[i] != nullptr)
		{
			renderTarget[i]->Release();
		}
	}

	//PMD�N���X
	if (PMD::GetInstance() != nullptr)
	{
		PMD::Destroy();
	}

	//BMP�N���X
	if (BMP::GetInstance() != nullptr)
	{
		BMP::Destroy();
	}
}

// �r���[�s��̃Z�b�g
DirectX::XMMATRIX Device::SetViewMatrix(void)
{
	//�_�~�[�錾
	FLOAT pos = .0f;
	DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
	//�J�����̈ʒu
	DirectX::XMVECTOR eye		= { 0, pos,  -1 };
	//�J�����̏œ_
	DirectX::XMVECTOR target	= { 0, pos,   0 };
	//�J�����̏����
	DirectX::XMVECTOR upper		= { 0, 1,     0 };

	view = DirectX::XMMatrixLookAtLH(eye, target, upper);

	return view;
}

// �v���W�F�N�V�����s��̃Z�b�g
DirectX::XMMATRIX Device::SetProjectionMatrix(void)
{
	//�_�~�[�錾
	DirectX::XMMATRIX projection = DirectX::XMMatrixIdentity();

	projection = DirectX::XMMatrixPerspectiveFovLH(RAD(90), ((static_cast<FLOAT>(WINDOW_X) / static_cast<FLOAT>(WINDOW_Y))), 0.5f, 500.0f);
	
	return projection;
}

// WVP�̃Z�b�g
void Device::SetWVP(void)
{
	wvp.world = DirectX::XMMatrixIdentity();
	wvp.viewProjection = SetViewMatrix() * SetProjectionMatrix();
}

// �f�o�C�X�̐���
HRESULT Device::CreateDevice(void)
{
	for (auto& i : levels)
	{
		//�f�o�C�X����
		result = D3D12CreateDevice(nullptr, i, IID_PPV_ARGS(&command.dev));
		if (result == S_OK)
		{
			//�@�\���x���̌���
			level = i;
			break;
		}
	}

	return result;
}

// �R�}���h����̐���
HRESULT Device::CreateCommand(void)
{
	if (CreateDevice() == S_OK)
	{
		//�R�}���h�A���P�[�^����
		result = command.dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command.allocator));
		if (FAILED(result))
		{
			OutputDebugString(_T("�R�}���h�A���P�[�^�̐����F���s\n"));

			return result;
		}

		//�R�}���h���X�g����
		result = command.dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command.allocator, nullptr, IID_PPV_ARGS(&command.list));
		if (FAILED(result))
		{
			OutputDebugString(_T("�R�}���h���X�g�̐����F���s\n"));

			return result;
		}

		//�R�}���h���X�g�����
		command.list->Close();

		//�R�}���h�L���[�ݒ�p�\���̂̐ݒ�
		D3D12_COMMAND_QUEUE_DESC desc;
		SecureZeroMemory(&desc, sizeof(desc));
		desc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask	= 0;
		desc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Type		= D3D12_COMMAND_LIST_TYPE_DIRECT;

		//�R�}���h�L���[����
		result = command.dev->CreateCommandQueue(&desc, IID_PPV_ARGS(&command.queue));
		if (FAILED(result))
		{
			OutputDebugString(_T("�R�}���h�L���[�̐����F���s\n"));

			return result;
		}
	}
	else
	{
		OutputDebugString(_T("�f�o�C�X�̐����F���s\n"));

		return S_FALSE;
	}
	
	return result;
}

// �C���^�[�t�F�[�X�t�@�N�g���[�̐���
HRESULT Device::CreateFactory(void)
{
	//�C���^�[�t�F�[�X�t�@�N�g���[����
	result = CreateDXGIFactory1(IID_PPV_ARGS(&swap.factory));

	return result;
}

// �X���b�v�`�F�C���̐���
HRESULT Device::CreateSwapChain(void)
{
	if (CreateFactory() == S_OK)
	{
		//�X���b�v�`�F�C���ݒ�p�\���̂̐ݒ�
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

		//�X���b�v�`�F�C������
		result = swap.factory->CreateSwapChainForHwnd(command.queue, win.lock()->GetWindowHandle(), &desc, nullptr, nullptr, (IDXGISwapChain1**)(&swap.swapChain));
		if (FAILED(result))
		{
			OutputDebugString(_T("�X���b�v�`�F�C���̐����F���s\n"));

			return result;
		}

		//�o�b�N�o�b�t�@���ۑ�
		swap.bufferCnt = desc.BufferCount;
	}
	else
	{
		OutputDebugString(_T("�t�@�N�g���[�̐����F���s\n"));

		return S_FALSE;
	}

	return result;
}

// �����_�[�^�[�Q�b�g�̐���
HRESULT Device::CreateRenderTargetView(void)
{
	//�q�[�v�ݒ�p�\���̂̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask					= 0;
	heapDesc.NumDescriptors				= swap.bufferCnt;
	heapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	//�����_�[�^�[�Q�b�g(���_)�p�q�[�v����
	result = command.dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&view[mode[0]].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("�����_�[�^�[�Q�b�g�p�q�[�v�̐����F���s\n"));

		return result;
	}

	//�q�[�v�T�C�Y���擾
	view[mode[0]].size = command.dev->GetDescriptorHandleIncrementSize(heapDesc.Type);

	//�����_�[�^�[�Q�b�g(���_)�p�q�[�v�̐擪���擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle = view[mode[0]].heap->GetCPUDescriptorHandleForHeapStart();

	//�����_�[�^�[�Q�b�g�ݒ�p�\���̂̐ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC renderDesc;
	SecureZeroMemory(&renderDesc, sizeof(renderDesc));
	renderDesc.Format					= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	renderDesc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
	renderDesc.Texture2D.MipSlice		= 0;
	renderDesc.Texture2D.PlaneSlice		= 0;

	//�����_�[�^�[�Q�b�g���̃������m��
	renderTarget.resize(swap.bufferCnt);

	for (UINT i = 0; i < renderTarget.size(); i++)
	{
		//�o�b�t�@�̎擾
		result = swap.swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTarget[i]));
		if (FAILED(result))
		{
			continue;
		}

		//�����_�[�^�[�Q�b�g����
		command.dev->CreateRenderTargetView(renderTarget[i], &renderDesc, handle);

		//�n���h���̈ʒu���ړ�
		handle.ptr += view[mode[0]].size;
	}

	return result;
}

// �[�x�X�e���V���̐���
HRESULT Device::CreateDepthStencil(void)
{
	//�q�[�v�ݒ�p�\���̂̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask					= 0;
	heapDesc.NumDescriptors				= swap.bufferCnt;
	heapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	//�[�x�X�e���V���p�q�[�v����
	result = command.dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&view[mode[1]].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("�[�x�X�e���V���p�q�[�v�̐����F���s\n"));

		return result;
	}

	//�q�[�v�T�C�Y���擾
	view[mode[1]].size = command.dev->GetDescriptorHandleIncrementSize(heapDesc.Type);

	//�q�[�v�v���p�e�B�ݒ�p�\���̂̐ݒ�
	D3D12_HEAP_PROPERTIES prop;
	SecureZeroMemory(&prop, sizeof(prop));
	prop.CPUPageProperty				= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.CreationNodeMask				= 1;
	prop.MemoryPoolPreference			= D3D12_MEMORY_POOL_UNKNOWN;
	prop.Type							= D3D12_HEAP_TYPE_DEFAULT;
	prop.VisibleNodeMask				= 1;

	//���\�[�X�ݒ�p�\���̂̐ݒ�
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

	//�N���A�l�ݒ�p�\���̂̐ݒ�
	D3D12_CLEAR_VALUE clearValue;
	SecureZeroMemory(&clearValue, sizeof(clearValue));
	clearValue.Format					= DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth		= 1.0f;
	clearValue.DepthStencil.Stencil		= 0;

	//�[�x�X�e���V���p���\�[�X����
	result = command.dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&view[mode[1]].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("�[�x�X�e���V���p���\�[�X�̐����F���s\n"));

		return result;
	}

	//�[�x�X�e���V���r���[�ݒ�p�\���̂̐ݒ�
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	SecureZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format						= DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension				= D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags						= D3D12_DSV_FLAG_NONE;

	//�[�x�X�e���V���r���[����
	command.dev->CreateDepthStencilView(view[mode[1]].resource,&dsvDesc, view[mode[1]].heap->GetCPUDescriptorHandleForHeapStart());

	return result;
}

// �t�F���X�̐���
HRESULT Device::CreateFence(void)
{
	if (command.dev == nullptr)
	{
		return S_FALSE;
	}

	//�t�F���X����
	result = command.dev->CreateFence(fence.fenceCnt, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence.fence));
	if (FAILED(result))
	{
		OutputDebugString(_T("�t�F���X�̐����F���s\n"));

		return result;
	}

	//�t�F���X�l�̍X�V
	fence.fenceCnt = 1;

	//�t�F���X�C�x���g����
	fence.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (fence.fenceEvent == nullptr)
	{
		OutputDebugString(_T("�t�F���X�C�x���g�̐����F���s\n"));

		return S_FALSE;
	}

	return result;
}

// ���[�g�V�O�l�`���̃V���A���C�Y
HRESULT Device::SerializeRootSignature(void)
{
	// �f�B�X�N���v�^�����W�̐ݒ�.
	D3D12_DESCRIPTOR_RANGE range[4];
	SecureZeroMemory(&range, sizeof(range));

	//���[�g�p�����[�^�̐ݒ�.
	D3D12_ROOT_PARAMETER param[4];
	SecureZeroMemory(&param, sizeof(param));

	//�萔�o�b�t�@�p
	range[0].RangeType								= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[0].NumDescriptors							= 1;
	range[0].BaseShaderRegister						= 0;
	range[0].RegisterSpace							= 0;
	range[0].OffsetInDescriptorsFromTableStart		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	param[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[0].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	param[0].DescriptorTable.NumDescriptorRanges	= 1;
	param[0].DescriptorTable.pDescriptorRanges		= &range[0];

	//�e�N�X�`���p
	range[1].RangeType								= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[1].NumDescriptors							= 1;
	range[1].BaseShaderRegister						= 0;
	range[1].RegisterSpace							= 0;
	range[1].OffsetInDescriptorsFromTableStart		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	param[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;
	param[1].DescriptorTable.NumDescriptorRanges	= 1;
	param[1].DescriptorTable.pDescriptorRanges		= &range[1];

	//�}�e���A���p
	range[2].RangeType								= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[2].NumDescriptors							= 1;
	range[2].BaseShaderRegister						= 1;
	range[2].RegisterSpace							= 0;
	range[2].OffsetInDescriptorsFromTableStart		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	param[2].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[2].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	param[2].DescriptorTable.NumDescriptorRanges	= 1;
	param[2].DescriptorTable.pDescriptorRanges		= &range[2];

	//�{�[���p
	range[3].RangeType								= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[3].NumDescriptors							= 1;
	range[3].BaseShaderRegister						= 2;
	range[3].RegisterSpace							= 0;
	range[3].OffsetInDescriptorsFromTableStart		= D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	param[3].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[3].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	param[3].DescriptorTable.NumDescriptorRanges	= 1;
	param[3].DescriptorTable.pDescriptorRanges		= &range[3];

	//�ÓI�T���v���[�̐ݒ�
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

	//���[�g�V�O�l�`���ݒ�p�\���̂̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.NumParameters								= _countof(param);
	desc.pParameters								= param;
	desc.NumStaticSamplers							= 1;
	desc.pStaticSamplers							= &sampler;
	desc.Flags										= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//���[�g�V�O�l�`���̃V���A���C�Y��
	result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootsignature.signature, &rootsignature.error);
	
	return result;
}

// ���[�g�V�O�l�`���̐���
HRESULT Device::CreateRootSignature(void)
{
	if (SerializeRootSignature() == S_OK)
	{
		//���[�g�V�O�l�`������
		result = command.dev->CreateRootSignature(0, rootsignature.signature->GetBufferPointer(), rootsignature.signature->GetBufferSize(), IID_PPV_ARGS(&rootsignature.rootSignature));
		if (FAILED(result))
		{
			OutputDebugString(_T("���[�g�V�O�l�`���̐����F���s\n"));

			return result;
		}
	}
	else
	{
		OutputDebugString(_T("�V���A���C�Y���F���s\n"));

		return S_FALSE;
	}

	return result;
}

// �e�N�X�`���p�V�F�[�_�[�R���p�C��
HRESULT Device::ShaderCompileTexture(void)
{
	//���_�V�F�[�_�̃R���p�C��
	result = D3DCompileFromFile(_T("shader.hlsl"), nullptr, nullptr, "TextureVS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pipeline.vertex, &rootsignature.error);
	if (FAILED(result))
	{
		OutputDebugString(_T("���_�V�F�[�_�R���p�C���F���s\n"));

		return result;
	}

	//�s�N�Z���V�F�[�_�̃R���p�C��
	result = D3DCompileFromFile(_T("shader.hlsl"), nullptr, nullptr, "TexturePS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pipeline.pixel, &rootsignature.error);
	if (FAILED(result))
	{
		OutputDebugString(_T("�s�N�Z���V�F�[�_�R���p�C���F���s\n"));

		return result;
	}

	return result;
}

// ���f���p�V�F�[�_�[�R���p�C��
HRESULT Device::ShaderCompileModel(void)
{
	//���_�V�F�[�_�̃R���p�C��
	result = D3DCompileFromFile(_T("shader.hlsl"), nullptr, nullptr, "ModelVS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pipeline.vertex, &rootsignature.error);
	if (FAILED(result))
	{
		OutputDebugString(_T("���_�V�F�[�_�R���p�C���F���s\n"));

		return result;
	}

	//�s�N�Z���V�F�[�_�̃R���p�C��
	result = D3DCompileFromFile(_T("shader.hlsl"), nullptr, nullptr, "ModelPS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pipeline.pixel, &rootsignature.error);
	if (FAILED(result))
	{
		OutputDebugString(_T("�s�N�Z���V�F�[�_�R���p�C���F���s\n"));

		return result;
	}

	return result;
}

// �p�C�v���C���̐���
HRESULT Device::CreatePipeLine(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
	//���_���C�A�E�g�ݒ�p�\���̂̐ݒ�
	D3D12_INPUT_ELEMENT_DESC input[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BORN",     0, DXGI_FORMAT_R16G16_UINT,     0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHT",   0, DXGI_FORMAT_R8_UINT,         0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	//���X�^���C�U�[�X�e�[�g�ݒ�p�\���̂̐ݒ�
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

	//�����_�[�^�[�Q�b�g�u�����h�ݒ�p�\����
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

	//�u�����h�X�e�[�g�ݒ�p�\����
	D3D12_BLEND_DESC descBS;
	descBS.AlphaToCoverageEnable			= FALSE;
	descBS.IndependentBlendEnable			= FALSE;
	for (UINT i = 0; i < swap.bufferCnt; i++)
	{
		descBS.RenderTarget[i]				= renderBlend;
	}

	//�p�C�v���C���X�e�[�g�ݒ�p�\����
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

	//�p�C�v���C������
	result = command.dev->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline.pipeline));
	if (FAILED(result))
	{
		OutputDebugString(_T("�p�C�v���C���̐����F���s\n"));

		return result;
	}

	return result;
}

// �e�N�X�`���p���_�o�b�t�@�̐���
HRESULT Device::CreateVertexBufferTexture(void)
{
	//�O�p�`�̒��_���W(�ォ�玞�v���)
	Vertex tran[] =
	{
		{ { -1.0f / 2.0f,  1.0f / 2.0f,	0.0f }, {0, 0} },	//����
		{ {  1.0f / 2.0f,  1.0f / 2.0f,	0.0f }, {1, 0} },	//�E��
		{ {  1.0f / 2.0f, -1.0f / 2.0f,	0.0f }, {1, 1} },	//�E��

		{ {  1.0f / 2.0f, -1.0f / 2.0f,	0.0f }, {1, 1} },	//�E��
		{ { -1.0f / 2.0f, -1.0f / 2.0f, 0.0f }, {0, 1} },	//����
		{ { -1.0f / 2.0f,  1.0f / 2.0f,	0.0f }, {0, 0} }	//����
	};

	//���_�p���\�[�X����
	result = command.dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(tran)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&view[mode[0]].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("���_�o�b�t�@�p���\�[�X�̐����F���s\n"));

		return result;
	}

	//���M�p�f�[�^
	UCHAR* data = nullptr;

	//���M�͈�
	D3D12_RANGE range = { 0,0 };

	//�}�b�s���O
	result = view[mode[0]].resource->Map(0, &range, reinterpret_cast<void**>(&data));
	if (FAILED(result))
	{
		OutputDebugString(_T("���_�p���\�[�X�̃}�b�s���O�F���s\n"));

		return result;
	}

	//���_�f�[�^�̃R�s�[
	memcpy(data, &tran, (sizeof(tran)));

	//�A���}�b�s���O
	view[mode[0]].resource->Unmap(0, nullptr);

	//���_�o�b�t�@�ݒ�p�\���̂̐ݒ�
	vertexView.BufferLocation	= view[mode[0]].resource->GetGPUVirtualAddress();
	vertexView.SizeInBytes		= sizeof(tran);
	vertexView.StrideInBytes	= sizeof(Vertex);

	return result;
}

// ���f���p���_�o�b�t�@�̐���
HRESULT Device::CreateVertexBufferModel(void)
{
	//���_�p���\�[�X����
	result = command.dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(VERETX) * PMD::GetInstance()->GetVertex().size()), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&view[mode[0]].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("���_�o�b�t�@�p���\�[�X�̐����F���s\n"));

		return result;
	}

	//���M�p�f�[�^
	UCHAR* data = nullptr;

	//���M�͈�
	D3D12_RANGE range = { 0,0 };

	//�}�b�s���O
	result = view[mode[0]].resource->Map(0, &range, reinterpret_cast<void**>(&data));
	if (FAILED(result))
	{
		OutputDebugString(_T("���_�o�b�t�@�p���\�[�X�̃}�b�s���O�F���s\n"));

		return result;
	}

	//���_�f�[�^�̃R�s�[
	memcpy(data, &PMD::GetInstance()->GetVertex()[0], (sizeof(VERETX) * PMD::GetInstance()->GetVertex().size()));

	//�A���}�b�s���O
	view[mode[0]].resource->Unmap(0, nullptr);

	//���_�o�b�t�@�ݒ�p�\���̂̐ݒ�
	vertexView.BufferLocation	= view[mode[0]].resource->GetGPUVirtualAddress();
	vertexView.SizeInBytes		= sizeof(VERETX) * PMD::GetInstance()->GetVertex().size();
	vertexView.StrideInBytes	= sizeof(VERETX);

	//�C���f�b�N�X
	result = command.dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(USHORT) * PMD::GetInstance()->GetIndex().size()), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&view[mode[0]].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("�C���f�b�N�X�o�b�t�@�p���\�[�X�̐����F���s\n"));

		return result;
	}

	//���M�p�f�[�^
	UCHAR* idata = nullptr;

	//�}�b�s���O
	result = view[mode[0]].resource->Map(0, &range, reinterpret_cast<void**>(&data));
	if (FAILED(result))
	{
		OutputDebugString(_T("�C���f�b�N�X�o�b�t�@�p���\�[�X�̃}�b�s���O�F���s\n"));

		return result;
	}

	//�R�s�[
	memcpy(data, &PMD::GetInstance()->GetIndex()[0], sizeof(USHORT) * PMD::GetInstance()->GetIndex().size());

	//�A���}�b�s���O
	view[mode[0]].resource->Unmap(0, nullptr);

	//���_�C���f�b�N�X�r���[�ݒ�p�\���̂̐ݒ�
	indexView.BufferLocation	= view[mode[0]].resource->GetGPUVirtualAddress();
	indexView.SizeInBytes		= sizeof(USHORT) * PMD::GetInstance()->GetIndex().size();
	indexView.Format			= DXGI_FORMAT_R16_UINT;

	return result;
}

// �萔�o�b�t�@�̐���
HRESULT Device::CreateConstantBuffer(void)
{
	//�萔�o�b�t�@�ݒ�p�\���̂̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.NumDescriptors					= 2;
	heapDesc.Flags							= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type							= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	
	//�q�[�v����
	result = command.dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&view[mode[2]].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("�萔�o�b�t�@�p�q�[�v�̐����F���s\n"));

		return S_FALSE;
	}

	//�q�[�v�T�C�Y���擾
	view[mode[2]].size = command.dev->GetDescriptorHandleIncrementSize(heapDesc.Type);

	//�q�[�v�ݒ�p�\���̂̐ݒ�
	D3D12_HEAP_PROPERTIES prop;
	SecureZeroMemory(&prop, sizeof(prop));
	prop.Type								= D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty					= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference				= D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask					= 1;
	prop.VisibleNodeMask					= 1;

	//���\�[�X�ݒ�p�\���̂̐ݒ�
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

	//���\�[�X����
	result = command.dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&view[mode[2]].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("�萔�o�b�t�@�p���\�[�X�̐����F���s\n"));

		return result;
	}

	//�萔�o�b�t�@�r���[�ݒ�p�\���̂̐ݒ�
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.BufferLocation						= view[mode[2]].resource->GetGPUVirtualAddress();
	desc.SizeInBytes						= (sizeof(WVP) + 0xff) &~0xff;

	//�萔�o�b�t�@�r���[����
	command.dev->CreateConstantBufferView(&desc, view[mode[2]].heap->GetCPUDescriptorHandleForHeapStart());

	//���M�͈�
	D3D12_RANGE range = { 0, 0 };
	
	//�}�b�s���O
	result = view[mode[2]].resource->Map(0, &range, (void**)(&data));
	if (FAILED(result))
	{
		OutputDebugString(_T("�萔�o�b�t�@�p���\�[�X�̃}�b�s���O�F���s\n"));

		return result;
	}

	//�R�s�[
	memcpy(data, &wvp, sizeof(DirectX::XMMATRIX));

	return result;
}

// ��������
void Device::Init(void)
{
	//WVP
	SetWVP();

	//�R�}���h
	result = CreateCommand();
	if (FAILED(result))
	{
		return;
	}

	//�X���b�v�`�F�C��
	result = CreateSwapChain();
	if (FAILED(result))
	{
		return;
	}

	//�����_�[�^�[�Q�b�g
	result = CreateRenderTargetView();
	if (FAILED(result))
	{
		return;
	}

	//�[�x�X�e���V��
	result = CreateDepthStencil();
	if (FAILED(result))
	{
		return;
	}

	//�t�F���X
	result = CreateFence();
	if (FAILED(result))
	{
		return;
	}

	//���[�g�V�O�l�`��
	result = CreateRootSignature();
	if (FAILED(result))
	{
		return;
	}

	//�萔�o�b�t�@
	result = CreateConstantBuffer();
	if (FAILED(result))
	{
		return;
	}


	TextInit();
	//ModelInit();
}

// �e�N�X�`���p�̏�������
void Device::TextInit(void)
{
	/*if (BMP::GetInstance() == nullptr)
	{
		//BMP�N���X�̃C���X�^���X
		BMP::Create();
	}

	//BMP�ǂݍ���
	result = BMP::GetInstance()->LoadBMP(0, "sample/texturesample24bit.bmp", command.dev);
	if (FAILED(result))
	{
		return;
	}*/

	//�V�F�[�_�[
	result = ShaderCompileTexture();
	if (FAILED(result))
	{
		return;
	}

	//�p�C�v���C��
	result = CreatePipeLine(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	if (FAILED(result))
	{
		return;
	}

	//���_�f�[�^
	result = CreateVertexBufferTexture();
	if (FAILED(result))
	{
		return;
	}
}

// ���f���p�̏�����
void Device::ModelInit(void)
{
	if (PMD::GetInstance() == nullptr)
	{
		//PMD�N���X�̃C���X�^���X�C���X�^���X
		PMD::Create();
	}

	//PMD�ǂݍ���
	result = PMD::GetInstance()->LoadPMD("MikuMikuDance/UserFile/Model/�����~�N.pmd", command.dev);
	if (FAILED(result))
	{
		return;
	}

	//�{�[���̉�]
	//PMD::GetInstance()->BornRotation("����", DirectX::XMMatrixRotationZ(RAD(60)));

	//�V�F�[�_�[
	result = ShaderCompileModel();
	if (FAILED(result))
	{
		return;
	}

	//�p�C�v���C��
	result = CreatePipeLine(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	//result = CreatePipeLine(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
	if (FAILED(result))
	{
		return;
	}

	//���_�f�[�^
	result = CreateVertexBufferModel();
	if (FAILED(result))
	{
		return;
	}
}

// �ҋ@����
void Device::Wait(void)
{
	//�t�F���X�l�X�V
	fence.fenceCnt++;

	//�t�F���X�l��ύX
	result = command.queue->Signal(fence.fence, fence.fenceCnt);
	if (FAILED(result))
	{
		OutputDebugString(_T("�t�F���X�l�̍X�V�F���s\n"));

		return;
	}

	//������ҋ@(�|�[�����O)
	while (fence.fence->GetCompletedValue() != fence.fenceCnt)
	{
		/*auto a = fence.fenceCnt;
		std::stringstream s;
		s << a;
		OutputDebugStringA(s.str().c_str());*/

		//�t�F���X�C�x���g�̃Z�b�g
		result = fence.fence->SetEventOnCompletion(fence.fenceCnt, fence.fenceEvent);
		if (FAILED(result))
		{
			OutputDebugString(_T("�t�F���X�C�x���g�̃Z�b�g�F���s\n"));

			return;
		}

		//�t�F���X�C�x���g�̑ҋ@
		WaitForSingleObject(fence.fenceEvent, INFINITE);
	}
}

// �r���[�|�[�g�̃Z�b�g
D3D12_VIEWPORT Device::SetViewPort(void)
{
	//�r���[�|�[�g�ݒ�p�\���̂̐ݒ�
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

// �V�U�[�̃Z�b�g
RECT Device::SetScissor(void)
{
	//�V�U�[�ݒ�p�\���̂̐ݒ�
	RECT scissor;
	SecureZeroMemory(&scissor, sizeof(scissor));
	scissor.left	= 0;
	scissor.right	= WINDOW_X;
	scissor.top		= 0;
	scissor.bottom	= WINDOW_Y;

	return scissor;
}

// �o���A�̐ݒu
void Device::Barrier(D3D12_RESOURCE_STATES befor, D3D12_RESOURCE_STATES affter)
{
	//�o���A�ݒ�p�\���̂̐ݒ�
	D3D12_RESOURCE_BARRIER barrier;
	ZeroMemory(&barrier, sizeof(barrier));
	barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource	= renderTarget[swap.swapChain->GetCurrentBackBufferIndex()];
	barrier.Transition.StateBefore	= befor;
	barrier.Transition.StateAfter	= affter;
	barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_FLAG_NONE;

	//�o���A�ݒu
	command.list->ResourceBarrier(1, &barrier);
}

// �萔�o�b�t�@�̃Z�b�g
void Device::SetConstantBuffer(void)
{
	//�萔�o�b�t�@�q�[�v�̐擪�n���h�����擾
	D3D12_GPU_DESCRIPTOR_HANDLE handle = view[mode[2]].heap->GetGPUDescriptorHandleForHeapStart();

	//�萔�o�b�t�@�q�[�v�̃Z�b�g
	command.list->SetDescriptorHeaps(1, &view[mode[2]].heap);

	//�萔�o�b�t�@�f�B�X�N���v�^�[�e�[�u���̃Z�b�g
	command.list->SetGraphicsRootDescriptorTable(0, handle);
}

// �����_�[�^�[�Q�b�g�̃Z�b�g
void Device::SetRenderTarget(void)
{
	//���_�q�[�v�̐擪�n���h���̎擾
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(view[mode[0]].heap->GetCPUDescriptorHandleForHeapStart(), swap.swapChain->GetCurrentBackBufferIndex(), view[mode[0]].size);

	//�����_�[�^�[�Q�b�g�̃Z�b�g
	command.list->OMSetRenderTargets(1, &handle, false, &view[mode[1]].heap->GetCPUDescriptorHandleForHeapStart());

	//�N���A�J���[�̎w��
	const FLOAT clearColor[] = { 1.0f,0.0f,1.0f,1.0f };
	//�����_�[�^�[�Q�b�g�̃N���A
	command.list->ClearRenderTargetView(handle, clearColor, 0, nullptr);
}

// �[�x�X�e���V���r���[�̃N���A
void Device::ClearDepthStencil(void)
{
	//�[�x�X�e���V���q�[�v�̐擪�n���h���̎擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle_DSV = view[mode[1]].heap->GetCPUDescriptorHandleForHeapStart();

	//�[�x�X�e���V���r���[�̃N���A
	command.list->ClearDepthStencilView(handle_DSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

// ����
void Device::UpData(void)
{
	if (input.lock()->InputKey(DIK_RIGHT) == TRUE)
	{
		//��]
		angle[0]++;
		//�s��X�V
		wvp.world = DirectX::XMMatrixRotationY(RAD(angle[0]));
	}
	else if (input.lock()->InputKey(DIK_LEFT) == TRUE)
	{
		//��]
		angle[0]--;
		//�s��X�V
		wvp.world = DirectX::XMMatrixRotationY(RAD(angle[0]));
	}
	
	//�s��f�[�^�X�V
	memcpy(data, &wvp, sizeof(WVP));
	

	//�R�}���h�A���P�[�^�̃��Z�b�g
	command.allocator->Reset();
	//���X�g�̃��Z�b�g
	command.list->Reset(command.allocator, pipeline.pipeline);

	//���[�g�V�O�l�`���̃Z�b�g
	command.list->SetGraphicsRootSignature(rootsignature.rootSignature);

	//�p�C�v���C���̃Z�b�g
	command.list->SetPipelineState(pipeline.pipeline);

	SetConstantBuffer();

	//�r���[�̃Z�b�g
	command.list->RSSetViewports(1, &SetViewPort());

	//�V�U�[�̃Z�b�g
	command.list->RSSetScissorRects(1, &SetScissor());

	// Present ---> RenderTarget
	Barrier(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	SetRenderTarget();

	ClearDepthStencil();
	
	//���_�o�b�t�@�r���[�̃Z�b�g
	command.list->IASetVertexBuffers(0, 1, &vertexView);

	// �`��
	command.list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	command.list->DrawInstanced(6, 1, 0, 0);
	
	// RenderTarget ---> Present
	Barrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	//�R�}���h���X�g�̋L�^�I��
	command.list->Close();

	//���X�g�̔z��
	ID3D12CommandList *commandList[] = { command.list };
	//�z��łȂ��ꍇ�Fqueue->ExecuteCommandLists(1, (ID3D12CommandList*const*)&list);
	command.queue->ExecuteCommandLists(_countof(commandList), commandList);

	//���A�\��ʂ𔽓]
	swap.swapChain->Present(1, 0);

	Wait();
}
