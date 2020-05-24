#include <gamemanager.h>

extern BOOL ActiveApp;

/****************************************************************************
GameManager's constructor: creates the three objects, Easydraw, sound and input
*****************************************************************************/

GameManager::GameManager(int w, int h, char *WindowName, int IconID)
{	
	Clear();
	DDraw = new EASYDRAW(w, h, WndProc, WindowName, IconID);
	
	if( DDraw ) // did it get created?
	{
		Component::DDraw = DDraw;		
		DSound = new EASYSOUND(DDraw->GetWindowHandle());
	}

	if( DSound ) // did dsound get created
		DInput = new EASYINPUT(DDraw->GetWindowHandle());

	if( DInput )
		WindowManager::DInput = DInput;

	windowmgr = new WindowManager;
//	Entity::SetEasyDraw(DDraw);
}

GameManager::~GameManager()
{
	delete DInput;
	delete DSound;
	delete DDraw;
	delete windowmgr;

	Clear();
}

void GameManager::Clear()
{
	DDraw = NULL, DSound = NULL, DInput = NULL, windowmgr = NULL;
}

void GameManager::MainGameLoop()
{
	MSG msg;

	start(); // pure virtual function

	while( 1 ) // windows messageloop
	{
		if( PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) )
		{
			if( msg.message != WM_QUIT )
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
				break;
		}
		else if( ActiveApp ) // activeapp is global (set at WM_ACTIVATEAPP)
		{			
			frame(); // pure virtual
		}
		else
			WaitMessage();
	}		

	stop(); // pure virtual function
}

void GameManager::PauseGame(BOOL pause, int ResumeKey)
{
	//ActiveApp = !pause; // pausegame(false) for active game

	DDraw->DrawString(DDraw->GetScreenWidth() / 2, DDraw->GetScreenHeight() / 2, "Game Paused");
	DDraw->Swap();
	Sleep(200);
	while( !DInput->KeyDown(ResumeKey) ) ;
	Sleep(200);
	//ActiveApp = !ActiveApp;
}

void GameManager::StartTimer(int timer_id, int elapse, TIMERPROC func)
{
	SetTimer(DDraw->GetWindowHandle(), timer_id, elapse, func);
}

void GameManager::KillTimer(int timer_id)
{
	::KillTimer(DDraw->GetWindowHandle(), timer_id);
}

void GameManager::Quit()
{
	SendMessage(DDraw->GetWindowHandle(), WM_CLOSE, 0, 0);
}

LRESULT CALLBACK GameManager::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{	
	switch( iMsg )
	{
		case WM_ACTIVATEAPP :
			ActiveApp = (wParam ? TRUE : FALSE);
			return 0;

		case WM_SYSCOMMAND :
			if( wParam == SC_CLOSE )
				PostQuitMessage(0);
			break;

		case WM_DESTROY :
			PostQuitMessage(0);
			return 0;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}