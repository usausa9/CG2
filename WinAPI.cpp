#include "WinAPI.h"

LRESULT WinAPI::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ���b�Z�[�W�ɉ����ăQ�[���ŗL�̏������s��
	switch (msg)
	{
		// �E�B���h�E���j�����ꂽ
	case WM_DESTROY:
		// OS�ɑ΂��āA�A�v���̏I����`����
		PostQuitMessage(0);
		return 0;
	}

	// �W���̃��b�Z�[�W�������s��
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WinAPI::Set()
{
	// �E�B���h�E�N���X�̐ݒ�
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProc;	// �E�B���h�E�v���V�[�W����ݒ�
	w.lpszClassName = L"DirectXGame";		// �E�B���h�E�N���X��
	w.hInstance = GetModuleHandle(nullptr); // �E�B���h�E�n���h��
	w.hCursor = LoadCursor(NULL, IDC_ARROW);// �J�[�\���w��

	// �E�B���h�E�N���X��OS�ɓo�^����
	RegisterClassExW(&w);
}

void WinAPI::DebugText(LPCSTR text)
{
	// �R���\�[���ւ̕����o��
	OutputDebugStringA(text);
}

void WinAPI::CreateWindowObj()
{
	// �E�B���h�E�T�C�Y{ X���W Y���W ���� �c�� }
	wrc = { 0, 0, width, height };

	// �����ŃT�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g�̐���
	hwnd = CreateWindow(
		w.lpszClassName,		//�N���X��
		L"MyEngine",			// �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,	// �W���I�ȃE�B���h�E�X�^�C��
		CW_USEDEFAULT,			// �\��X���W (OS�ɔC����)
		CW_USEDEFAULT,			// �\��Y���W (OS�ɔC����)
		wrc.right - wrc.left,	// �E�B���h�E����
		wrc.bottom - wrc.top,	// �E�B���h�E�c��
		nullptr,				// �e�E�B���h�E�n���h��
		nullptr,				// ���j���[�n���h��
		w.hInstance,			// �Ăяo���A�v���P�[�V�����n���h��
		nullptr);				// �I�v�V����
}

void WinAPI::Show()
{
	// �E�B���h�E��\����Ԃɂ���
	ShowWindow(hwnd, SW_SHOW);
}