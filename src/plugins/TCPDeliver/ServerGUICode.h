
BOOL CALLBACK DialogProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
  )
{
	char buf[MAX_PATH];
	switch (uMsg) 
	{
		case WM_COMMAND:
			switch(wParam) {
				case IDCANCEL:
					ShowWindow(hwndDlg,SW_MINIMIZE);
					break;
/*				case IDC_CHECK_EnableRGB:
					break;
				case IDC_CHECK_R:
					break;
				case IDC_CHECK_G:
					break;
				case IDC_CHECK_B:
					break;*/
			}
			break;

		case WM_INITDIALOG:
			wsprintf(buf,"%d",0);
			return true;
	}
	return false;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_COMMAND:
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
    }
	return 0;
}

void startWindow() {
	MSG msg;
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= "TCPServer GUI";
	wcex.hIconSm		= LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	RegisterClassEx(&wcex);
	hDlg=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_SERVERGUI),NULL,DialogProc);

    while (GetMessage(&msg, NULL, 0, 0))
	if (NULL == hDlg || !IsDialogMessage(hDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
	}
}


