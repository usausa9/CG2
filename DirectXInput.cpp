#include "DirectXInput.h"
#include <cassert>

#define DIRECTINPUT_VERSION 0x0800 // DirectInput�̃o�[�W�����w��
#include <dinput.h>

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

// DirectInput�̏�����
static IDirectInput8* directInput = nullptr;
static IDirectInputDevice8* keyboard = nullptr;

// �S�L�[�̓��͏�Ԃ��擾����
static BYTE keys[256] = {};

// �S�L�[��1F�O�̓��͏�Ԃ��擾����
static BYTE prev[256] = {};

void DirectXInput::InputInit(HRESULT result, WNDCLASSEX w, HWND hwnd )
{
	// DirectInput�̏�����
	result = DirectInput8Create(
		w.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	// �L�[�{�[�h�f�o�C�X�̐���
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	// ���̓f�[�^�`���̃Z�b�g
	result = keyboard->SetDataFormat(&c_dfDIKeyboard); // �W���`��
	assert(SUCCEEDED(result));

	// �r�����䃌�x���̃Z�b�g
	result = keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));
}

void DirectXInput::InputUpdate()
{
	// �O��X�V
	for (int i = 0; i < 256; ++i)
	{
		prev[i] = keys[i];
	}
	
	// �L�[�{�[�h���̎擾�J�n
	keyboard->Acquire();
	keyboard->GetDeviceState(sizeof(keys), keys);
}

//�������ςȂ�
bool DirectXInput::IsKeyDown(UINT8 key)
{
	return keys[key];
}

//�������u��
bool DirectXInput::IsKeyTrigger(UINT8 key)
{
	return keys[key] && !prev[key];
}

//�������u��
bool DirectXInput::GetKeyReleased(UINT8 key)
{
	return !keys[key] && !prev[key];
}