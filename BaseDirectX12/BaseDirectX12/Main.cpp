#define EXPORT_MAIN
#include "Main.h"
#include <d3d12.h>


// インスタンス処理
void Create(void)
{
	//ウィンドウクラスのインスタン
	win = std::make_shared<Window>();

	//インプットクラスのインスタンス
	input = std::make_shared<Input>(win);

	//デバイスクラスのインスタンス
	dev = std::make_shared<Device>(win, input);
}

// メモリ解放処理
void Destroy(void)
{
}

// ========== WinMain関数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Create();

	//表示させたいウィンドウハンドル,表示の指定(SW_からなる定義使用)
	ShowWindow(win->GetWindowHandle(), nCmdShow);

	//メッセージ用構造体
	MSG msg;
	SecureZeroMemory(&msg, sizeof(msg));

	while (msg.message != WM_QUIT)
	{
		//呼び出し側スレッドが所有しているウィンドウに送信されたメッセージの保留されている物を取得
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//仮想キーメッセージを文字メッセージに変換
			TranslateMessage(&msg);
			//1つのウィドウプロシージャにメッセージを送出する
			DispatchMessage(&msg);
		}
		else
		{
			//エスケープキーでループ終了
			if (input->InputKey(DIK_ESCAPE) == TRUE)
			{
				break;
			}

			//アップデート処理
			dev->UpData();
		}
	}

	Destroy();

	return msg.wParam;
}