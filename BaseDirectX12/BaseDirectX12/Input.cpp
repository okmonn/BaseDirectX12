#include "Input.h"
#include "Typedef.h"
#include <tchar.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dinput8.lib")

// コンストラクタ
Input::Input(std::weak_ptr<Window>winAdr)
{
	//ウィンドウクラス参照
	win = winAdr;

	//参照結果の初期化
	result = S_OK;

	//インプットの初期化
	input = nullptr;

	//インプットデバイスの初期化
	dev = nullptr;

	//キー情報配列の初期化
	for (UINT i = 0; i < 256; i++)
	{
		key[i] = 0;
	}


	//エラーを出力に表示させる
#ifdef _DEBUG
	ID3D12Debug *debug = nullptr;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (FAILED(result))
		int i = 0;
	debug->EnableDebugLayer();
	debug->Release();
	debug = nullptr;
#endif

	result = SetInputDevice();
}

// デストラクタ
Input::~Input()
{
	//インプットデバイス
	if (dev != nullptr)
	{
		//キーボードへのアクセス権利の開放
		dev->Unacquire();

		dev->Release();
	}

	//インプット
	if (input != nullptr)
	{
		input->Release();
	}
}

// インプットの生成
HRESULT Input::CreateInput(void)
{
	//インプット生成
	result = DirectInput8Create(GetModuleHandle(0), VERSION, IID_IDirectInput8, (void**)(&input), NULL);
	if (FAILED(result))
	{
		OutputDebugString(_T("インプットの生成：失敗\n"));

		return result;
	}

	return result;
}

// インプットデバイスの生成
HRESULT Input::CreateInputDevice(void)
{
	result = CreateInput();
	if (FAILED(result))
	{
		return result;
	}

	//インプットデバイス生成
	result = input->CreateDevice(GUID_SysKeyboard, &dev, NULL);
	if (FAILED(result))
	{
		OutputDebugString(_T("インプットデバイスの生成：失敗\n"));

		return result;
	}

	return result;
}

// インプットデバイスをキーボードにセット
HRESULT Input::SetInputDevice(void)
{
	result = CreateInputDevice();
	if (FAILED(result))
	{
		return result;
	}

	//キーボードにセット
	result = dev->SetDataFormat(&keybord);
	if (FAILED(result))
	{
		OutputDebugString(_T("インプットデバイスのキーボードセット：失敗\n"));
		return result;
	}

	//協調レベルをセット
	result = dev->SetCooperativeLevel(win.lock()->GetWindowHandle(), DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	if (FAILED(result))
	{
		OutputDebugString(_T("協調レベルのセット：失敗\n"));

		return result;
	}

	//入力デバイスへのアクセス権利を取得
	dev->Acquire();

	return result;
}

// キー入力
BOOL Input::InputKey(UINT data)
{
	//ダミー宣言
	BOOL flag = FALSE;

	//キー情報を取得
	dev->GetDeviceState(sizeof(key), &key);

	if (key[data] & 0x80)
	{
		flag = TRUE;
	}

	return flag;
}
