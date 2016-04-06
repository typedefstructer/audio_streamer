#define _WINSOCKAPI_
#include<windows.h>
#include<winsock2.h>
#include<ws2tcpip.h>

#include<dsound.h>
#include<stdio.h>
#include<stdint.h>
#include<math.h>

#define global static
#define persist static
#define internal static

struct win32_window_dimension
{
	int Width;
	int Height;
};

struct win32_bitmap_buffer
{
	BITMAPINFO Info;
	int Width;
	int Height;
	void *Memory;
};

global bool AppRunning;
global win32_bitmap_buffer GlobalBitmapBuffer;
global LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

internal void
Win32BlitBitmap(HDC DeviceContext, win32_bitmap_buffer *Buffer, int WindowWidth, int WindowHeight)
{
	int OffSetX = (WindowWidth - Buffer->Width)/2;
	int OffSetY = (WindowHeight- Buffer->Height)/2;

	PatBlt(DeviceContext, 0, 0, OffSetX, WindowHeight, WHITENESS);
	PatBlt(DeviceContext, OffSetX + Buffer->Width, 0, WindowWidth, WindowHeight, WHITENESS);

	PatBlt(DeviceContext, 0, 0, WindowWidth, OffSetY, WHITENESS);
	PatBlt(DeviceContext, 0, OffSetY + Buffer->Height, WindowWidth, WindowHeight, WHITENESS);

	StretchDIBits(DeviceContext,
				  OffSetX, OffSetY, Buffer->Width, Buffer->Height,
				  0, 0, Buffer->Width, Buffer->Height,
				  Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

internal void
Win32AllocBitmapBuffer(win32_bitmap_buffer *Buffer, int Width, int Height)
{
	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Info = {};
	Buffer->Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	Buffer->Info.bmiHeader.biWidth = Width;
	Buffer->Info.bmiHeader.biHeight = -Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	Buffer->Width  = Width;
	Buffer->Height = Height;
	int BitmapBufferSize = Width * Height * 4;
	Buffer->Memory = VirtualAlloc(0, BitmapBufferSize, MEM_COMMIT, PAGE_READWRITE);
}

internal win32_window_dimension
Win32WindowDimension(HWND Window)
{
	win32_window_dimension Result = {};
	RECT Rect;
	GetClientRect(Window, &Rect);

	Result.Width  = Rect.right - Rect.left;
	Result.Height = Rect.bottom - Rect.top;

	return Result;
}

void PlayBack(char *Capture, int RecIndex, int BufferSize)
{   
	void *Region1, *Region2;
	DWORD Region1Bytes, Region2Bytes;
	DWORD PlayCursor;
	DWORD WriteCursor;
	DWORD LockBytes, RegionSize;
		
	GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);	
	if(WriteCursor >= PlayCursor)
	{
		LockBytes  = WriteCursor;
		int Test = BufferSize - WriteCursor + PlayCursor;		
		RegionSize = Test;
	}
	else
	{
		LockBytes  = WriteCursor;
		int Test = PlayCursor - WriteCursor;
		RegionSize = Test; 
	}
		
	GlobalSecondaryBuffer->Lock(LockBytes, RegionSize,
								&Region1, &Region1Bytes, &Region2, &Region2Bytes,
								0);
		
	int16_t *Sample = (int16_t*)Region1;
	int16_t *CapStore = (int16_t *)Capture;
	int NoiseGate = 8000;
	int PlayBackIndex = 0;
	for(int i=0;i<Region1Bytes/4;i++)
	{
		// float val = sin((float)i/valu)*4000;
		if(PlayBackIndex < RecIndex)
		{		   				
			*Sample++ = *CapStore++;
			*Sample++ = *CapStore++;				
		}
		else
		{			
			*Sample++ = 0;
			*Sample++ = 0;
		}
		PlayBackIndex++;
	}
	Sample = (int16_t*)Region2;
	for(int i=0;i<Region2Bytes/4;i++)
	{
		// float val = sin((float)i/valu)*4000;
		if(PlayBackIndex < RecIndex)
		{
			*Sample++ = *CapStore++;
			*Sample++ = *CapStore++;
		}
		else
		{
			*Sample++ = 0;
			*Sample++ = 0;
		}						
	}
				
	GlobalSecondaryBuffer->Unlock(Region1, Region1Bytes, Region2, Region2Bytes);		
}

void DrawRect(win32_bitmap_buffer *Buffer, int MinX, int MinY, int MaxX, int MaxY,
			  uint32_t Color)
{
	MinY = MinY+Buffer->Height/2;
	MaxY = MaxY+Buffer->Height/2;
	if(MinX < 0)
	{
		MinX = 0;
	}

	if(MinY < 0)
	{
		MinY = 0;
	}

	if(MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}

	if(MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	uint32_t *Row = (uint32_t *)Buffer->Memory + MinY * Buffer->Width + MinX;
	for(int Y=MinY; Y<MaxY; Y++)
	{
		uint32_t *Pixel = Row;
		for(int X=MinX; X<MaxX; X++)
		{
			*Pixel++ = Color;
		}
		Row = Row + Buffer->Width;
	}
}

void UpdateBitMap(win32_bitmap_buffer *Buffer, int16_t *Wave, int32_t WaveSize)
{
	uint32_t *Row = (uint32_t*)Buffer->Memory;
	for(int Y = 0; Y<Buffer->Height; Y++)
	{
		uint32_t *Pixel = Row;
		for(int X = 0; X<Buffer->Width; X++)
		{
			*Pixel++ = 0x00000FFFF;
		}
		Row += Buffer->Width;
	}
	
	for(int i=0;i<WaveSize;i++)
	{
		float fBx = (float)i*((float)(Buffer->Width*10)/(float)WaveSize);
		float fBy = (float)Wave[i]*((float)(Buffer->Height/2)/(float)pow(2,16));
		DrawRect(Buffer, fBx, fBy, fBx+2, fBy+2, 0x00FF0000);
	}	
}

void InitSocket()
{
	WSADATA wd;
	WSAStartup(MAKEWORD(2, 2), &wd);
}

void InitDSound(HWND Window, int BufferSize)
{
	LPDIRECTSOUND DirectSound;	
	DSBUFFERDESC PrimaryBufferDesc = {}, SecondaryBufferDesc = {};
	LPDIRECTSOUNDBUFFER PrimaryBuffer = {}, SecondaryBuffer  = {};
	WAVEFORMATEX WaveFormat = {};   
	
	WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat.nChannels  = 2;
    WaveFormat.nSamplesPerSec = 48000;
    WaveFormat.wBitsPerSample = 16;
    WaveFormat.nBlockAlign = WaveFormat.wBitsPerSample * WaveFormat.nChannels / 8;    
	WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
	
	DirectSoundCreate(0, &DirectSound, 0);
	DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY);

	PrimaryBufferDesc.dwSize  = sizeof(DSBUFFERDESC);
	PrimaryBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;   	
	DirectSound->CreateSoundBuffer(&PrimaryBufferDesc, &PrimaryBuffer, 0);   	
	
	PrimaryBuffer->SetFormat(&WaveFormat);

	SecondaryBufferDesc.dwSize  = sizeof(DSBUFFERDESC);
	SecondaryBufferDesc.dwFlags = DSBCAPS_GLOBALFOCUS;
	SecondaryBufferDesc.dwBufferBytes = BufferSize;
	SecondaryBufferDesc.lpwfxFormat = &WaveFormat;

	DirectSound->CreateSoundBuffer(&SecondaryBufferDesc, &SecondaryBuffer, 0);
	GlobalSecondaryBuffer = SecondaryBuffer;
}

LRESULT CALLBACK
windowproc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT result = 0;
	switch(Message)
	{
		case WM_CLOSE:
		{
			AppRunning = false; 
			PostQuitMessage(0);
		} break;

		default:
		{
			result = DefWindowProc(Window, Message, WParam, LParam);			
		}
	}
	return result;
}

int WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
	
	
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = windowproc;
	wc.hInstance = Instance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = "nlife_shadow_client";

	RegisterClassEx(&wc);
	HWND Window = CreateWindowEx(0, wc.lpszClassName, "nlife_shadow_client",
							   WS_OVERLAPPEDWINDOW | WS_VISIBLE,
							   CW_USEDEFAULT, CW_USEDEFAULT,
							   400, 200, 0, 0, Instance, 0);

	const DWORD BufferSize = (48000*4)/16;

	InitSocket();	
	InitDSound(Window, BufferSize); // 1/16 Seconds Worth Of Sound	

	MSG Message;	
	GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

	addrinfo *server = 0, server_hints = {};
	server_hints.ai_family = AF_INET;
	server_hints.ai_socktype = SOCK_STREAM;
	server_hints.ai_protocol = IPPROTO_TCP;
	server_hints.ai_flags = AI_PASSIVE;

	getaddrinfo("10.1.75.6", "1295", &server_hints, &server);

	int connect_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	connect(connect_socket, server->ai_addr, server->ai_addrlen);
	
	AppRunning = true;	
	char Capture[BufferSize];

	const int lag = 256;
	
	int16_t *Out = (int16_t*)Capture;		
	for(int i=0;i<BufferSize/4;i++)
	{
		float val = sin((float)i/2.0f)*5000;
		*Out++ = val;
		*Out++ = val;
	}

	float Valu = 1.0f;
	Win32AllocBitmapBuffer(&GlobalBitmapBuffer, 400, 200);	  
	while(AppRunning)
	{
		while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}		
	   		
		int16_t *Out = (int16_t*)Capture;		
		for(int i=0;i<BufferSize/4;i++)
		{
			float val = sin((float)i/Valu)*5000;
			*Out++ = val;
			*Out++ = val;
		}
		
		Valu += 0.1f;
		if(Valu > 100) Valu = 1;
		int RecvSize = recv(connect_socket, Capture, BufferSize, 0);
		
#if 1
		UpdateBitMap(&GlobalBitmapBuffer, (int16_t*)Capture, BufferSize/(4));		
		HDC DeviceContext = GetDC(Window);
		{
			win32_window_dimension Dimension = Win32WindowDimension(Window);
			Win32BlitBitmap(DeviceContext, &GlobalBitmapBuffer, Dimension.Width, Dimension.Height);
		}		
		ReleaseDC(Window, DeviceContext);		
#endif
		
		PlayBack(Capture, BufferSize/(4), BufferSize);		
	}

	return 0;
}
