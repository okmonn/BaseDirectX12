#pragma once
#define INITGUID
#include "Window.h"
#include <d3d12.h>
#include <dinput.h>
#include <memory>

#define VERSION 0x0800

class Input
{
public:
	// コンストラクタ
	Input(std::weak_ptr<Window>winAdr);
	// デストラクタ
	~Input();

	// インプットの生成
	HRESULT CreateInput(void);
	// インプットデバイスの生成
	HRESULT CreateInputDevice(void);

	// インプットデバイスをキーボードにセット
	HRESULT SetInputDevice(void);

	// キー入力
	BOOL InputKey(UINT data);

private:
	// ウィンドウクラス参照
	std::weak_ptr<Window>win;

	// 参照結果
	HRESULT result;

	// インプット
	LPDIRECTINPUT8 input;

	// インプットデバイス
	LPDIRECTINPUTDEVICE8 dev;

	// キー情報
	BYTE key[256];
};