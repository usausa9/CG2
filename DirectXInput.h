#pragma once
#include <dinput.h>

class DirectXInput
{
public:
// �������ƃA�b�v�f�[�g�̊֐� �p��
	static void InputInit(HRESULT result, WNDCLASSEX w ,HWND hwnd);
	static void InputUpdate();

// �L�[�{�[�h���͏����p (�Ԃ�l0,1)
	static bool IsKeyDown(char key);		//�������ςȂ�
	static bool IsKeyTrigger(char key);		//�������u��
	static bool GetKeyReleased(char key);	//�������u��
};

