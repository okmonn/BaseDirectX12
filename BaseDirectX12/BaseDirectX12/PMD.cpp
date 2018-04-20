#include "PMD.h"
#include "BMP.h"
#include <tchar.h>
#include<sstream>
#include <algorithm>

#pragma comment (lib,"d3d12.lib")
#pragma comment (lib,"dxgi.lib")

//�C���X�^���X�ϐ��̏�����
PMD* PMD::s_Instance = nullptr;

// �R���X�g���N�^
PMD::PMD()
{
	//�Q�ƌ��ʂ̏�����
	result = S_OK;

	//���M�f�[�^�̏�����
	data = nullptr;

	//�{�[���p���M�f�[�^
	bornData = nullptr;

	//�}�e���A���\���̂̏�����
	SecureZeroMemory(&mat, sizeof(mat));

	//�f�[�^����̍\���̂̏�����
	SecureZeroMemory(&view, sizeof(view));
	
	//�{�[���p�f�[�^����̍\���̂̏�����
	SecureZeroMemory(&bornView, sizeof(bornView));

	//PMD�w�b�_�[�\���̂̏�����
	SecureZeroMemory(&header, sizeof(header));

	//PMD���_�f�[�^�\���̔z��̏�����
	vertex.clear();

	//PMD�C���f�b�N�X�f�[�^�z��̏�����
	index.clear();

	//PMD�}�e���A���f�[�^�z��̏�����
	material.clear();

	//PMD�{�[���f�[�^�z��̏�����
	born.clear();

	//�{�[�����W�z��̏�����
	pos.clear();

	//�{�[���m�[�h�z��̏�����
	node.clear();

	//�{�[�����z��̏�����
	map.clear();

	//�{�[���s��z��̏�����
	matrix.clear();

	//BMP�N���X�̃C���X�^���X
	if (BMP::GetInstance() == nullptr)
	{
		BMP::Create();
	}


	//VMD�w�b�_�[�\���̂̏�����
	SecureZeroMemory(&vmd_header, sizeof(vmd_header));

	//VMD���[�V�����\���̔z��̏�����
	motion.clear();

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
PMD::~PMD()
{
	//�f�[�^
	if (view.heap != nullptr)
	{
		view.heap->Release();
	}
	if (view.resource != nullptr)
	{
		//�A���}�b�v
		view.resource->Unmap(0, nullptr);

		view.resource->Release();
	}

	//�{�[���p�f�[�^
	if (bornView.heap != nullptr)
	{
		bornView.heap->Release();
	}
	if (bornView.resource != nullptr)
	{
		//�A���}�b�v
		bornView.resource->Unmap(0, nullptr);

		bornView.resource->Release();
	}

	//BMP�N���X
	if (BMP::GetInstance() != nullptr)
	{
		BMP::Destroy();
	}
}

// �C���X�^���X
void PMD::Create(void)
{
	if (s_Instance == nullptr)
	{
		s_Instance = new PMD;
	}
}

// �j��
void PMD::Destroy(void)
{
	if (s_Instance != nullptr)
	{
		delete s_Instance;
	}

	s_Instance = nullptr;
}

// �t�H���_�[�Ƃ̘A��
std::string PMD::FolderPath(std::string path, const char * textureName)
{
	//�_�~�[�錾
	int pathIndex1 = path.rfind('/');
	int pathIndex2 = path.rfind('\\');
	int pathIndex = max(pathIndex1, pathIndex2);

	std::string folderPath = path.substr(0, pathIndex);
	folderPath += "/";
	folderPath += textureName;

	return folderPath;
}

// ���j�R�[�h�ϊ�
std::wstring PMD::ChangeUnicode(const CHAR * str)
{
	//�������̎擾
	auto byteSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str, -1, nullptr, 0);

	std::wstring wstr;
	wstr.resize(byteSize);

	//�ϊ�
	byteSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str, -1, &wstr[0], byteSize);

	return wstr;
}

// PMD�ǂݍ���
HRESULT PMD::LoadPMD(std::string fileName, ID3D12Device * dev)
{
	//�t�@�C��
	FILE *file;

	//�_�~�[�錾
	const char* path = fileName.c_str();

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

	//�w�b�_�[�̓ǂݍ���
	fread(&header, sizeof(header), 1, file);

	//���_�f�[�^�z��̃������T�C�Y�m��
	vertex.resize(header.vertexNum);

	//���_�f�[�^�̓ǂݍ���
	for (auto& v : vertex)
	{
		fread(&v.pos,        sizeof(v.pos),        1, file);
		fread(&v.normal,     sizeof(v.normal),     1, file);
		fread(&v.uv,         sizeof(v.uv),         1, file);
		fread(&v.bornNum,    sizeof(v.bornNum),    1, file);
		fread(&v.bornWeight, sizeof(v.bornWeight), 1, file);
		fread(&v.edge,       sizeof(v.edge),       1, file);
	}

	//�C���f�b�N�X���i�[�p
	UINT indexNum = 0;

	//�C���f�b�N�X���̓ǂݍ���
	fread(&indexNum, sizeof(UINT), 1, file);

	//�C���f�b�N�X�f�[�^�z��̃������T�C�Y�m��
	index.resize(indexNum);

	for (UINT i = 0; i < indexNum; i++)
	{
		//�C���f�b�N�X�f�[�^�̓ǂݍ���
		fread(&index[i], sizeof(USHORT), 1, file);
	}

	//�}�e���A�����i�[�p
	UINT materialNum = 0;

	//�}�e���A�����̓ǂݍ���
	fread(&materialNum, sizeof(UINT), 1, file);

	//�}�e���A���f�[�^�z��̃������T�C�Y�m��
	material.resize(materialNum);

	//�}�e���A���f�[�^�̓ǂݍ���
	fread(&material[0], sizeof(Material), materialNum, file);

	//�{�[�����i�[�p
	UINT bornNum = 0;

	//�{�[�����̓ǂݍ���
	fread(&bornNum, sizeof(USHORT), 1, file);

	//�{�[���f�[�^�z��̃������T�C�Y�m��
	born.resize(bornNum);

	//�{�[���f�[�^�̓ǂݍ���
	for (auto& b : born)
	{
		fread(&b.name,                 sizeof(b.name), 1, file);
		fread(&b.parent_born_index,    sizeof(b.parent_born_index),    1, file);
		fread(&b.tail_born_index,      sizeof(b.tail_born_index),      1, file);
		fread(&b.type,                 sizeof(b.type),                 1, file);
		fread(&b.ik_parent_born_index, sizeof(b.ik_parent_born_index), 1, file);
		fread(&b.pos,                  sizeof(b.pos),                  1, file);
	}

	//�t�@�C�������
	fclose(file);


	//SetTexture(dev);
	SetTextureWIC(dev);

	SetBorn();

	result = CreateConstantBuffer(dev);
	result = LoadVMD("MikuMikuDance/UserFile/Motion/pose.vmd");

	return result;
}

// �e�N�X�`���ǂݍ���
void PMD::SetTexture(ID3D12Device* dev)
{
	for (UINT i = 0; i < material.size(); i++)
	{
		if (material[i].textureFilePath[0] != '\0')
		{
			//�ǂݍ���
			result = BMP::GetInstance()->LoadBMP(i, FolderPath("MikuMikuDance/UserFile/Model/", material[i].textureFilePath), dev);
			if (FAILED(result))
			{
				OutputDebugString(_T("�e�N�X�`���̓ǂݍ��݁F���s\n"));
			}
		}
	}
}

// WIC�e�N�X�`���ǂݍ���
void PMD::SetTextureWIC(ID3D12Device * dev)
{
	for (UINT i = 0; i < material.size(); i++)
	{
		if (material[i].textureFilePath[0] != '\0')
		{
			result = BMP::GetInstance()->LoadTextureWIC(i, ChangeUnicode(FolderPath("MikuMikuDance/UserFile/Model/", material[i].textureFilePath).c_str()), dev);
			if (FAILED(result))
			{
				OutputDebugString(_T("�e�N�X�`���̓ǂݍ��݁F���s\n"));
			}
		}
	}
}

// �{�[���̃Z�b�g
void PMD::SetBorn(void)
{
	//�{�[�����W�z��̃������T�C�Y�m��
	pos.resize(born.size());

	//�{�[���m�[�h�z��̃������T�C�Y�m��
	node.resize(born.size());

	//�{�[���s��z��̃������T�C�Y�m��
	matrix.resize(born.size());

	//�{�[���s��̑S�z��̏�����
	std::fill(matrix.begin(), matrix.end(), DirectX::XMMatrixIdentity());

	for (UINT i = 0; i < born.size(); i++)
	{
		//�{�[�����i�[
		map[born[i].name] = i;

		if (born[i].tail_born_index != 0)
		{
			//�擪���W�i�[
			pos[i].head = born[i].pos;
			//�������W�i�[
			pos[i].tail = born[born[i].tail_born_index].pos;
		}

		if (born[i].parent_born_index != 0xffff)
		{
			//�Q�ƃ{�[���i�[
			node[born[i].parent_born_index].index.push_back(i);
		}
	}
}

// �{�[���̉�]
void PMD::BornRotation(std::string name, const DirectX::XMMATRIX matrix)
{
	//�{�[���m�[�h�z��ɉ����Ȃ��Ƃ�
	if (node.empty())
	{
		return;
	}

	//�_�~�[�錾
	DirectX::XMVECTOR head = DirectX::XMLoadFloat3(&pos[map[name]].head);
	DirectX::XMVECTOR tmp = DirectX::XMVectorSet(-pos[map[name]].head.x, -pos[map[name]].head.y, -pos[map[name]].head.z, 0);
	DirectX::XMVECTOR tail = DirectX::XMLoadFloat3(&pos[map[name]].tail);
	DirectX::XMMATRIX mat = DirectX::XMMatrixTranslationFromVector(tmp);

	mat *= matrix;
	mat *= DirectX::XMMatrixTranslationFromVector(head);

	tail = DirectX::XMVector3Transform(tail, mat);

	DirectX::XMStoreFloat3(&pos[map[name]].tail, tail);

	this->matrix[map[name]] = mat;

	for (auto &child : node[map[name]].index)
	{
		MoveBorn(child, mat);
	}

	//�R�s�[
	memcpy(bornData, &this->matrix[0], (sizeof(DirectX::XMMATRIX) * this->matrix.size() + 0xff) &~0xff);
}

// �q�{�[���̉�]
void PMD::MoveBorn(USHORT index, const DirectX::XMMATRIX & matrix)
{
	//�{�[���m�[�h�z��ɉ����Ȃ��Ƃ�
	if (node.empty() || index == 0)
	{
		return;
	}

	DirectX::XMVECTOR head = DirectX::XMLoadFloat3(&pos[index].head);
	head = DirectX::XMVector3Transform(head, matrix);
	DirectX::XMStoreFloat3(&pos[index].head, head);

	DirectX::XMVECTOR tail = DirectX::XMLoadFloat3(&pos[index].tail);
	tail = DirectX::XMVector3Transform(tail, matrix);
	DirectX::XMStoreFloat3(&pos[index].tail, tail);

	this->matrix[index] = matrix;

	for (auto child : node[index].index)
	{
		MoveBorn(child, matrix);
	}
}

// �萔�o�b�t�@�̐���
HRESULT PMD::CreateConstantBuffer(ID3D12Device * dev)
{
	//�萔�o�b�t�@�ݒ�p�\���̂̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	SecureZeroMemory(&heapDesc, sizeof(heapDesc));
	heapDesc.NumDescriptors			= 2 + material.size();
	heapDesc.Flags					= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type					= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//�q�[�v����
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&view.heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("���f���p�q�[�v�̐����F���s\n"));

		return result;
	}

	//�q�[�v�T�C�Y���擾
	view.size = dev->GetDescriptorHandleIncrementSize(heapDesc.Type);

	//�q�[�v�ݒ�p�\���̂̐ݒ�
	D3D12_HEAP_PROPERTIES prop;
	SecureZeroMemory(&prop, sizeof(prop));
	prop.Type						= D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference		= D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask			= 1;
	prop.VisibleNodeMask			= 1;

	//���\�[�X�ݒ�p�\���̂̐ݒ�
	D3D12_RESOURCE_DESC resourceDesc;
	SecureZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width				= ((sizeof(Mat) + 0xff) &~0xff) * ((sizeof(Mat) + 0xff) &~0xff);
	resourceDesc.Height				= 1;
	resourceDesc.DepthOrArraySize	= 1;
	resourceDesc.MipLevels			= 1;
	resourceDesc.Format				= DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count	= 1;
	resourceDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//���\�[�X����
	result = dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&view.resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("���f���p���\�[�X�̐����F���s\n"));

		return result;
	}

	//�萔�o�b�t�@�r���[�ݒ�p�\���̂̐ݒ�
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
	SecureZeroMemory(&desc, sizeof(desc));
	desc.SizeInBytes				= (sizeof(Mat) + 0xff) &~0xff;

	auto address = view.resource->GetGPUVirtualAddress();
	auto handle = view.heap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < material.size(); i++)
	{
		desc.BufferLocation			= address;

		//�萔�o�b�t�@�r���[����
		dev->CreateConstantBufferView(&desc, handle);

		address += desc.SizeInBytes;

		handle.ptr += view.size;
	}

	//���M�͈�
	D3D12_RANGE range = { 0, 0 };

	//�}�b�s���O
	result = view.resource->Map(0, &range, (void**)(&data));
	if (FAILED(result))
	{
		OutputDebugString(_T("���f���p���\�[�X�̃}�b�s���O�F���s\n"));

		return result;
	}

	//�R�s�[
	memcpy(data, &mat, sizeof(DirectX::XMMATRIX));


	//�{�[��
	//�萔�o�b�t�@�ݒ�p�\���̂̐ݒ�
	heapDesc.NumDescriptors			= 2;
	heapDesc.Flags					= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type					= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//�q�[�v����
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&bornView.heap));
	if (FAILED(result))
	{
		OutputDebugString(_T("�{�[���p�q�[�v�̐����F���s\n"));

		return result;
	}

	//�q�[�v�T�C�Y���擾
	bornView.size = dev->GetDescriptorHandleIncrementSize(heapDesc.Type);

	//�q�[�v�ݒ�p�\���̂̐ݒ�
	prop.Type						= D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference		= D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask			= 1;
	prop.VisibleNodeMask			= 1;

	//���\�[�X�ݒ�p�\���̂̐ݒ�
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width				= ((sizeof(DirectX::XMMATRIX) * matrix.size() + 0xff) &~0xff);
	resourceDesc.Height				= 1;
	resourceDesc.DepthOrArraySize	= 1;
	resourceDesc.MipLevels			= 1;
	resourceDesc.Format				= DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count	= 1;
	resourceDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//���\�[�X����
	result = dev->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&bornView.resource));
	if (FAILED(result))
	{
		OutputDebugString(_T("�{�[���p���\�[�X�̐����F���s\n"));

		return result;
	}

	//�萔�o�b�t�@�r���[�ݒ�p�\���̂̐ݒ�
	desc.BufferLocation				= bornView.resource->GetGPUVirtualAddress();
	desc.SizeInBytes				= (sizeof(DirectX::XMMATRIX) * matrix.size() + 0xff) &~0xff;

	//�萔�o�b�t�@�r���[����
	dev->CreateConstantBufferView(&desc, bornView.heap->GetCPUDescriptorHandleForHeapStart());

	//�}�b�s���O
	result = bornView.resource->Map(0, &range, (void**)(&bornData));
	if (FAILED(result))
	{
		OutputDebugString(_T("�{�[���p���\�[�X�̃}�b�s���O�F���s\n"));

		return result;
	}

	//�R�s�[
	memcpy(bornData, &matrix[0], (sizeof(DirectX::XMMATRIX) * matrix.size() + 0xff) &~0xff);

	return result;
}

// �`��
void PMD::Draw(ID3D12GraphicsCommandList * list, D3D12_INDEX_BUFFER_VIEW indexView)
{
	//�q�[�v�̃Z�b�g
	list->SetDescriptorHeaps(1, &bornView.heap);

	//�f�B�X�N���v�^�[�e�[�u���̃Z�b�g
	list->SetGraphicsRootDescriptorTable(3, bornView.heap->GetGPUDescriptorHandleForHeapStart());

	//���_�C���f�b�N�X�r���[�̃Z�b�g
	list->IASetIndexBuffer(&indexView);

	//�g�|���W�[�ݒ�
	list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//�`��
	//�I�t�Z�b�g
	UINT offset = 0;

	//���M�p�f�[�^
	UINT8* dummy = data;

	//�q�[�v�̐擪�n���h�����擾
	D3D12_GPU_DESCRIPTOR_HANDLE handle = view.heap->GetGPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < material.size(); i++)
	{
		//��{�F�i�[
		mat.diffuse = material[i].diffuse;

		//�e�N�X�`���Ή��`�F�b�N
		mat.existTexture = (material[i].textureFilePath[0] != '\0');

		if (mat.existTexture)
		{
			//�e�N�X�`���K��
			//BMP::GetInstance()->SetDraw(i, list);
			BMP::GetInstance()->SetDrawWIC(i, list);
		}

		//�q�[�v�̃Z�b�g
		list->SetDescriptorHeaps(1, &view.heap);

		//�f�B�X�N���v�^�[�e�[�u���̃Z�b�g
		list->SetGraphicsRootDescriptorTable(2, handle);

		//�R�s�[
		memcpy(dummy, &mat, sizeof(Mat));

		//�`��
		list->DrawIndexedInstanced(material[i].indexNum, 1, offset, 0, 0);

		//�n���h���X�V
		handle.ptr += view.size;

		//�f�[�^�X�V
		dummy = (UINT8*)(((sizeof(Mat) + 0xff) &~0xff) + (CHAR*)(dummy));

		//�I�t�Z�b�g�X�V
		offset += material[i].indexNum;
	}
}

// ���_�f�[�^�̎擾
std::vector<VERETX> PMD::GetVertex(void)
{
	return vertex;
}

// ���_�C���f�b�N�X�f�[�^�̎擾
std::vector<USHORT> PMD::GetIndex(void)
{
	return index;
}

// VMD�ǂݍ���
HRESULT PMD::LoadVMD(std::string fileName)
{
	//�t�@�C��
	FILE *file;

	//�_�~�[�錾
	const char* path = fileName.c_str();

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

	//�w�b�_�[�̓ǂݍ���
	fread(&vmd_header, sizeof(vmd_header), 1, file);

	//���[�V�����f�[�^���i�[�p
	DWORD motionNum = 0;

	//���[�V�����f�[�^���̓ǂݍ���
	fread(&motionNum, sizeof(DWORD), 1, file);

	//���[�V�����f�[�^�z��̃������m��
	motion.resize(motionNum);

	for (auto& m : motion)
	{
		fread(&m.bornName, sizeof(m.bornName), 1, file);
		fread(&m.frameNo, sizeof(m.frameNo), 1, file);
		fread(&m.location, sizeof(m.location), 1, file);
		fread(&m.rotatation, sizeof(m.rotatation), 1, file);
		fread(&m.interpolation, sizeof(m.interpolation), 1, file);
	}

	//�t�@�C�������
	fclose(file);

	//Motion();

	return S_OK;
}

// ���[�V����
void PMD::Motion(void)
{
	for (auto& pose : motion)
	{
		DirectX::XMVECTOR vec = DirectX::XMLoadFloat4(&pose.rotatation);
		Motion(pose.bornName, vec);
	}
}

void PMD::Motion(std::string name, DirectX::XMVECTOR quaternion)
{
	//�_�~�[�錾
	DirectX::XMVECTOR head = DirectX::XMLoadFloat3(&pos[map[name]].head);
	DirectX::XMVECTOR tmp = DirectX::XMVectorSet(-pos[map[name]].head.x, -pos[map[name]].head.y, -pos[map[name]].head.z, 0);
	DirectX::XMVECTOR tail = DirectX::XMLoadFloat3(&pos[map[name]].tail);
	DirectX::XMMATRIX mat = DirectX::XMMatrixTranslationFromVector(tmp);

	mat *= DirectX::XMMatrixRotationQuaternion(quaternion);
	mat *= DirectX::XMMatrixTranslationFromVector(head);

	tail = DirectX::XMVector3Transform(tail, mat);

	DirectX::XMStoreFloat3(&pos[map[name]].tail, tail);

	this->matrix[map[name]] = mat;

	for (auto &child : node[map[name]].index)
	{
		MoveBorn(child, mat);
	}

	//�R�s�[
	memcpy(bornData, &this->matrix[0], (sizeof(DirectX::XMMATRIX) * this->matrix.size() + 0xff) &~0xff);
}

void PMD::Motion(USHORT index, DirectX::XMMATRIX matrix)
{
	DirectX::XMVECTOR head = DirectX::XMLoadFloat3(&pos[index].head);
	head = DirectX::XMVector3Transform(head, matrix);
	DirectX::XMStoreFloat3(&pos[index].head, head);

	DirectX::XMVECTOR tail = DirectX::XMLoadFloat3(&pos[index].tail);
	tail = DirectX::XMVector3Transform(tail, matrix);
	DirectX::XMStoreFloat3(&pos[index].tail, tail);

	this->matrix[index] = matrix;

	for (auto child : node[index].index)
	{
		MoveBorn(child, matrix);
	}
}
