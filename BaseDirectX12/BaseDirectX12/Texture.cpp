#include "Texture.h"
#include "d3dx12.h"
#include<sstream>
#include <tchar.h>

#pragma comment (lib,"d3d12.lib")
#pragma comment (lib,"dxgi.lib")

//�C���X�^���X�ϐ��̏�����
Texture* Texture::s_Instance = nullptr;

// �R���X�g���N�^
Texture::Texture()
{
	//�Q�ƌ���
	result = S_OK;

	//�f�[�^�̏�����
	data.clear();

	//���_�o�b�t�@�r���[�\���̂̏�����
	SecureZeroMemory(&view, sizeof(view));


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
Texture::~Texture()
{
	//�f�[�^
	for (auto itr = data.begin(); itr != data.end(); ++itr)
	{
		if (itr->second.texture->resorce != nullptr)
		{
			itr->second.texture->resorce->Release();
		}
		if (itr->second.texture->heap != nullptr)
		{
			itr->second.texture->heap->Release();
		}
	}
}

// �C���X�^���X��
void Texture::Create(void)
{
	if (s_Instance == nullptr)
	{
		s_Instance = new Texture;
	}
}

// �j��
void Texture::Destroy(void)
{
	if (s_Instance != nullptr)
	{
		delete s_Instance;
	}

	s_Instance = nullptr;
}

// �摜�f�[�^�̓ǂݍ���
HRESULT Texture::LoadBMP(USHORT index, std::string fileName, UINT swapBufferNum, ID3D12Device* dev)
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
	data[index].size = { header.biWidth, header.biHeight };

	//�f�[�^�T�C�Y���̃������m��(�r�b�g�̐[����32bit�̏ꍇ)
	//bmp.resize(header.biSizeImage);
	//�f�[�^�T�C�Y���̃������m��(�r�b�g�̐[����24bit�̏ꍇ)
	data[index].bmp.resize(header.biWidth * header.biHeight * 4);

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

	if (CreateVertexHeap(index, swapBufferNum, dev) == S_OK)
	{
		result = CreateVertexBuffer(index, dev);
	}

	if (CreateConstantHeap(index, dev) == S_OK)
	{
		result = CreateConstantBuffer(index, dev);
	}

	return result;
}

// ���_�q�[�v�̐���
HRESULT Texture::CreateVertexHeap(USHORT index, UINT swapBufferNum, ID3D12Device* dev)
{
	//�q�[�v�ݒ�p�\���̂̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = swapBufferNum;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	//�����_�[�^�[�Q�b�g(���_)�p�q�[�v����
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&data[index].texture[0].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("���_�p�q�[�v�̐����F���s\n"));

		return result;
	}

	return result;
}

// ���_�o�b�t�@�̐���
HRESULT Texture::CreateVertexBuffer(USHORT index, ID3D12Device* dev)
{
	//�z��̃������m��
	data[index].vertex.resize(6);

	//���_�p���\�[�X����
	result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * data[index].vertex.size()), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&data[index].texture[0].resorce));
	if (FAILED(result))
	{
		OutputDebugString(_T("���_�o�b�t�@�p���\�[�X�̐����F���s\n"));

		return result;
	}

	//���M�p�f�[�^
	

	//���M�͈�
	D3D12_RANGE range = { 0,0 };

	//�}�b�s���O
	result = data[index].texture[0].resorce->Map(0, &range, reinterpret_cast<void**>(&d));
	if (FAILED(result))
	{
		OutputDebugString(_T("���_�p���\�[�X�̃}�b�s���O�F���s\n"));

		return result;
	}

	//���_�f�[�^�̃R�s�[
	memcpy(d, &data[index].vertex, (sizeof(Vertex) * data[index].vertex.size()));

	//�A���}�b�s���O
	data[index].texture[0].resorce->Unmap(0, nullptr);

	//���_�o�b�t�@�ݒ�p�\���̂̐ݒ�
	view.BufferLocation = data[index].texture[0].resorce->GetGPUVirtualAddress();
	view.SizeInBytes = sizeof(Vertex) * data[index].vertex.size();
	view.StrideInBytes = sizeof(Vertex);

	return result;
}

// �萔�q�[�v�̐���
HRESULT Texture::CreateConstantHeap(USHORT index, ID3D12Device * dev)
{//�q�[�v�ݒ�p�\���̂̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//�q�[�v����
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&data[index].texture[1].heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("�e�N�X�`���p�q�[�v�̐����F���s\n"));

		return result;
	}

	return result;
}

// �萔�o�b�t�@�̐���
HRESULT Texture::CreateConstantBuffer(USHORT index, ID3D12Device * dev)
{
	//�q�[�v�X�e�[�g�ݒ�p�\���̂̐ݒ�
	D3D12_HEAP_PROPERTIES prop;
	SecureZeroMemory(&prop, sizeof(prop));
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	prop.CreationNodeMask = 1;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	prop.Type = D3D12_HEAP_TYPE_CUSTOM;
	prop.VisibleNodeMask = 1;

	//���\�[�X�ݒ�p�\���̂̐ݒ�
	D3D12_RESOURCE_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = data[index].size.width;
	desc.Height = data[index].size.height;
	desc.DepthOrArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//���\�[�X����
	result = dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&data[index].texture[1].resorce));
	if (FAILED(result))
	{
		OutputDebugString(_T("�e�N�X�`���p���\�[�X�̐����F���s\n"));

		return result;
	}

	//�V�F�[�_���\�[�X�r���[�ݒ�p�\���̂̐ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderDesc;
	SecureZeroMemory(&shaderDesc, sizeof(shaderDesc));
	shaderDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	shaderDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	shaderDesc.Texture2D.MipLevels = 1;
	shaderDesc.Texture2D.MostDetailedMip = 0;
	shaderDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//�q�[�v�̐擪�n���h�����擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle = data[index].texture[1].heap->GetCPUDescriptorHandleForHeapStart();

	//�V�F�[�_�[���\�[�X�r���[�̐���
	dev->CreateShaderResourceView(data[index].texture[1].resorce, &shaderDesc, handle);

	return result;
}

// ���_�ݒ�
HRESULT Texture::SetVertex(USHORT index, Vector2 pos, Vector2 size, Vector2 wSize)
{
	//���_�ݒ�
	data[index].vertex[0] = { { -(pos.x / (wSize.x / 2)),             (pos.y / (wSize.y / 2)),	          0.0f },{ 0.0f, 0.0f } };	//����
	data[index].vertex[1] = { {  ((pos.x + size.x) / (wSize.x )),     (pos.y / (wSize.y / 2)),	          0.0f },{ 1.0f, 0.0f } };	//�E��
	data[index].vertex[2] = { {  ((pos.x + size.x) / (wSize.x )),    -((pos.y + size.y) / (wSize.y )), 0.0f },{ 1.0f, 1.0f } };	//�E��
	data[index].vertex[3] = { {  ((pos.x + size.x) / (wSize.x )),    -((pos.y + size.y) / (wSize.y )), 0.0f },{ 1.0f, 1.0f } };	//�E��
	data[index].vertex[4] = { { -(pos.x / (wSize.x / 2))           , -((pos.y + size.y) / (wSize.y)), 0.0f },{ 0.0f, 1.0f } };	//����
	data[index].vertex[5] = { { -(pos.x / (wSize.x / 2))           ,  (pos.y / (wSize.y / 2)),	          0.0f },{ 0.0f, 0.0f } };  //����

	//���M�p�f�[�^
	

	//���M�͈�
	D3D12_RANGE range = { 0,0 };

	//�}�b�s���O
	result = data[index].texture[0].resorce->Map(0, &range, reinterpret_cast<void**>(&d));
	if (FAILED(result))
	{
		OutputDebugString(_T("���_�p���\�[�X�̃}�b�s���O�F���s\n"));

		return result;
	}

	//���_�f�[�^�̃R�s�[
	memcpy(d, &data[index].vertex, (sizeof(Vertex) * data[index].vertex.size()));

	//�A���}�b�s���O
	data[index].texture[0].resorce->Unmap(0, nullptr);

	//���_�o�b�t�@�ݒ�p�\���̂̐ݒ�
	view.BufferLocation = data[index].texture[0].resorce->GetGPUVirtualAddress();
	view.SizeInBytes = sizeof(Vertex) * data[index].vertex.size();
	view.StrideInBytes = sizeof(Vertex);

	return result;
}

// �`��
HRESULT Texture::Draw(USHORT index, Vector2 pos, Vector2 size, Vector2 wSize, ID3D12GraphicsCommandList * list)
{
	result = SetVertex(index, pos, size, wSize);
	if (FAILED(result))
	{
		OutputDebugString(_T("���_���̃Z�b�g�F���s\n"));

		return result;
	}

	//���_�o�b�t�@�r���[�̃Z�b�g
	list->IASetVertexBuffers(0, 1, &view);

	//�g�|���W�[�ݒ�
	list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//�{�b�N�X�ݒ�p�\���̂̐ݒ�
	D3D12_BOX box;
	SecureZeroMemory(&box, sizeof(box));
	box.back = 1;
	box.bottom = data[index].size.height;
	box.front = 0;
	box.left = 0;
	box.right = data[index].size.width;
	box.top = 0;

	//�T�u���\�[�X�ɏ�������
	result = data[index].texture[1].resorce->WriteToSubresource(0, &box, &data[index].bmp[0], (box.right * 4), (box.bottom * 4));
	if (FAILED(result))
	{
		OutputDebugString(_T("�T�u���\�[�X�ւ̏������݁F���s\n"));

		return result;
	}

	//�q�[�v�̐擪�n���h�����擾
	D3D12_GPU_DESCRIPTOR_HANDLE handle = data[index].texture[1].heap->GetGPUDescriptorHandleForHeapStart();

	//�q�[�v�̃Z�b�g
	list->SetDescriptorHeaps(1, &data[index].texture[1].heap);

	//�f�B�X�N���v�^�[�e�[�u���̃Z�b�g
	list->SetGraphicsRootDescriptorTable(1, handle);

	//�`��
	list->DrawInstanced(6, 1, 0, 0);

	return result;
}
