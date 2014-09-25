#include <windows.h>
#include "resource.h"

HBITMAP g_hbmBall = NULL;
const char g_szClassName[] = "myWindowClass";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
		case WM_CREATE:
        {
			g_hbmBall = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP1));
			if (g_hbmBall == NULL)
				MessageBox(hwnd, "Could not load Image!", "Error", MB_OK | MB_ICONEXCLAMATION);		
		
            HMENU hMenu, hSubMenu, hIcon, hIconSm;
            hMenu = CreateMenu();
            hSubMenu = CreatePopupMenu();
			AppendMenu(hSubMenu, MF_STRING, ID_FILE_OPEN, "&Open File");
			AppendMenu(hSubMenu, MF_STRING, ID_FILE_EXIT, "E&xit");
            AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&File");

            hSubMenu = CreatePopupMenu();
            AppendMenu(hSubMenu, MF_STRING, ID_STUFF_GO, "&About");
            AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&Help");

            SetMenu(hwnd, hMenu);


            hIcon = LoadImage(NULL, "image.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
            if(hIcon)
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            else
                MessageBox(hwnd, "Could not load large icon!", "Error", MB_OK | MB_ICONERROR);

            hIconSm = LoadImage(NULL, "image.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
            if(hIconSm)
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
            else
                MessageBox(hwnd, "Could not load small icon!", "Error", MB_OK | MB_ICONERROR);
        }
        break;
        case WM_COMMAND:
            switch(LOWORD(wParam))
			{
				case ID_FILE_EXIT:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;
				case ID_FILE_OPEN:
				{
					OPENFILENAME ofn;
					char szFileName[MAX_PATH] = "";

					ZeroMemory(&ofn, sizeof(ofn));

					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = hwnd;
					ofn.lpstrFilter = "Files (*.ppn)\0*.ppm\0All Files (*.*)\0*.*\0";
					ofn.lpstrFile = szFileName;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					ofn.lpstrDefExt = "ppm";

					if (GetOpenFileName(&ofn))
					{
						HWND hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
						LoadTextFileToEdit(hEdit, szFileName);
						MessageBox(hwnd, szFileName, "Error", MB_OK | MB_ICONEXCLAMATION);
					}
				}
				break;
				case ID_STUFF_GO:
					MessageBox(hwnd, "About!", "Error", MB_OK | MB_ICONERROR);
				break;
			}
        break;
		case WM_PAINT:
		{
			 BITMAP bm;
			 PAINTSTRUCT ps;

			 HDC hdc = BeginPaint(hwnd, &ps);

			 HDC hdcMem = CreateCompatibleDC(hdc);
			 HBITMAP hbmOld = SelectObject(hdcMem, g_hbmBall);

			 GetObject(g_hbmBall, sizeof(bm), &bm);

			 BitBlt(hdc, 0, 0, bm.bmWidth * 2, bm.bmHeight * 2, hdcMem, -50, -50, SRCCOPY);

			 SelectObject(hdcMem, hbmOld);
			 DeleteDC(hdcMem);

			 EndPaint(hwnd, &ps);
		}
		break;
		case WM_LBUTTONDOWN:  
		{
            char szFileName[MAX_PATH];
            HINSTANCE hInstance = GetModuleHandle(NULL);

            GetModuleFileName(hInstance, szFileName, MAX_PATH);
            MessageBox(hwnd, szFileName, "This program is:", MB_OK | MB_ICONINFORMATION);
        }
		break;              
        case WM_CLOSE:
			DeleteObject(g_hbmBall);
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
	

    if(!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "PPM Image Editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}

BOOL LoadTextFileToEdit(HWND hEdit, LPCTSTR pszFileName)
{
	HANDLE hFile;
	BOOL bSuccess = FALSE;

	hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwFileSize;

		dwFileSize = GetFileSize(hFile, NULL);
		if (dwFileSize != 0xFFFFFFFF)
		{
			LPSTR pszFileText;

			pszFileText = (LPSTR)GlobalAlloc(GPTR, dwFileSize + 1);
			if (pszFileText != NULL)
			{
				DWORD dwRead;
				ReadFile(hFile, pszFileText, dwFileSize, &dwRead, NULL);
				GlobalFree(pszFileText);
			}
		}
		CloseHandle(hFile);
	}
	return bSuccess;
}