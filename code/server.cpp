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

#include "unifft.cpp"

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

global b32 AppRunning;

void Win32BlitBitmap(HDC DeviceContext, win32_bitmap_buffer *Buffer, int WindowWidth, int WindowHeight)
{
	int OffSetX = (WindowWidth - Buffer->Width)/2;
	int OffSetY = (WindowHeight- Buffer->Height)/2;

	PatBlt(DeviceContext, 0, 0, OffSetX, WindowHeight, BLACKNESS);
	PatBlt(DeviceContext, OffSetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);

	PatBlt(DeviceContext, 0, 0, WindowWidth, OffSetY, BLACKNESS);
	PatBlt(DeviceContext, 0, OffSetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
	
	StretchDIBits(DeviceContext, OffSetX, OffSetY, Buffer->Width, Buffer->Height,
				  0, 0, Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

void Win32AllocBitmapBuffer(win32_bitmap_buffer *Buffer, int Width, int Height)
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

	Buffer->Width = Width;
	Buffer->Height = Height;
	int BitmapBufferSize = Width * Height * 4;
	Buffer->Memory = VirtualAlloc(0, BitmapBufferSize, MEM_COMMIT, PAGE_READWRITE);
}

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

void InitDSoundSpeakers(HWND Window, int BufferSize, LPDIRECTSOUNDBUFFER *SoundBuffer)
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
	*SoundBuffer = SecondaryBuffer;
}

void InitDSoundMic(int BufferSize, LPDIRECTSOUNDCAPTUREBUFFER *SoundCaptureBuffer)
{
	LPDIRECTSOUNDCAPTURE8 DirectSoundCapture;
	DSCBUFFERDESC SecondaryBufferDesc = {};
	LPDIRECTSOUNDCAPTUREBUFFER CaptureBuffer;
	WAVEFORMATEX WaveFormat = {};

	WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.nChannels = 2;
	WaveFormat.nSamplesPerSec = 48000;
	WaveFormat.wBitsPerSample = 16;
	WaveFormat.nBlockAlign  = WaveFormat.wBitsPerSample * WaveFormat.nChannels / 8;
	WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

	SecondaryBufferDesc.dwSize = sizeof(DSCBUFFERDESC);
	SecondaryBufferDesc.dwBufferBytes = BufferSize;
	SecondaryBufferDesc.lpwfxFormat = &WaveFormat;

	DirectSoundCaptureCreate8(0, &DirectSoundCapture, 0);
	DirectSoundCapture->CreateCaptureBuffer(&SecondaryBufferDesc, &CaptureBuffer, 0);
	*SoundCaptureBuffer = CaptureBuffer;
}

void InitSocket()
{
	WSADATA wd;
	WSAStartup(MAKEWORD(2, 2), &wd);
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


int socket_listen(char *ip, char *port)
{
	addrinfo *server = 0, server_hints = {};
	server_hints.ai_family = AF_INET;
	server_hints.ai_socktype = SOCK_STREAM;
	server_hints.ai_protocol = IPPROTO_TCP;
	server_hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, port, &server_hints, &server);

	int server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	bind(server_socket, server->ai_addr, (int)server->ai_addrlen);
	listen(server_socket, 16);

	return server_socket;
}

void RenderSound(LPDIRECTSOUNDBUFFER SoundBuffer, s16 *Buffer, int SampleCount, int BufferSize)
{
	void *Region1, *Region2;
	DWORD Region1Bytes, Region2Bytes;
	DWORD PlayCursor;
	DWORD WriteCursor;
	DWORD LockByte, RegionSize;

	SoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
	if(WriteCursor >= PlayCursor)
	{
		LockByte = WriteCursor;
		RegionSize = BufferSize - WriteCursor + PlayCursor;
	}
	else
	{
		LockByte = WriteCursor;
		RegionSize = PlayCursor - WriteCursor;
	}

	SoundBuffer->Lock(LockByte, RegionSize, &Region1, &Region1Bytes, &Region2, &Region2Bytes, 0);
	s16 *Sample = (s16*)Region1;
	s32 PlayBackIndex = 0;
	for(int i=0;i<Region1Bytes/4;i++)
	{
		if(PlayBackIndex < SampleCount)
		{
			*Sample++ = *Buffer++;
			*Sample++ = *Buffer++;
		}
		else
		{
			*Sample++ = 0;
			*Sample++ = 0;
		}
		PlayBackIndex += 2;
	}

	Sample = (s16*)Region2;
	for(int i=0;i<Region2Bytes/4;i++)
	{
		if(PlayBackIndex < SampleCount)
		{
			*Sample++ = *Buffer++;
			*Sample++ = *Buffer++;
		}
		else
		{
			*Sample++ = 0;
			*Sample++ = 0;
		}
		PlayBackIndex += 2;
	}

    SoundBuffer->Unlock(Region1, Region1Bytes, Region2, Region2Bytes);	
}

void RecordMic(LPDIRECTSOUNDCAPTUREBUFFER MicBuffer, s16 *Buffer, int BufferSize, s32 *Count)
{
	void *Region1, *Region2;
	DWORD Region1Bytes, Region2Bytes;
	DWORD ReadCursor, CaptureCursor;
	DWORD LockByte, RegionSize;

	MicBuffer->GetCurrentPosition(&CaptureCursor, &ReadCursor);
	if(CaptureCursor >= ReadCursor)
	{
		LockByte = CaptureCursor + 4;
		RegionSize = BufferSize - LockByte + ReadCursor;
	}
	else
	{
		LockByte = CaptureCursor + 4;
		RegionSize = ReadCursor - LockByte;
	}

	MicBuffer->Lock(LockByte, RegionSize, &Region1, &Region1Bytes, &Region2, &Region2Bytes, 0);

	s16 *MicIn = (s16*)Region1;
	for(int i=0;i<Region1Bytes/4;i++)
	{
		*Buffer++ = *MicIn++;
		*Buffer++ = *MicIn++;		
		*Count += 2;		
	}

	MicIn = (s16*)Region2;
	for(int i=0;i<Region2Bytes/4;i++)
	{
		*Buffer++ = *MicIn++;
		*Buffer++ = *MicIn++;
		*Count += 2;		
	}

	
	MicBuffer->Unlock(Region1, Region1Bytes, Region2, Region2Bytes);
}

void DrawRect(win32_bitmap_buffer *Buffer, r32 RMinX, r32 RMinY, r32 RMaxX, r32 RMaxY,
			  r32 RR, r32 GG, r32 BB)
{
	s32 MinX = RMinX;
	s32 MaxX = RMaxX;
	s32 MinY = RMinY;
	s32 MaxY = RMaxY;

	MinY = Buffer->Height - MinY;
	MaxY = Buffer->Height - MaxY;


	s32 Tmp = MinY;
	MinY = MaxY;
	MaxY = Tmp;
	
	// printf("%d %d\n", MinY, MaxY);
	if(MinX < 0)
	{
		MinX = 0;		
	}

	if(MinY < 0)
	{
		MinY = 0;		
	}

	if(MaxX < 0)
	{
		MaxX = 0;		
	}

	if(MaxY < 0)
	{
		MaxY = 0;		
	}
		
	if(MinX > Buffer->Width)
	{
		MinX = Buffer->Width;
	}

	if(MaxY > Buffer->Height)
	{
		MinY = Buffer->Height;
	}

	if(MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}

	if(MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}
	
	u32 R = RR * 255.0f;
	u32 G = GG * 255.0f;
	u32 B = BB * 255.0f;
	u32 A = 0;
	u32 Color = A<<24|R<<16|G<<8|B;

	u32 *Row = (u32 *)Buffer->Memory + MinY * Buffer->Width + MinX;
	for(u32 Y=MinY; Y<MaxY; Y++)
	{
		u32 *Pixel = Row;
		for(u32 X=MinX; X<MaxX; X++)
		{
			*Pixel++ = Color;
		}
		Row += Buffer->Width;
	}	
}
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


void Zero(s8 *Buffer, s32 BufferSize)
{
	for(int i=0;i<BufferSize;i++)
	{
		Buffer[i] = 0;
	}
}

void Copy(s8 *Out, s8 *In, s32 BufferSize)
{
	for(int i=0;i<BufferSize;i++)
	{
		Out[i] = In[i];
	}
}

#define LOWFREQ 3000
#define HIGHFREQ 30000

void FFT(s16 *Out, s16 *In, s32 SampleCount)
{
	double *A_re, *A_im, *W_re, *W_im;
	double *A_ma;
	
	int n = pow(2, ceil(log(SampleCount)/log(2)));
		
	A_re = (double*)malloc(sizeof(double)*n); 
	A_im = (double*)malloc(sizeof(double)*n); 
	A_ma = (double*)malloc(sizeof(double)*n); 
	W_re = (double*)malloc(sizeof(double)*n/2); 
	W_im = (double*)malloc(sizeof(double)*n/2);
	
	
	Zero((s8*)A_re, sizeof(double)*n);
	Zero((s8*)A_im, sizeof(double)*n);

	for(int i=0;i<SampleCount;i++)
	{
		A_re[i] = In[i];
	}

	
	compute_W(n, W_re, W_im);
	fft(n, A_re, A_im, W_re, W_im);
	permute_bitrev(n, A_re, A_im);

	for(int i=0;i<n;i++)
	{
		A_re[i] = A_re[i]/n;
		A_im[i] = A_im[i]/n;
		A_ma[i] = sqrt(A_re[i]*A_re[i] + A_im[i]*A_im[i])*100;
	}
		
	for(int i=0;i<n;i++)
	{
		Out[i] = A_ma[i];
		//if(i<n/3 || i > 2*n/3) Out[i] = 0;		
	}
	
	free(A_re);
	free(A_ma);
	free(A_im); 
	free(W_re); 
	free(W_im); 
}

void IFFT(s16 *Out, s16 *In, s16* Ift, s32 SampleCount)
{
	double *A_re, *A_im, *W_re, *W_im;
	double *A_ma;
	
	int n = pow(2, ceil(log(SampleCount)/log(2)));
		
	A_re = (double*)malloc(sizeof(double)*n); 
	A_im = (double*)malloc(sizeof(double)*n); 
	A_ma = (double*)malloc(sizeof(double)*n); 
	W_re = (double*)malloc(sizeof(double)*n/2); 
	W_im = (double*)malloc(sizeof(double)*n/2);
	
	
	Zero((s8*)A_re, sizeof(double)*n);
	Zero((s8*)A_im, sizeof(double)*n);
	
	for(int i=0;i<SampleCount;i++)
	{		
		A_re[i] = In[i];					
	}
	
	compute_W(n, W_re, W_im);
	fft(n, A_re, A_im, W_re, W_im);			
	permute_bitrev(n, A_re, A_im);

	for(int i=0;i<n;i++)
	{
		if(i<n/3 || i > 2*n/3)
		// if(i < (n/3 + n/9) || i > (2*n/3 - n/9))
			A_re[i] = A_im[i] = 0; 
	}
	
	fft(n, A_re, A_im, W_re, W_im);
	permute_bitrev(n, A_re, A_im);
	
	for(int i=0;i<n;i++)
	{
		A_re[i] = A_re[i]/(n/2);
		A_im[i] = -A_im[i]/(n/2);
	    A_ma[i] = A_re[i]+A_im[i];		
	}
	
	for(int i=0;i<SampleCount;i++)
	{
		Out[i] = A_ma[n-i]*5;
		Ift[i] = Out[i];
	}
		
	free(A_re);
	free(A_ma);
	free(A_im); 
	free(W_re); 
	free(W_im); 	
}

int WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{	
	HWND Window = GetWindow("nlife-class", "sound-server", 1920, 1080, WindowProc);
	const DWORD BufferSize = (48000*4)/16; //48000KHz And Each Sample Is 4 Bytes for One Second

	LPDIRECTSOUNDBUFFER Speaker;
	LPDIRECTSOUNDCAPTUREBUFFER Mic;
	win32_bitmap_buffer BitmapBuffer;
		
	InitSocket();	
	InitDSoundSpeakers(Window, BufferSize, &Speaker);
	InitDSoundMic(BufferSize, &Mic);

	int server_socket = socket_listen("0.0.0.0", "1295"); // The App Listen On This address

	Win32AllocBitmapBuffer(&BitmapBuffer, 1920, 1080); // 400 x 200 Bitmap to be displayed in the gui	
	MSG Message;
	AppRunning = true;

	s8 Capture[BufferSize];
	s8 Raw[BufferSize];
	s8 Fourier[BufferSize];
	s8 Inverse[BufferSize];
		
	Mic->Start(DSCBSTART_LOOPING);
	Speaker->Play(0, 0, DSBPLAY_LOOPING);
	int i=0, j;
	while(AppRunning)
	{
		while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}

		Zero(Capture, BufferSize);
		Zero(Fourier, BufferSize);
		Zero(Inverse, BufferSize);
				
		s32 SampleCount = 0;
		RecordMic(Mic, (s16*)Capture, BufferSize, &SampleCount);		
		Copy(Raw, Capture, BufferSize);
		
		int n = pow(2, ceil(log(SampleCount)/log(2)));
		s16 *Ft = (s16*)malloc(n*sizeof(s16));
		s16 *IFt = (s16*)malloc(n*sizeof(s16));
		
		FFT(Ft, (s16*)Raw, SampleCount);
		IFFT((s16*)Inverse, (s16*)Raw, IFt, SampleCount);	   

		DrawRect(&BitmapBuffer, 0, 0, BitmapBuffer.Width, BitmapBuffer.Height, 0, 0, 0);


		r32 LxO = n/3;
		r32 HxO = 2*n/3;
		
		r32 LX = LxO*((r32)BitmapBuffer.Width/(r32)n);
		r32 HX = HxO*((r32)BitmapBuffer.Width/(r32)n);
		r32 SX = SampleCount*((r32)BitmapBuffer.Width/(r32)n);

		
		
		DrawRect(&BitmapBuffer, LX, 0,
				 LX+1, BitmapBuffer.Height, 1, 1, 1);
		
		DrawRect(&BitmapBuffer, HX, 0,
				 HX+1, BitmapBuffer.Height, 1, 1, 1);

		// DrawRect(&BitmapBuffer, SX, 0,
				 // SX+1, BitmapBuffer.Height, 0, 1, 1);

		
		RenderWave(&BitmapBuffer, (s16*)Capture, SampleCount, 10, 20,
				   BitmapBuffer.Height-200, 1.0f, 0.0f, 0.0f,
				   BitmapBuffer.Width, BitmapBuffer.Height);		

		RenderWave(&BitmapBuffer, (s16*)Ft, n, 5, 1,
				   20, 1.0f, 0.0f, 1.0f, BitmapBuffer.Width, BitmapBuffer.Height);

		RenderWave(&BitmapBuffer, (s16*)IFt, n, 5, 10,
				   BitmapBuffer.Height/2, 1.0f, 1.0f, 0.0f, BitmapBuffer.Width,
				   BitmapBuffer.Height);

		free(Ft);
		free(IFt);
		HDC DeviceContext = GetDC(Window);
		{
			win32_window_dimension Dimension = Win32WindowDimension(Window);
			Win32BlitBitmap(DeviceContext, &BitmapBuffer, Dimension.Width, Dimension.Height);
		}
		
		ReleaseDC(Window, DeviceContext);		
		RenderSound(Speaker, (s16*)Inverse, SampleCount, BufferSize);
	}

	WSACleanup();
	return 0;
}
