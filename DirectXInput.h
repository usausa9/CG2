#pragma once
#include <dinput.h>

class DirectXInput
{
public:
// 初期化とアップデートの関数 用意
	static void InputInit(HRESULT result, WNDCLASSEX w ,HWND hwnd);
	static void InputUpdate();

// キーボード入力処理用 (返り値0,1)
	static bool IsKeyDown(char key);		//押しっぱなし
	static bool IsKeyTrigger(char key);		//押した瞬間
	static bool GetKeyReleased(char key);	//離した瞬間
};

