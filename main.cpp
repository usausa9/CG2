#pragma region include��

#include "DirectXInput.h" // �N���X����
#include "Vector2.h"
#include "Vector3.h"
#include "Matrix4.h"
#include "WinAPI.h"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <vector>
#include <string>
#include <DirectXMath.h> 
#include <DirectXTex.h>

using namespace DirectX;

#include <d3dcompiler.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

#pragma endregion

// �萔�o�b�t�@�p �f�[�^�\���� (�}�e���A��)
struct ConstBufferDataMaterial
{
	XMFLOAT4 color; // �F(RGBA)
};

// �萔�o�b�t�@�p�f�[�^�\���� (3D�ϊ��s��)
struct ConstBufferDataTransform
{
	XMMATRIX mat; // 3D�ϊ��s��
};

// 3D�I�u�W�F�N�g�^
struct Object3d
{
	// �萔�o�b�t�@ (�s��p)
	ID3D12Resource* constBuffTransform;

	// �萔�o�b�t�@�}�b�v (�s��p)
	ConstBufferDataTransform* constMapTransform;

	// �A�t�B���ϊ����
	XMFLOAT3 scale = { 1,1,1 };
	XMFLOAT3 rotation = { 0,0,0 };
	XMFLOAT3 position = { 0,0,0 };

	// ���[���h�ϊ��s��
	XMMATRIX matWorld;

	// �e�I�u�W�F�N�g�ւ̃|�C���^
	Object3d* parent = nullptr;
};

// 3D�I�u�W�F�N�g������
void InitializeObject3d(Object3d* object, ID3D12Device* device);

// 3D�I�u�W�F�N�g�X�V����
void UpdateObject3d(Object3d* object, XMMATRIX& matView, XMMATRIX& matProjection);

// 3D�I�u�W�F�N�g�`�揈��
void DrawObject3D(Object3d* object, ID3D12GraphicsCommandList* commandList, D3D12_VERTEX_BUFFER_VIEW& vbView, D3D12_INDEX_BUFFER_VIEW& ibView, UINT numIndices);

// Windows�A�v���ł̃G���g���[�|�C���g (main�֐�)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#pragma region WindowsAPI������
	WinAPI window;	// �E�B���h�E�N���X
	window.Set();
	window.CreateWindowObj();
	window.Show();
#pragma endregion

#pragma region DirectX����������
// DirectX���������� ��������

#ifdef _DEBUG
// �f�o�b�O���C���[���I����
	ID3D12Debug* debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
	}
#endif

	HRESULT result;
	ID3D12Device* device = nullptr;
	IDXGIFactory7* dxgiFactory = nullptr;
	IDXGISwapChain4* swapChain = nullptr;
	ID3D12CommandAllocator* commandAllocator = nullptr;
	ID3D12GraphicsCommandList* commandList = nullptr;
	ID3D12CommandQueue* commandQueue = nullptr;
	ID3D12DescriptorHeap* rtvHeap = nullptr;

	// DXGI�t�@�N�g���[�̐���
	result = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(result));

	// �A�_�v�^�[�̗񋓗p
	std::vector<IDXGIAdapter4*> adapters;

	// �����ɓ���̖��O�����A�_�v�^�[�I�u�W�F�N�g������
	IDXGIAdapter4* tmpAdapter = nullptr;

	// �p�t�H�[�}���X���������̂��珇�ɁA�S�ẴA�_�v�^�[��񋓂���
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference
	(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND;
		i++)
	{
		// ���I�z��ɒǉ�����
		adapters.push_back(tmpAdapter);
	}

	// �Ó��ȃA�_�v�^��I�ʂ���
	for (size_t i = 0; i < adapters.size(); i++)
	{
		DXGI_ADAPTER_DESC3 adapterDesc;

		// �A�_�v�^�[�̏����擾����
		adapters[i]->GetDesc3(&adapterDesc);

		// �\�t�g�E�F�A�f�o�C�X�����
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			// �f�o�C�X���̗p���ă��[�v�𔲂���
			tmpAdapter = adapters[i];
			break;
		}
	}

	// �Ή����x���̔z��
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D_FEATURE_LEVEL featureLevel;
	for (size_t i = 0; i < _countof(levels); i++)
	{
		// �̗p�����A�_�v�^�[�Ńf�o�C�X�𐶐�
		result = D3D12CreateDevice(tmpAdapter, levels[i],
			IID_PPV_ARGS(&device));

		if (result == S_OK)
		{
			// �f�o�C�X�𐶐��ł������_�Ń��[�v�𔲂���
			featureLevel = levels[i];
			break;
		}
	}

	// �R�}���h�A���P�[�^�𐶐�
	result = device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(result));

	// �R�}���h���X�g�𐶐�
	result = device->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator, nullptr,
		IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(result));

	// �R�}���h�L���[�̐ݒ�
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};

	// �R�}���h�L���[�𐶐�
	result = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	assert(SUCCEEDED(result));

	// �X���b�v�`�F�[���̐ݒ�
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = 1280;
	swapChainDesc.Height = 720;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // �F���̏���
	swapChainDesc.SampleDesc.Count = 1; // �}���`�T���v�����Ȃ�
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER; // �o�b�N�o�b�t�@�p
	swapChainDesc.BufferCount = 2; // �o�b�t�@����2�ɐݒ�
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // �t���b�v��͔j��
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// �X���b�v�`�F�[���̐���
	result = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue, window.hwnd, &swapChainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)&swapChain);
	assert(SUCCEEDED(result));

	// �f�X�N���v�^�q�[�v�̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // �����_�[�^�[�Q�b�g�r���[
	rtvHeapDesc.NumDescriptors = swapChainDesc.BufferCount; // ���\��2��

	// �f�X�N���v�^�q�[�v�̐���
	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

	// �[�x�o�b�t�@ ���\�[�X�ݒ�
	D3D12_RESOURCE_DESC depthResourceDesc{};
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Width = window.width;		// �����_�[�^�[�Q�b�g�ɍ��킹��
	depthResourceDesc.Height = window.height;	// �����_�[�^�[�Q�b�g�ɍ��킹��
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;	// �[�x�l�t�H�[�}�b�g
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// �f�v�X�X�e���V��

	// �[�x�l�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES depthHeapProp{};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

	// �[�x�l�̃N���A�ݒ�
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;		// �[�x�l1.0f(�ő�l)�ŃN���A
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;	// �[�x�l�t�H�[�}�b�g

	// ���\�[�X����
	ID3D12Resource* depthBuff = nullptr;
	result = device->CreateCommittedResource(
		&depthHeapProp,	// �q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc,// ���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&depthBuff));
	assert(SUCCEEDED(result));

	// �[�x�r���[�p�f�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;						// �[�x�r���[��1��
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// �f�v�X�X�e���V���r���[
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	// �[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // �[�x�l�t�H�[�}�b�g
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(
		depthBuff,
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// �o�b�N�o�b�t�@
	std::vector<ID3D12Resource*> backBuffers;
	backBuffers.resize(swapChainDesc.BufferCount);

	// �X���b�v�`�F�[���̑S�Ẵo�b�t�@�ɂ��ď�������
	for (size_t i = 0; i < backBuffers.size(); i++)
	{
		// �X���b�v�`�F�[������o�b�t�@���擾
		swapChain->GetBuffer((UINT)i, IID_PPV_ARGS(&backBuffers[i]));

		// �f�X�N���v�^�q�[�v�̃n���h�����擾
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

		// �����\���ŃA�h���X�������
		rtvHandle.ptr += i * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);

		// �����_�[�^�[�Q�b�g�r���[�̐ݒ�
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

		// �V�F�[�_�[�̌v�Z���ʂ�SRGB�ɕϊ����ď�������
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		// �����_�[�^�[�Q�b�g�r���[�̐���
		device->CreateRenderTargetView(backBuffers[i], &rtvDesc, rtvHandle);
	}

	// �t�F���X�̐���
	ID3D12Fence* fence = nullptr;
	UINT64 fenceVal = 0;
	result = device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	// DirectX���������� �����܂�

	// DirectInput�̏�����
	DirectXInput _key;
	_key.InputInit(result, window.w, window.hwnd); // ��Input�N���X :: �C�j�V�����C�Y

#pragma endregion
#pragma region �`�揉��������

// �q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES cbHeapProp{};
	cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD; // GPU�̓]���p

	// ���\�[�X�ݒ�
	D3D12_RESOURCE_DESC cbResourceDesc{};
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Width = (sizeof(ConstBufferDataMaterial) + 0xff) & ~0xff; // 256�o�C�g�A���C�������g
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* constBuffMaterial = nullptr;
	
	// �萔�o�b�t�@�̐���
	result = device->CreateCommittedResource(
		&cbHeapProp,	// �q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDesc,// ���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuffMaterial));
	assert(SUCCEEDED(result));

	// �萔�o�b�t�@�̃}�b�s���O
	ConstBufferDataMaterial* constMapMaterial = nullptr;
	result = constBuffMaterial->Map(0, nullptr, (void**)&constMapMaterial); // �}�b�s���O
	assert(SUCCEEDED(result));
	
	////�萔�o�b�t�@�̐���
	//ID3D12Resource* constBuffTransform0 = nullptr;
	//ConstBufferDataTransform* constMapTransform0 = nullptr;

	//ID3D12Resource* constBuffTransform1 = nullptr;
	//ConstBufferDataTransform* constMapTransform1 = nullptr;

	// 3D�I�u�W�F�N�g�̐�
	const size_t kObjectCount = 50;

	// 3D�I�u�W�F�N�g�̔z��
	Object3d object3ds[kObjectCount];

	// �z����̑S�I�u�W�F�N�g�ɑ΂���
	for (int i = 0; i < _countof(object3ds); i++)	{

		// ������
		InitializeObject3d(&object3ds[i], device);

		// �������火�͐e�q�\���̃T���v��
		if ( i > 0 ) { // �擪�ȊO�Ȃ�

			// ��O�̃I�u�W�F�N�g��e�I�u�W�F�N�g�Ƃ���
			object3ds[i].parent = &object3ds[ i - 1 ];

			// �e�I�u�W�F�N�g��9���̑傫��
			object3ds[i].scale = { 0.9f,0.9f,0.9f };

			// �e�I�u�W�F�N�g�ɑ΂���Z�������30�x��]
			object3ds[i].rotation = { 0.0f,0.0f,XMConvertToRadians(30.0f) };

			// �e�I�u�W�F�N�g�ɑ΂���Z������-8.0f���炷
			object3ds[i].position = { 0.0f,0.0f,-8.0f };
		}
	}

	
	{
		// �q�[�v�ݒ�
		D3D12_HEAP_PROPERTIES cbHeapProp{};
		cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
		
		// ���\�[�X�ݒ�
		D3D12_RESOURCE_DESC cbResourceDesc{};
		cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		cbResourceDesc.Width = (sizeof(ConstBufferDataTransform) + 0xff) & ~0xff;
		cbResourceDesc.Height = 1;
		cbResourceDesc.DepthOrArraySize = 1;
		cbResourceDesc.MipLevels = 1;
		cbResourceDesc.SampleDesc.Count = 1;
		cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	}

	//// �P�ʍs�����
	//constMapTransform->mat = XMMatrixIdentity();

	//constMapTransform->mat.r[0].m128_f32[0] = 2.0f / window_width;
	//constMapTransform->mat.r[1].m128_f32[1] = -2.0f / window_height;
	//constMapTransform->mat.r[3].m128_f32[0] = -1.0f;
	//constMapTransform->mat.r[3].m128_f32[1] = 1.0f;
	/*constMapTransform->mat = XMMatrixOrthographicOffCenterLH(0.0f, window_width, window_height, 0.0f, 0.0f, 1.0f);*/

	// �������e�ϊ��s��̌v�Z
	XMMATRIX matProjection =
	XMMatrixPerspectiveFovLH(
		XMConvertToRadians(45.0f),	// �㉺��p45�x
		(float)window.width / window.height,
		0.1f, 1000.0f
	);

	// �r���[�ϊ��s��
	XMMATRIX matView;
	XMFLOAT3 eye(0, 0, -100);	// ���_���W
	XMFLOAT3 target(0, 0, 0);	// �����_���W
	XMFLOAT3 up(0, 1, 0);		// ������x�N�g��
	matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	// �f�X�N���v�^�����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE descriptorRange{};
	descriptorRange.NumDescriptors = 1;		// ��x�̕`��Ɏg���e�N�X�`����1���Ȃ̂�1
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange.BaseShaderRegister = 0;	// �e�N�X�`�����W�X�^0��
	descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �l���������ނƎ����I�ɓ]�������
	constMapMaterial->color = XMFLOAT4(1, 1, 1, 1.0f); // RGBA�Ŕ������̐�

	// ���[�g�p�����[�^�̐ݒ�
	D3D12_ROOT_PARAMETER rootParams[3] = {};

	// �萔�o�b�t�@0�� 
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;		// ���
	rootParams[0].Descriptor.ShaderRegister = 0;						// �萔�o�b�t�@�̔ԍ�
	rootParams[0].Descriptor.RegisterSpace = 0;							// �f�t�H���g�l
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;		// �S�ẴV�F�[�_���猩����

	// �e�N�X�`�����W�X�^0��
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	// ���
	rootParams[1].DescriptorTable.pDescriptorRanges = &descriptorRange;			// �f�X�N���v�^�����W
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;						// �f�t�H���g�l
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;				// �S�ẴV�F�[�_���猩����

	// �萔�o�b�t�@1�� 
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;		// ���
	rootParams[2].Descriptor.ShaderRegister = 1;						// �萔�o�b�t�@�̔ԍ�
	rootParams[2].Descriptor.RegisterSpace = 0;							// �f�t�H���g�l
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;		// �S�ẴV�F�[�_���猩����

	//// �������s�N�Z����
	//const size_t textureWidth = 256;

	//// �c�����s�N�Z����
	//const size_t textureHeight = 256;

	//// �z��̗v�f��
	//const size_t imageDataCount = textureWidth * textureHeight;

	//// �摜�C���[�W�f�[�^�z��
	//XMFLOAT4* imageData = new XMFLOAT4[imageDataCount]; // ��ŕK�����

	//// �S�s�N�Z���̐F��������
	//for (size_t i = 0; i < imageDataCount; i++)
	//{
	//	imageData[i].x = 1.0f;
	//	imageData[i].y = 0.0f;
	//	imageData[i].z = 0.0f;
	//	imageData[i].w = 1.0f;
	//}

	TexMetadata metadata{};
	ScratchImage scratchImg{};
	// WIC�e�N�X�`���̃��[�h
	result = LoadFromWICFile(
		L"Resources/texture.png",	// [Resources]�t�H���_��[texture.png]
		WIC_FLAGS_NONE,
		&metadata, scratchImg);

	ScratchImage mipChain{};
	// �~�b�v�}�b�v����
	result = GenerateMipMaps(
		scratchImg.GetImages(), scratchImg.GetImageCount(), scratchImg.GetMetadata(),
		TEX_FILTER_DEFAULT, 0, mipChain);
	if (SUCCEEDED(result)) {
		scratchImg = std::move(mipChain);
		metadata = scratchImg.GetMetadata();
	}

	// �ǂݍ��񂾃f�B�t���[�Y�e�N�X�`����SRGB�Ƃ��Ĉ���
	metadata.format = MakeSRGB(metadata.format);

	TexMetadata metadata2{};
	ScratchImage scratchImg2{};
	// WIC�e�N�X�`���̃��[�h
	result = LoadFromWICFile(
		L"Resources/reimu.png",	// [Resources]�t�H���_��[texture.png]
		WIC_FLAGS_NONE,
		&metadata2, scratchImg2);

	ScratchImage mipChain2{};
	// �~�b�v�}�b�v����
	result = GenerateMipMaps(
		scratchImg2.GetImages(), scratchImg2.GetImageCount(), scratchImg2.GetMetadata(),
		TEX_FILTER_DEFAULT, 0, mipChain2);
	if (SUCCEEDED(result)) {
		scratchImg2 = std::move(mipChain2);
		metadata2 = scratchImg2.GetMetadata();
	}

	// �ǂݍ��񂾃f�B�t���[�Y�e�N�X�`����SRGB�Ƃ��Ĉ���
	metadata2.format = MakeSRGB(metadata2.format);

	// �q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES textureHeapProp{};
	textureHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM; // GPU�̓]���p
	textureHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	textureHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	// ���\�[�X�ݒ�
	D3D12_RESOURCE_DESC textureResourceDesc{};
	textureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureResourceDesc.Format = metadata.format;
	textureResourceDesc.Width = metadata.width;
	textureResourceDesc.Height = (UINT)metadata.height;
	textureResourceDesc.DepthOrArraySize = (UINT16)metadata.arraySize;
	textureResourceDesc.MipLevels = (UINT16)metadata.mipLevels;
	textureResourceDesc.SampleDesc.Count = 1;

	D3D12_RESOURCE_DESC textureResourceDesc2{};
	textureResourceDesc2.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureResourceDesc2.Format = metadata2.format;
	textureResourceDesc2.Width = metadata2.width;
	textureResourceDesc2.Height = (UINT)metadata2.height;
	textureResourceDesc2.DepthOrArraySize = (UINT16)metadata2.arraySize;
	textureResourceDesc2.MipLevels = (UINT16)metadata2.mipLevels;
	textureResourceDesc2.SampleDesc.Count = 1;

#pragma region ���_�o�b�t�@

	// ���_�f�[�^�\����
	struct Vertex
	{
		XMFLOAT3 pos;	// xyz���W
		XMFLOAT3 normal;// �@���x�N�g�� 
		XMFLOAT2 uv;	// uv���W
	};

	// ���_�f�[�^
	Vertex vertices[] =
	{
		//  X     Y     Z     �@��   U     V
		// �O
		{{-5.0f,-5.0f,-5.0f }, {}, {0.0f, 1.0f}},	// ����
		{{-5.0f, 5.0f,-5.0f }, {}, {0.0f, 0.0f}},	// ����
		{{ 5.0f,-5.0f,-5.0f }, {}, {1.0f, 1.0f}},	// �E��
		{{ 5.0f, 5.0f,-5.0f }, {}, {1.0f, 0.0f}},	// �E��
							   
 		// ��				   
 		{{-5.0f, 5.0f, 5.0f }, {}, {0.0f, 0.0f}},	// ����
		{{-5.0f,-5.0f, 5.0f }, {}, {0.0f, 1.0f}},	// ����
		{{ 5.0f, 5.0f, 5.0f }, {}, {1.0f, 0.0f}},	// �E��
		{{ 5.0f,-5.0f, 5.0f }, {}, {1.0f, 1.0f}},	// �E��
							   
 		// ��				   
 		{{-5.0f,-5.0f,-5.0f }, {}, {0.0f, 1.0f}},	// ����
		{{-5.0f,-5.0f, 5.0f }, {}, {0.0f, 0.0f}},	// ����
		{{-5.0f, 5.0f,-5.0f }, {}, {1.0f, 1.0f}},	// �E��
		{{-5.0f, 5.0f, 5.0f }, {}, {1.0f, 0.0f}},	// �E��
							   
 		// �E				   
 		{{ 5.0f,-5.0f, 5.0f }, {}, {0.0f, 0.0f}},	// ����
		{{ 5.0f,-5.0f,-5.0f }, {}, {0.0f, 1.0f}},	// ����
		{{ 5.0f, 5.0f, 5.0f }, {}, {1.0f, 0.0f}},	// �E��
		{{ 5.0f, 5.0f,-5.0f }, {}, {1.0f, 1.0f}},	// �E��
							   
 		// ��				   
 		{{-5.0f,-5.0f,-5.0f }, {}, {0.0f, 1.0f}},	// ����
		{{ 5.0f,-5.0f,-5.0f }, {}, {0.0f, 0.0f}},	// ����
		{{-5.0f,-5.0f, 5.0f }, {}, {1.0f, 1.0f}},	// �E��
		{{ 5.0f,-5.0f, 5.0f }, {}, {1.0f, 0.0f}},	// �E��
							   
 		// ��				   
 		{{ 5.0f, 5.0f,-5.0f }, {}, {0.0f, 0.0f}},	// ����
		{{-5.0f, 5.0f,-5.0f }, {}, {0.0f, 1.0f}},	// ����
		{{ 5.0f, 5.0f, 5.0f }, {}, {1.0f, 0.0f}},	// �E��
		{{-5.0f, 5.0f, 5.0f }, {}, {1.0f, 1.0f}},	// �E��
	};

	// �C���f�b�N�X�f�[�^
	unsigned short indices[] =
	{
		// �O
		0,1,2,	// �O�p�`1��
		2,1,3,	// �O�p�`2��

		// ��
		4,5,6,	// �O�p�`3��
		6,5,7,	// �O�p�`4��

		// ��
		8,9,10,	// �O�p�`5��
		10,9,11,	// �O�p�`6��

		// �E
		12,13,14,	// �O�p�`7��
		14,13,15,	// �O�p�`8��

		// ��
		16,17,18,	// �O�p�`9��
		18,17,19,	// �O�p�`10��

		// ��
		20,21,22,	// �O�p�`11��
		22,21,23,	// �O�p�`12��

	};

	// 5-4
	float angle = 0.0f; // �J�����̉�]�p

	// ���_�f�[�^�S�̂̃T�C�Y = ���_�f�[�^����̃T�C�Y * ���_�f�[�^�̗v�f��
	UINT sizeVB = static_cast<UINT>(sizeof(vertices[0]) * _countof(vertices));

	// ���_�o�b�t�@�̐ݒ�
	D3D12_HEAP_PROPERTIES heapProp{}; // �q�[�v�ݒ�
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD; // GPU�ւ̓]���p

	// ���_�f�[�^ ���\�[�X�ݒ�
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeVB; // ���_�f�[�^�S�̂̃T�C�Y
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// �@���̌v�Z
	for (int i = 0; i < _countof(indices) / 3; i++) {
		// �O�p�`1���ƂɌv�Z���Ă���
		// �O�p�`�̃C���f�b�N�X�����o���āA�ꎞ�I�ȕϐ��ɓ����
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];

		// �O�p�`���\�����钸�_���W���x�N�g���ɑ��
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);

		// p0��p1�x�N�g���Ap0��p2�x�N�g�����v�Z�i�x�N�g���̌��Z�j
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);

		// �O�ς͗������琂���ȃx�N�g��
		XMVECTOR normal = XMVector3Cross(v1, v2);

		// ���K���i������1�ɂ���j
		normal = XMVector3Normalize(normal);

		// ���߂��@���𒸓_�f�[�^�ɑ��
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	// ���_�o�b�t�@�̐���
	ID3D12Resource* vertBuff = nullptr;
	result = device->CreateCommittedResource(
		&heapProp, // �q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&resDesc, // ���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	assert(SUCCEEDED(result));

	// GPU��̃o�b�t�@�ɑΉ��������z������(���C����������)���擾
	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	assert(SUCCEEDED(result));

	// �S���_�ɑ΂���
	for (int i = 0; i < _countof(vertices); i++)
	{
		vertMap[i] = vertices[i]; // ���W���R�s�[
	}

	// �q���������
	vertBuff->Unmap(0, nullptr);

	// ���_�o�b�t�@�r���[�̍쐬
	D3D12_VERTEX_BUFFER_VIEW vbView{};

	// GPU���z�A�h���X
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();

	// ���_�o�b�t�@�̃T�C�Y
	vbView.SizeInBytes = sizeVB;

	// ���_1���̃f�[�^�T�C�Y
	vbView.StrideInBytes = sizeof(vertices[0]);

#pragma endregion

#pragma region �C���f�b�N�X�o�b�t�@

	// �C���f�b�N�X�f�[�^�S�̂̃T�C�Y
	UINT sizeIB = static_cast<UINT>(sizeof(uint16_t) * _countof(indices));

	// �C���f�b�N�X�f�[�^ ���\�[�X�ݒ�
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeIB; // �C���f�b�N�X�f�[�^�S�̂̃T�C�Y
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// �C���f�b�N�X�o�b�t�@�̐���
	ID3D12Resource* indexBuff = nullptr;
	result = device->CreateCommittedResource(
		&heapProp, // �q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&resDesc, // ���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuff));
	assert(SUCCEEDED(result));

	// �C���f�b�N�X�o�b�t�@���}�b�s���O
	uint16_t* indexMap = nullptr;
	result = indexBuff->Map(0, nullptr, (void**)&indexMap);

	// �S�C���f�b�N�X�ɑ΂���
	for (int i = 0; i < _countof(indices); i++) {
		indexMap[i] = indices[i]; //�C���f�b�N�X���R�s�[
	}

	// �C���f�b�N�X �}�b�s���O����
	indexBuff->Unmap(0, nullptr);

	// �C���f�b�N�X�o�b�t�@�r���[�̍쐬
	D3D12_INDEX_BUFFER_VIEW ibView{};
	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeIB;

#pragma endregion

#pragma region �e�N�X�`���o�b�t�@

	// �e�N�X�`���o�b�t�@�̐���
	ID3D12Resource* texBuff = nullptr;
	result = device->CreateCommittedResource(
		&textureHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texBuff));
	assert(SUCCEEDED(result));

	// �e�N�X�`���o�b�t�@�̐���
	ID3D12Resource* texBuff2 = nullptr;
	result = device->CreateCommittedResource(
		&textureHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureResourceDesc2,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texBuff2));
	assert(SUCCEEDED(result));

	// �S�~�b�v�}�b�v�ɂ���
	for (size_t i = 0; i < metadata.mipLevels; i++)
	{
		// �~�b�v�}�b�v���x�����w�肵�ăC���[�W���擾
		const Image* img = scratchImg.GetImage(i, 0, 0);

		// �e�N�X�`���o�b�t�@�Ƀf�[�^�]��
		result = texBuff->WriteToSubresource(
			(UINT)i,
			nullptr,		// �S�̈�փR�s�[
			img->pixels,	// ���f�[�^�A�h���X
			(UINT)img->rowPitch,	// 1���C���T�C�Y
			(UINT)img->slicePitch	// �S�T�C�Y
		);
		assert(SUCCEEDED(result));
	}

	// �S�~�b�v�}�b�v�ɂ���
	for (size_t i = 0; i < metadata2.mipLevels; i++)
	{
		// �~�b�v�}�b�v���x�����w�肵�ăC���[�W���擾
		const Image* img2 = scratchImg2.GetImage(i, 0, 0);

		// �e�N�X�`���o�b�t�@�Ƀf�[�^�]��
		result = texBuff2->WriteToSubresource(
			(UINT)i,
			nullptr,		// �S�̈�փR�s�[
			img2->pixels,	// ���f�[�^�A�h���X
			(UINT)img2->rowPitch,	// 1���C���T�C�Y
			(UINT)img2->slicePitch	// �S�T�C�Y
		);
		assert(SUCCEEDED(result));
	}
	
	//// ���f�[�^���
	//delete[] "Resoources/texture.png";

	// SRV�̍ő��
	const size_t kMaxSRVCount = 2056;

	// �f�X�N���v�^�q�[�v�̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// �V�F�[�_�[���猩����悤��
	srvHeapDesc.NumDescriptors = kMaxSRVCount;

	// �ݒ�����Ƃ�SRV�p�f�X�N���v�^�q�[�v�𐶐�
	ID3D12DescriptorHeap* srvHeap = nullptr;
	result = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
	assert(SUCCEEDED(result));

	// SRV�q�[�v�̐擪�n���h�����擾
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();

	// �V�F�[�_�[���\�[�X�r���[�ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};		// �ݒ�\����
	srvDesc.Format = textureResourceDesc.Format;// RGBA float
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	// 2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = textureResourceDesc.MipLevels;

	// �n���h���̎w���ʒu�ɃV�F�[�_�[���\�[�X�r���[�쐬
	device->CreateShaderResourceView(texBuff, &srvDesc, srvHandle);

	// ��n���h����i�߂�
	UINT incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	srvHandle.ptr += incrementSize;

	// �V�F�[�_�[���\�[�X�r���[�ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};		// �ݒ�\����
	srvDesc2.Format = textureResourceDesc2.Format;// RGBA float
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	// 2D�e�N�X�`��
	srvDesc2.Texture2D.MipLevels = textureResourceDesc2.MipLevels;

	// �n���h���̎w���ʒu�ɃV�F�[�_�[���\�[�X�r���[�쐬
	device->CreateShaderResourceView(texBuff2, &srvDesc2, srvHandle);


#pragma endregion

	ID3DBlob* vsBlob = nullptr;		 // ���_�V�F�[�_�I�u�W�F�N�g
	ID3DBlob* psBlob = nullptr;		 // �s�N�Z���V�F�[�_�I�u�W�F�N�g
	ID3DBlob* errorBlob = nullptr;	 // �G���[�I�u�W�F�N�g
	// ���_�V�F�[�_�̓ǂݍ��݂ƃR���p�C��
	result = D3DCompileFromFile(
		L"BasicVS.hlsl", // �V�F�[�_�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�\�ɂ���
		"main", "vs_5_0", // �G���g���[�|�C���g���A�V�F�[�_�[���f���w��
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�ݒ�
		0,
		&vsBlob, &errorBlob);

	// �G���[�Ȃ�
	if (FAILED(result))
	{
		// errorBlob����G���[���e��string�^�ɃR�s�[
		std::string error;
		error.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		// �G���[���e���o�̓E�B���h�E�ɕ\��
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	// �s�N�Z���V�F�[�_�̓ǂݍ��݂ƃR���p�C��
	result = D3DCompileFromFile(
		L"BasicPS.hlsl", // �V�F�[�_�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�\�ɂ���
		"main", "ps_5_0", // �G���g���[�|�C���g���A�V�F�[�_�[���f���w��
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�ݒ�
		0,
		&psBlob, &errorBlob);

	// �G���[�Ȃ�
	if (FAILED(result))
	{
		// errorBlob����G���[���e��string�^�ɃR�s�[
		std::string error;
		error.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		// �G���[���e���o�̓E�B���h�E�ɕ\��
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	// ���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ // xyz���W
			"POSITION",									// �Z�}���e�B�b�N��
			0,											// �����Z�}���e�B�b�N������������Ƃ��Ɏg���C���f�b�N�X (0�ł������[)
			DXGI_FORMAT_R32G32B32_FLOAT,				// �v�f���ƃr�b�g����\�� (XYZ��3��float�^�Ȃ̂�R32G32B32_FLOAT)
			0,											// ���̓X���b�g�C���f�b�N�X (0�ł������[)
			D3D12_APPEND_ALIGNED_ELEMENT,				// �f�[�^�̃I�t�Z�b�g�l (D3D12_APPEND_ALIGNED_ELEMENT���Ǝ����ݒ�
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // ���̓f�[�^��� (�W����D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DAT)
			0											// ��x�ɕ`�悷��C���X�^���X�� (0�ł������[)
		}, // ���W�ȊO�ɐF�A�e�N�X�`��UV�Ȃǂ�n���ꍇ�͍X�ɑ�����B

		{ // �@���x�N�g��
			"NORMAL",									// �Z�}���e�B�b�N��
			0,											// �����Z�}���e�B�b�N������������Ƃ��Ɏg���C���f�b�N�X (0�ł������[)
			DXGI_FORMAT_R32G32B32_FLOAT,				// �v�f���ƃr�b�g����\�� (XYZ��3��float�^�Ȃ̂�R32G32B32_FLOAT)
			0,											// ���̓X���b�g�C���f�b�N�X (0�ł������[)
			D3D12_APPEND_ALIGNED_ELEMENT,				// �f�[�^�̃I�t�Z�b�g�l (D3D12_APPEND_ALIGNED_ELEMENT���Ǝ����ݒ�
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // ���̓f�[�^��� (�W����D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DAT)
			0											// ��x�ɕ`�悷��C���X�^���X�� (0�ł������[)
		}, // ���W�ȊO�ɐF�A�e�N�X�`��UV�Ȃǂ�n���ꍇ�͍X�ɑ�����B

		{ // uv���W
			"TEXCOORD",									// �Z�}���e�B�b�N��
			0,											// �����Z�}���e�B�b�N������������Ƃ��Ɏg���C���f�b�N�X (0�ł������[)
			DXGI_FORMAT_R32G32_FLOAT,				    // �v�f���ƃr�b�g����\�� (uv��2��float�^�Ȃ̂�R32G32_FLOAT)
			0,											// ���̓X���b�g�C���f�b�N�X (0�ł������[)
			D3D12_APPEND_ALIGNED_ELEMENT,				// �f�[�^�̃I�t�Z�b�g�l (D3D12_APPEND_ALIGNED_ELEMENT���Ǝ����ݒ�
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // ���̓f�[�^��� (�W����D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DAT)
			0											// ��x�ɕ`�悷��C���X�^���X�� (0�ł������[)
		}, // ���W�ȊO�ɐF�A�e�N�X�`��UV�Ȃǂ�n���ꍇ�͍X�ɑ�����B
	};

	// �O���t�B�b�N�X�p�C�v���C���ݒ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};

	// �V�F�[�_�[�̐ݒ�
	pipelineDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	pipelineDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	pipelineDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	pipelineDesc.PS.BytecodeLength = psBlob->GetBufferSize();

	// �T���v���}�X�N�̐ݒ�
	pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // �W���ݒ�

	// ���X�^���C�U�̐ݒ�
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK; // �w�ʂ��J�����O
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // �|���S�����h��Ԃ�
	//pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME; // �|���S������h��Ԃ��Ȃ��ꍇ�͂�����(���C���[�t���[��)
	pipelineDesc.RasterizerState.DepthClipEnable = true; // �[�x�N���b�s���O��L����

	// �u�����h�X�e�[�g
	//pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // RBGA�S�Ẵ`�����l����`��
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = pipelineDesc.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // RBGA�S�Ẵ`�����l����`��

	//// ���ʐݒ� (a�l)
	//blenddesc.BlendEnable = true;					// �u�����h��L���ɂ���
	//blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;	// ���Z
	//blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;	// �\�[�X�̒l��100% �g��
	//blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;	// �f�X�g�̒l��  0% �g��

	//// ���Z����
	//blenddesc.BlendOp = D3D12_BLEND_OP_ADD;	// ���Z
	//blenddesc.SrcBlend = D3D12_BLEND_ONE; 	// �\�[�X�̒l��100% �g��
	//blenddesc.DestBlend = D3D12_BLEND_ONE;	// �f�X�g�̒l��100% �g��

	//// ���Z����
	//blenddesc.BlendOp = D3D12_BLEND_OP_SUBTRACT;	// ���Z
	//blenddesc.SrcBlend = D3D12_BLEND_ONE;			// �\�[�X�̒l��100% �g��
	//blenddesc.DestBlend = D3D12_BLEND_ONE;		// �f�X�g�̒l��100% �g��

	//// �F���]
	//blenddesc.BlendOp = D3D12_BLEND_OP_ADD;			// ���Z
	//blenddesc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;	// 1.0f - �f�X�g�J���[�̒l
	//blenddesc.DestBlend = D3D12_BLEND_ZERO;			// �f�X�g�̒l��  0% �g��

	//// ����������
	//blenddesc.BlendOp = D3D12_BLEND_OP_ADD;			// ���Z
	//blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;		// �\�[�X�̃A���t�@�l
	//blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;	// 1.0f - �\�[�X�̃A���t�@�l

	// ���_���C�A�E�g�̐ݒ�
	pipelineDesc.InputLayout.pInputElementDescs = inputLayout;
	pipelineDesc.InputLayout.NumElements = _countof(inputLayout);

	// �}�`�̌`��ݒ�
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// ���̑��̐ݒ�
	pipelineDesc.NumRenderTargets = 1; // �`��Ώۂ�1��
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0~255�w���RGBA
	pipelineDesc.SampleDesc.Count = 1; // 1�s�N�Z���ɂ�1��T���v�����O

	// �e�N�X�`���T���v���[�̐ݒ�
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//   ���J��Ԃ�(�^�C�����O)
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//   �c�J��Ԃ�(�^�C�����O)
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	// ���s�J��Ԃ�(�^�C�����O)
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	// �{�[�_�[�̎��͍�
	samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;		// �S�ă��j�A���
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;	// �~�b�v�}�b�v�ő�l
	samplerDesc.MinLOD = 0.0f;				// �~�b�v�}�b�v�ŏ��l
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// �s�N�Z���V�F�[�_�[����̂ݎg�p�\
	
	// ���[�g�V�O�l�`��
	ID3D12RootSignature* rootSignature;

	// ���[�g�V�O�l�`���̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParams;	// ���[�g�p�����[�^�̐擪�A�h���X
	rootSignatureDesc.NumParameters = _countof(rootParams);		// ���[�g�p�����[�^��
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	// ���[�g�V�O�l�`���̃V���A���C�Y
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	assert(SUCCEEDED(result));
	result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(result));
	rootSigBlob->Release();

	// �p�C�v���C���Ƀ��[�g�V�O�l�`�����Z�b�g
	pipelineDesc.pRootSignature = rootSignature;

	// �f�v�X�X�e���V���X�e�[�g�̐ݒ�
	pipelineDesc.DepthStencilState.DepthEnable = true;							// �[�x�e�X�g���s��
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// �������݋���
	pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// ��������΍��i
	pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;		// �[�x�l�t�H�[�}�b�g

	// �p�C�v�����X�e�[�g�̐���
	ID3D12PipelineState* pipelineState = nullptr;
	result = device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState));
	assert(SUCCEEDED(result));

#pragma endregion

	// �Q�[�����[�v
	while (true)
	{
#pragma region �E�B���h�E���b�Z�[�W����
		// ���b�Z�[�W������H
		if (PeekMessage(&window.msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&window.msg); // �L�[���̓��b�Z�[�W�̏���
			DispatchMessage(&window.msg);  // �v���V�[�W���Ƀ��b�Z�[�W�𑗂�
		}

		// �~�{�^���ŏI�����b�Z�[�W��������Q�[�����[�v�𔲂���
		if (window.msg.message == WM_QUIT)
		{
			break;
		}
#pragma endregion
#pragma region DirectX���t���[������
		// DIrectX���t���[������(�X�V����) ��������

		_key.InputUpdate(); // ��Input�N���X :: �A�b�v�f�[�g

			// �����ꂩ�̃L�[�������Ă�����
		if (_key.IsKeyDown(DIK_UP) || _key.IsKeyDown(DIK_DOWN) ||
			_key.IsKeyDown(DIK_RIGHT) || _key.IsKeyDown(DIK_LEFT))
		{
			// ���W���ړ����鏈�� (Z���W)
			if (_key.IsKeyDown(DIK_UP)) { object3ds[0].position.y += 1.0f; }
			else if (_key.IsKeyDown(DIK_DOWN)) { object3ds[0].position.y -= 1.0f; }
			if (_key.IsKeyDown(DIK_RIGHT)) { object3ds[0].position.x += 1.0f; }
			else if (_key.IsKeyDown(DIK_LEFT)) { object3ds[0].position.x -= 1.0f; }
		}

#pragma region �r���[�s��̌v�Z
	if (_key.IsKeyDown(DIK_D) || _key.IsKeyDown(DIK_A))
	{
		if (_key.IsKeyDown(DIK_D)) { angle += XMConvertToRadians(10.0f); }
		else if (_key.IsKeyDown(DIK_A)) { angle -= XMConvertToRadians(10.0f); }

		// angle���W�A������Y������ɉ�]�B ���a�� -100
		eye.x = -100 * sinf(angle);
		eye.z = -100 * cosf(angle);

		matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	}

	if (_key.IsKeyDown(DIK_W) || _key.IsKeyDown(DIK_S))
	{
		if (_key.IsKeyDown(DIK_W)) { angle += XMConvertToRadians(10.0f); }
		else if (_key.IsKeyDown(DIK_S)) { angle -= XMConvertToRadians(10.0f); }

		// angle���W�A������Y������ɉ�]�B ���a�� -100
		eye.y = -100 * sinf(angle);
		eye.z = -100 * cosf(angle);

		matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	}
#pragma endregion 

		// 3D�I�u�W�F�N�g�X�V����
		for (size_t i = 0; i < _countof(object3ds); i++)
		{
			UpdateObject3d(&object3ds[i], matView, matProjection);
		}

		// �o�b�N�o�b�t�@�̔ԍ����擾(2�Ȃ̂�0�Ԃ�1��)
		UINT bbIndex = swapChain->GetCurrentBackBufferIndex();

		// 1.���\�[�X�o���A�ŏ������݉\�ɕύX
		D3D12_RESOURCE_BARRIER barrierDesc{};
		barrierDesc.Transition.pResource = backBuffers[bbIndex]; // �o�b�N�o�b�t�@���w��
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT; // �\����Ԃ���
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; // �`���Ԃ�
		commandList->ResourceBarrier(1, &barrierDesc);

		// 2.�`���̕ύX
		// �����_�[�^�[�Q�b�g�r���[�̃n���h�����擾
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += bbIndex * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);

		// �[�x�X�e���V���r���[�p�f�X�N���^�q�[�v�̃n���h�����擾
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);	

		// 3.��ʃN���A R G B A
		FLOAT clearColor[] = { 0.1f, 0.25f, 0.5f, 0.0f }; // ���ۂ��F

		/*if (_key.IsKeyDown(DIK_7))
		{
			clearColor[0] = 0.9f;
		}*/

		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

#pragma endregion 
#pragma region �O���t�B�b�N�X�R�}���h

		// 4.�`��R�}���h��������

		// �r���[�|�[�g�ݒ�R�}���h
		D3D12_VIEWPORT viewport{};
		viewport.Width = window.width;
		viewport.Height = window.height;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		// �r���[�|�[�g�ݒ�R�}���h���A�R�}���h���X�g�ɐς�
		commandList->RSSetViewports(1, &viewport);

		// �V�U�[��`
		D3D12_RECT scissorRect{};
		scissorRect.left = 0; // �؂蔲�����W��
		scissorRect.right = scissorRect.left + window.width; // �؂蔲�����W�E
		scissorRect.top = 0; // �؂蔲�����W��
		scissorRect.bottom = scissorRect.top + window.height; // �؂蔲�����W��

		// �V�U�[��`�ݒ�R�}���h���A�R�}���h���X�g�ɐς�
		commandList->RSSetScissorRects(1, &scissorRect);

		// �p�C�v���C���X�e�[�g�ƃ��[�g�V�O�l�`���̐ݒ�R�}���h
		commandList->SetPipelineState(pipelineState);
		commandList->SetGraphicsRootSignature(rootSignature);

		// �v���~�e�B�u�`��̐ݒ�R�}���h
		//commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);	 // �_�̃��X�g
		//commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);		 // ���̃��X�g
		//commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);	 // ���̃X�g���b�v
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	 // �O�p�`�̃��X�g
		//commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); // �O�p�`�̃X�g���b�v

		// ���_�o�b�t�@�r���[�̐ݒ�R�}���h
		commandList->IASetVertexBuffers(0, 1, &vbView);

		// �萔�o�b�t�@�r���[(CBV)�̐ݒ�R�}���h
		commandList->SetGraphicsRootConstantBufferView(0, constBuffMaterial->GetGPUVirtualAddress());

		// SRV�q�[�v�̐ݒ�R�}���h
		commandList->SetDescriptorHeaps(1, &srvHeap);

		// SRV�q�[�v�̐擪�n���h�����擾 (SRV���w���Ă���͂�)
		D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();

		// 2���ڂ��w�������悤�ɂ���SRV�̃n���h�������[�g�p�����[�^1�Ԃɐݒ�
		srvGpuHandle.ptr += incrementSize;
		commandList->SetGraphicsRootDescriptorTable(1, srvGpuHandle);

		//// SRV�q�[�v�̐擪�ɂ���SRV�����[�g�p�����[�^1�Ԃɐݒ�
		//commandList->SetGraphicsRootDescriptorTable(1, srvGpuHandle);

		// �C���f�b�N�X�o�b�t�@�r���[�̐ݒ�R�}���h
		commandList->IASetIndexBuffer(&ibView);

		// �S�I�u�W�F�N�g�ɂ��ď���

		// 3D�I�u�W�F�N�g�`�揈��
		for (int i = 0; i < _countof(object3ds); i++)
		{
			DrawObject3D(&object3ds[i], commandList, vbView, ibView, _countof(indices));
		}

		// 4.�`��R�}���h�����܂�

		// 5.���\�[�X�o���A��߂�
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET; // �`���Ԃ���
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT; // �\����Ԃ�
		commandList->ResourceBarrier(1, &barrierDesc);

		// ���߂̃N���[�Y
		result = commandList->Close();
		assert(SUCCEEDED(result));

		// �R�}���h���X�g�̎��s
		ID3D12CommandList* commandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(1, commandLists);
#pragma endregion
#pragma region ��ʓ���ւ�
		// ��ʂɕ\������o�b�t�@���t���b�v(���\�̓��ւ�)
		result = swapChain->Present(1, 0);
		assert(SUCCEEDED(result));

		// �R�}���h�̎��s������҂�
		commandQueue->Signal(fence, ++fenceVal);
		if (fence->GetCompletedValue() != fenceVal) {
			HANDLE event = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		// �L���[���N���A
		result = commandAllocator->Reset();
		assert(SUCCEEDED(result));

		// �ĂуR�}���h���X�g�𒙂߂鏀��
		result = commandList->Reset(commandAllocator, nullptr);
		assert(SUCCEEDED(result));

		// DIrectX���t���[������ �����܂�
#pragma endregion
	}

	return 0;
}

void InitializeObject3d(Object3d* object, ID3D12Device* device)
{
	HRESULT result;

	// �萔�o�b�t�@�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES heapProp{};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	// �萔�o�b�t�@�̃��\�[�X�ݒ�
	D3D12_RESOURCE_DESC resdesc{};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = (sizeof(ConstBufferDataTransform) + 0xff) & ~0xff;
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.SampleDesc.Count = 1;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// �萔�o�b�t�@�̐���
	result = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object->constBuffTransform));
	assert(SUCCEEDED(result));

	// �萔�o�b�t�@�̃}�b�s���O
	result = object->constBuffTransform->Map(0, nullptr, (void**)&object->constMapTransform);
	assert(SUCCEEDED(result));
}

// 3D�I�u�W�F�N�g�X�V����
void UpdateObject3d(Object3d* object, XMMATRIX& matView, XMMATRIX& matProjection)
{
	XMMATRIX matScale, matRot, matTrans;

	// �X�P�[���A��]�A���s�ړ��s��̌v�Z
	matScale = XMMatrixScaling(object->scale.x, object->scale.y, object->scale.z);
	matRot = XMMatrixIdentity();
	matRot *= XMMatrixRotationZ(object->rotation.z);
	matRot *= XMMatrixRotationX(object->rotation.x);
	matRot *= XMMatrixRotationY(object->rotation.y);
	matTrans = XMMatrixTranslation(
		object->position.x, object->position.y, object->position.z );

	// ���[���h�s��̍���
	object->matWorld = XMMatrixIdentity();	// �ό`�����Z�b�g
	object->matWorld *= matScale;			// ���[���h�s��ɃX�P�[�����O�𔽉f
	object->matWorld *= matRot;				// ���[���h�s��ɉ�]�𔽉f
	object->matWorld *= matTrans;			// ���[���h�s��ɕ��s�ړ��𔽉f

	// �e�I�u�W�F�N�g�������
	if (object->parent != nullptr) {
		// �e�I�u�W�F�N�g�̃��[���h�s����|����
		object->matWorld *= object->parent->matWorld;
	}

	// �萔�o�b�t�@�փf�[�^�]��
	object->constMapTransform->mat = object->matWorld * matView * matProjection;
}

void DrawObject3D(Object3d* object, ID3D12GraphicsCommandList* commandList, D3D12_VERTEX_BUFFER_VIEW& vbView, D3D12_INDEX_BUFFER_VIEW& ibView, UINT numIndices)
{
	// ���_�o�b�t�@�̐ݒ�
	commandList->IASetVertexBuffers(0, 1, &vbView);

	// �C���f�b�N�X�o�b�t�@�̐ݒ�
	commandList->IASetIndexBuffer(&ibView);

	// �萔�o�b�t�@�r���[(CBV)�̐ݒ�R�}���h
	commandList->SetGraphicsRootConstantBufferView(2, object->constBuffTransform->GetGPUVirtualAddress());

	// �`��R�}���h
	commandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0); // �S�Ă̒��_���g���ĕ`��
}