#define _WINSOCKAPI_
#include<windows.h>
#include<winsock2.h>
#include<ws2tcpip.h>

#include<dsound.h>
#include<stdint.h>
#include<math.h>
#include<stdio.h>

#define global static

global LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global LPDIRECTSOUNDCAPTUREBUFFER GlobalCaptureBuffer;
global bool AppRunning;
global int client_socket = -1;
global int server_socket;
void CopyRecordBuffer(char *Capture, void *Capture1, DWORD Capture1Bytes,
					  void *Capture2, DWORD Capture2Bytes, int *RecIndex)
{			
	int16_t *CapStore = (int16_t *)Capture;
	int16_t *CapIn = (int16_t *)Capture1;
	
	for(int i=0;i<Capture1Bytes/4; i++)
	{
		*CapStore++ = *CapIn++;
		*CapStore++ = *CapIn++;
		(*RecIndex) += 1;
	}


	CapIn = (int16_t*)Capture2;
	for(int i=0;i<Capture2Bytes/4; i++)
	{
		*CapStore++ = *CapIn++;
		*CapStore++ = *CapIn++;
		(*RecIndex) += 1;
	}

}


void Record(char *Capture, int *RecIndex, int BufferSize)
{	
	DWORD CaptureCursor;
	DWORD ReadCursor;
	DWORD LockCapture;
	DWORD CaptureRegion;
	void *Capture1, *Capture2;
	DWORD Capture1Bytes, Capture2Bytes;
		
	GlobalCaptureBuffer->GetCurrentPosition(&CaptureCursor, &ReadCursor);
	if(CaptureCursor >= ReadCursor)
	{
		LockCapture = CaptureCursor+4;			
		CaptureRegion = BufferSize-LockCapture+ReadCursor;
	}
	else
	{
		LockCapture = CaptureCursor+4;		
		CaptureRegion =  ReadCursor - LockCapture;					
	}

	for(int i=0;i<BufferSize;i++) Capture[i] = 0;		
	GlobalCaptureBuffer->Lock(LockCapture, CaptureRegion, &Capture1, &Capture1Bytes,
							  &Capture2, &Capture2Bytes, 0);


	CopyRecordBuffer(Capture, Capture1, Capture1Bytes, Capture2, Capture2Bytes, RecIndex);
		
	GlobalCaptureBuffer->Unlock(Capture1, Capture1Bytes, Capture2, Capture2Bytes);		
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

void InitDSoundRec(int BufferSize)
{	
	LPDIRECTSOUNDCAPTURE8 DirectSoundCapture;
	DSCBUFFERDESC SecondaryBufferDesc = {};
	LPDIRECTSOUNDCAPTUREBUFFER CaptureBuffer;
	WAVEFORMATEX WaveFormat = {};
   
	WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat.nChannels  = 2;
    WaveFormat.nSamplesPerSec = 48000;
    WaveFormat.wBitsPerSample = 16;
    WaveFormat.nBlockAlign = WaveFormat.wBitsPerSample * WaveFormat.nChannels / 8;    
	WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

	SecondaryBufferDesc.dwSize  = sizeof(DSBUFFERDESC);	
	SecondaryBufferDesc.dwBufferBytes = BufferSize;
	SecondaryBufferDesc.lpwfxFormat = &WaveFormat;   
	
	DirectSoundCaptureCreate8(0, &DirectSoundCapture, 0);	
	DirectSoundCapture->CreateCaptureBuffer(&SecondaryBufferDesc, &CaptureBuffer, 0);
	GlobalCaptureBuffer = CaptureBuffer;
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

void InitSocket()
{
	WSADATA wd;
	WSAStartup(MAKEWORD(2, 2), &wd);
}

DWORD AcceptThread(void *arg)
{
	while(1)
	client_socket = accept(server_socket, 0, 0);
	return 0;
}

int WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
	
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = windowproc;
	wc.hInstance = Instance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = "nlife_shadow";

	RegisterClassEx(&wc);
	HWND Window = CreateWindowEx(0, wc.lpszClassName, "nlife_shadow",
							   WS_OVERLAPPEDWINDOW | WS_VISIBLE,
							   CW_USEDEFAULT, CW_USEDEFAULT,
							   400, 200, 0, 0, Instance, 0);


	const DWORD BufferSize = (48000*4)/16;

    InitSocket();
	InitDSound(Window, BufferSize); // 1/16 Seconds Worth Of Sound	
	InitDSoundRec(BufferSize);	

	addrinfo *server = 0, server_hints = {};
	server_hints.ai_family = AF_INET;
	server_hints.ai_socktype = SOCK_STREAM;
	server_hints.ai_protocol = IPPROTO_TCP;
	server_hints.ai_flags = AI_PASSIVE;

	getaddrinfo("0.0.0.0", "1295", &server_hints, &server);
	
	server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	bind(server_socket, server->ai_addr, (int)server->ai_addrlen);
	listen(server_socket, 15);
	
	MSG Message;
		
	AppRunning = true;	
	char Capture[BufferSize];
	int RecIndex = 0;
	
	GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
	GlobalCaptureBuffer->Start(DSCBSTART_LOOPING);

	CreateThread(0, 0, AcceptThread, 0, 0, 0);
	while(AppRunning)
	{
		while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
		
		Record(Capture, &RecIndex, BufferSize);		
		if(client_socket != -1)
			if(send(client_socket, Capture, BufferSize, 0) <= 0)
			{				
				client_socket = -1;
			}
		
		//PlayBack(Capture, RecIndex, BufferSize);		
	}
	
	WSACleanup();
	return 0;
}
