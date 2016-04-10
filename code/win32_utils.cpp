win32_window_dimension
Win32WindowDimension(HWND Window)
{
	win32_window_dimension Result = {};
	RECT Rect;
	GetClientRect(Window, &Rect);

	Result.Width = Rect.right - Rect.left;
	Result.Height = Rect.bottom - Rect.top;

	return Result;
}

HWND GetWindow(const char *ClassName, const char *Title, u32 Width, u32 Height, window_procedure *WindowProcedure)
{
	WNDCLASSEX WindowClass = {};
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.lpfnWndProc = WindowProcedure;
	WindowClass.hInstance = GetModuleHandle(0);
	WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
	WindowClass.lpszClassName = ClassName;

	RegisterClassEx(&WindowClass);
	
	HWND Window = CreateWindowEx(0, WindowClass.lpszClassName, Title,
								 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
								 CW_USEDEFAULT, CW_USEDEFAULT, Width, Height,
								 0, 0, GetModuleHandle(0), 0);

	return Window;
}
