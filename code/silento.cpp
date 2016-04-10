#define _WINSOCKAPI_
#include<windows.h>
#include<winsock2.h>
#include<ws2tcpip.h>

#include<dsound.h>

#include<stdint.h>

#include<math.h>
#include<stdio.h>

#define global static

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  r32;
typedef double r64;

typedef u32 b32;
typedef LRESULT CALLBACK window_procedure(HWND , UINT, WPARAM, LPARAM);

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

struct Speakers
{
	LPDIRECTSOUNDBUFFER SoundBuffer;
    int SamplesPerSec;
	int BufferSize;
	s8 *Raw;
};

struct Microphone
{
	LPDIRECTSOUNDCAPTUREBUFFER SoundBuffer;
	int SamplesPerSec;
	int BufferSize;
	s8 *Raw;

	s32 RecordedSampleCount;
};

#include "win32_utils.cpp"
#include "sound_utils.cpp"
#include "gfx_utils.cpp"

global b32 AppRunning;

void RenderWave(win32_bitmap_buffer *Buffer, s16 *Wave, s32 WaveSize,
				s32 Size, s32 Spread, s32 OffSetY, r32 RR, r32 GG, r32 BB,
				s32 ScaleX, s32 ScaleY)	
{	
	for(s32 i = 0; i<WaveSize; i++)
	{
		r32 fBx = (r32)i*(((r32)ScaleX*Spread)/(r32)WaveSize);
		r32 fBy = (r32)Wave[i]*((((r32)ScaleY))/(r32)pow(2, 16));
		
		fBy += OffSetY;		
		DrawRect(Buffer, fBx, fBy, fBx+Size, fBy+Size, RR, GG, BB);
	}
}

LRESULT CALLBACK
WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
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
	HWND Window = GetWindow("nlife-class", "sound-server", 1920, 1080, WindowProc);

	win32_bitmap_buffer BitmapBuffer;
	Win32AllocBitmapBuffer(&BitmapBuffer, 1920, 1080);
	
	Speakers Speaker = {};		
	Speaker.SamplesPerSec = 48000;
	Speaker.BufferSize = (48000*4)/16;
	Speaker.Raw = (s8*)malloc(sizeof(s8)*Speaker.BufferSize);
	
	Microphone Mic = {};	
	Mic.SamplesPerSec = 48000;
	Mic.BufferSize = (48000*4)/16;
	Mic.Raw = (s8*)malloc(sizeof(s8)*Mic.BufferSize);
	
	InitDSoundSpeakers(Window, &Speaker);
	InitDSoundMic(&Mic);
	
	Record(&Mic);
	Play(&Speaker);
		
	MSG Message;
	AppRunning = true;
	   		
	while(AppRunning)
	{
		while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}

		Mic.RecordedSampleCount = 0;
		
		RecordMic(&Mic);
	    RenderSound(&Speaker, (s16*)Mic.Raw, Mic.RecordedSampleCount);
		
		DrawRect(&BitmapBuffer, 0, 0, BitmapBuffer.Width, BitmapBuffer.Height, 0, 0, 0); 
		RenderWave(&BitmapBuffer, (s16*)Mic.Raw, Mic.RecordedSampleCount*2, 10, 20,
				   BitmapBuffer.Height-200, 1.0f, 0.0f, 0.0f,
				   BitmapBuffer.Width, BitmapBuffer.Height);		
						
		HDC DeviceContext = GetDC(Window);
		{
			win32_window_dimension Dimension = Win32WindowDimension(Window);
			Win32BlitBitmap(DeviceContext, &BitmapBuffer, Dimension.Width, Dimension.Height);
		}	   
		ReleaseDC(Window, DeviceContext);				
	}

	WSACleanup();
	return 0;
}
