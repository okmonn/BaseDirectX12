#include "BMP.h"
#include "WICTextureLoader12.h"
#include <tchar.h>
#include<sstream>

#pragma comment (lib,"d3d12.lib")
#pragma comment (lib,"dxgi.lib")

//�C���X�^���X�ϐ��̏�����
BMP* BMP::s_Instance = nullptr;

// �R���X�g���N�^
BMP::BMP()
{
	//�Q�ƌ��ʂ̏�����
	result = S_OK;

	//BMP�f�[�^�\���̔z��̏�����
	data.clear();

	//WIC�̏�������
	CoInitialize(nullptr);

	//�f�R�[�h�f�[�^�z��̏�����
	decode.clear();

	//�T�u���\�[�X�z��̏�����
	sub.clear();


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
}

// �f�X�g���N�^
BMP::~BMP()
{
	//�f�[�^
	for (auto itr = data.begin(); itr != data.end(); ++itr)
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

	//�f�R�[�h�f�[�^
	for (auto itr = decode.begin(); itr != decode.end(); ++itr)
	{
		if (itr->second != nullptr)
		{
			itr->second.release();
		}
	}
}

// �C���X�^���X
void BMP::Create(void)
{
	if (s_Instance == nullptr)
	{
		s_Instance = new BMP;
	}
}

// �j��
void BMP::Destroy(void)
{
	if (s_Instance != nullptr)
	{
		delete s_Instance;
	}

	s_Instance = nullptr;
}

// BMP�f�[�^�̓ǂݍ���
HRESULT BMP::LoadBMP(USHORT index, std::string fileName, ID3D12Device* dev)
{
	//BMP�w�b�_�[�\����
	BITMAPINFOHEADER header;
	SecureZeroMemory(&header, sizeof(header));

	//BMP�t�@�C���w�b�_�[
	BITMAPFILEHEADER fileheader;
	SecureZeroMemory(&fileheader, sizeof(fileheader));

	//�_�~�[�錾
	const char* path = fileName.c_str();

	//�t�@�C��
	FILE *file;

	//�t�@�C���J�炭
	if ((fopen_s(&file, path, "rb")) != 0)
	{
		//�G���[�i���o�[�m�F
		auto a = (fopen_s(&file, path, "rb"));
		std::stringstream s;
		s << a;
		OutputDebugString(_T("�t�@�C�����J���܂���ł����F���s\n"));
		OutputDebugStringA(s.str().c_str());
		return S_FALSE;
	}

	//BMP�t�@�C���w�b�_�[�ǂݍ���
	fread(&fileheader, sizeof(fileheader), 1, file);

	//BMP�w�b�_�[�ǂݍ���
	fread(&header, sizeof(header), 1, file);

	//�摜�̕��ƍ����̕ۑ�
	data[index].size = { header.biWidth ,header.biHeight };

	//�f�[�^�T�C�Y���̃������m��(�r�b�g�̐[����32bit�̏ꍇ)
	//bmp.resize(header.biSizeImage);
	//�f�[�^�T�C�Y���̃������m��(�r�b�g�̐[����24bit�̏ꍇ)
	data[index].bmp.resize(header.biWidth * header.biHeight * 4);

	UCHAR* ptr = &data[index].bmp[0];

	for (int line = header.biHeight - 1; line >= 0; --line)
	{
		for (int count = 0; count < header.biWidth * 4; count += 4)
		{
			//��ԍ��̔z��ԍ�
			UINT address = line * header.biWidth * 4;
			data[index].bmp[address + count] = 0;
			fread(&data[index].bmp[address + count + 1], sizeof(UCHAR), 3, file);
		}
	}

	//�t�@�C�������
	fclose(file);

	if (CreateHeap(index, dev) == S_OK)
	{
		result = CreateResource(index, dev);
	}

	return result;
}

// WIC�e�N�X�`���̓ǂݍ���
HRESULT BMP::LoadTextureWIC(USHORT index, std::wstring fileName, ID3D12Device * dev)
{
	//�ǂݍ���
	result = DirectX::LoadWICTextureFromFile(dev, fileName.c_str(), &data[index].resource, decode[index], sub[index]);
	if (FAILED(result))
	{
		OutputDebugString(_T("WIC�e�N�X�`���̓ǂݍ��݁F���s\n"));

		return result;
	}

	result = CreateHeap(index, dev);
	if (FAILED(result))
	{
		return result;
	}

	//�V�F�[�_���\�[�X�r���[�ݒ�p�\���̂̐ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderDesc;
	SecureZeroMemory(&shaderDesc, sizeof(shaderDesc));
	shaderDesc.Format						= DXGI_FORMAT_R8G8B8A8_UNORM;
	shaderDesc.ViewDimension				= D3D12_SRV_DIMENSION_TEXTURE2D;
	shaderDesc.Texture2D.MipLevels			= 1;
	shaderDesc.Texture2D.MostDetailedMip	= 0;
	shaderDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//�q�[�v�̐擪�n���h�����擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle = data[index].heap->GetCPUDescriptorHandleForHeapStart();

	//�V�F�[�_�[���\�[�X�r���[�̐���
	dev->CreateShaderResourceView(data[index].resource, &shaderDesc, handle);

	return result;
}

// �q�[�v�̐���
HRESULT BMP::CreateHeap(USHORT index, ID3D12Device* dev)
{
	//�q�[�v�ݒ�p�\���̂̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask			= 0;
	heapDesc.NumDescriptors		= 2;
	heapDesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//�q�[�v����
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&data[index].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("�e�N�X�`���p�q�[�v�̐����F���s\n"));

		return result;
	}

	return result;
}

// ���\�[�X�̐���
HRESULT BMP::CreateResource(USHORT index, ID3D12Device * dev)
{
	//�q�[�v�X�e�[�g�ݒ�p�\���̂̐ݒ�
	D3D12_HEAP_PROPERTIES prop;
	SecureZeroMemory(&prop, sizeof(prop));
	prop.CPUPageProperty					= D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	prop.CreationNodeMask					= 1;
	prop.MemoryPoolPreference				= D3D12_MEMORY_POOL_L0;
	prop.Type								= D3D12_HEAP_TYPE_CUSTOM;
	prop.VisibleNodeMask					= 1;

	//���\�[�X�ݒ�p�\���̂̐ݒ�
	D3D12_RESOURCE_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.Dimension							= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width								= data[index].size.width;
	desc.Height								= data[index].size.height;
	desc.DepthOrArraySize					= 1;
	desc.Format								= DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count					= 1;
	desc.Flags								= D3D12_RESOURCE_FLAG_NONE;
	desc.Layout								= D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//���\�[�X����
	result = dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&data[index].resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("�e�N�X�`���p���\�[�X�̐����F���s\n"));

		return result;
	}

	//�V�F�[�_���\�[�X�r���[�ݒ�p�\���̂̐ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderDesc;
	SecureZeroMemory(&shaderDesc, sizeof(shaderDesc));
	shaderDesc.Format						= DXGI_FORMAT_R8G8B8A8_UNORM;
	shaderDesc.ViewDimension				= D3D12_SRV_DIMENSION_TEXTURE2D;
	shaderDesc.Texture2D.MipLevels			= 1;
	shaderDesc.Texture2D.MostDetailedMip	= 0;
	shaderDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//�q�[�v�̐擪�n���h�����擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle = data[index].heap->GetCPUDescriptorHandleForHeapStart();
	
	//�V�F�[�_�[���\�[�X�r���[�̐���
	dev->CreateShaderResourceView(data[index].resource, &shaderDesc, handle);

	return result;
}

// �`�揀��
void BMP::SetDraw(USHORT index, ID3D12GraphicsCommandList * list)
{
	//�{�b�N�X�ݒ�p�\���̂̐ݒ�
	D3D12_BOX box;
	SecureZeroMemory(&box, sizeof(box));
	box.back	= 1;
	box.bottom	= data[index].size.height;
	box.front	= 0;
	box.left	= 0;
	box.right	= data[index].size.width;
	box.top		= 0;

	//�T�u���\�[�X�ɏ�������
	result = data[index].resource->WriteToSubresource(0, &box, &data[index].bmp[0], (box.right * 4), (box.bottom * 4));
	if (FAILED(result))
	{
		OutputDebugString(_T("�T�u���\�[�X�ւ̏������݁F���s\n"));

		return;
	}

	//�q�[�v�̐擪�n���h�����擾
	D3D12_GPU_DESCRIPTOR_HANDLE handle = data[index].heap->GetGPUDescriptorHandleForHeapStart();

	//�q�[�v�̃Z�b�g
	list->SetDescriptorHeaps(1, &data[index].heap);

	//�f�B�X�N���v�^�[�e�[�u���̃Z�b�g
	list->SetGraphicsRootDescriptorTable(1, handle);
}

// WIC�p�`�揀��
void BMP::SetDrawWIC(USHORT index, ID3D12GraphicsCommandList * list)
{
	//���\�[�X�ݒ�p�\����
	D3D12_RESOURCE_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));

	desc = data[index].resource->GetDesc();

	//�{�b�N�X�ݒ�p�\���̂̐ݒ�
	D3D12_BOX box;
	SecureZeroMemory(&box, sizeof(box));
	box.back	= 1;
	box.bottom	= desc.Height;
	box.front	= 0;
	box.left	= 0;
	box.right	= (UINT)desc.Width;
	box.top		= 0;

	//�T�u���\�[�X�ɏ�������
	result = data[index].resource->WriteToSubresource(0, &box, decode[index].get(), sub[index].RowPitch, sub[index].SlicePitch);
	if (FAILED(result))
	{
		OutputDebugString(_T("WIC�T�u���\�[�X�ւ̏������݁F���s\n"));

		return;
	}

	//�q�[�v�̐擪�n���h�����擾
	D3D12_GPU_DESCRIPTOR_HANDLE handle = data[index].heap->GetGPUDescriptorHandleForHeapStart();

	//�q�[�v�̃Z�b�g
	list->SetDescriptorHeaps(1, &data[index].heap);

	//�f�B�X�N���v�^�[�e�[�u���̃Z�b�g
	list->SetGraphicsRootDescriptorTable(1, handle);
}

// �`��
void BMP::Draw(USHORT index, ID3D12GraphicsCommandList * list)
{
	//�g�|���W�[�ݒ�
	list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SetDraw(index, list);

	//�`��
	list->DrawInstanced(6, 1, 0, 0);
}