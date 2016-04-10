void InitDSoundSpeakers(HWND Window, Speakers *Speaker)
{
	LPDIRECTSOUND DirectSound;	
	DSBUFFERDESC PrimaryBufferDesc = {}, SecondaryBufferDesc = {};
	LPDIRECTSOUNDBUFFER PrimaryBuffer = {}, SecondaryBuffer  = {};
	WAVEFORMATEX WaveFormat = {};   
	
	WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat.nChannels  = 2;
    WaveFormat.nSamplesPerSec = Speaker->SamplesPerSec;
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
	SecondaryBufferDesc.dwBufferBytes = Speaker->BufferSize;
	SecondaryBufferDesc.lpwfxFormat = &WaveFormat;

	DirectSound->CreateSoundBuffer(&SecondaryBufferDesc, &SecondaryBuffer, 0);	
	Speaker->SoundBuffer = SecondaryBuffer;
}

void InitDSoundMic( Microphone *Mic)
{
	LPDIRECTSOUNDCAPTURE8 DirectSoundCapture;
	DSCBUFFERDESC SecondaryBufferDesc = {};
	LPDIRECTSOUNDCAPTUREBUFFER CaptureBuffer;
	WAVEFORMATEX WaveFormat = {};

	WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.nChannels = 2;
	WaveFormat.nSamplesPerSec = Mic->SamplesPerSec;
	WaveFormat.wBitsPerSample = 16;
	WaveFormat.nBlockAlign  = WaveFormat.wBitsPerSample * WaveFormat.nChannels / 8;
	WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

	SecondaryBufferDesc.dwSize = sizeof(DSCBUFFERDESC);
	SecondaryBufferDesc.dwBufferBytes = Mic->BufferSize;
	SecondaryBufferDesc.lpwfxFormat = &WaveFormat;

	DirectSoundCaptureCreate8(0, &DirectSoundCapture, 0);
	DirectSoundCapture->CreateCaptureBuffer(&SecondaryBufferDesc, &CaptureBuffer, 0);
	Mic->SoundBuffer = CaptureBuffer;
}

void
RenderSound(Speakers *Speaker, s16 *Input, s32 SampleCount)
{
	void *Region1, *Region2;
	DWORD Region1Bytes, Region2Bytes;
	DWORD PlayCursor;
	DWORD WriteCursor;
	DWORD LockByte, RegionSize;

	Speaker->SoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);	
	if(WriteCursor >= PlayCursor)
	{
		LockByte = WriteCursor;
		RegionSize = Speaker->BufferSize - WriteCursor + PlayCursor;
	}
	else
	{
		LockByte = WriteCursor;
		RegionSize = PlayCursor - WriteCursor;
	}
	
	Speaker->SoundBuffer->Lock(LockByte, RegionSize, &Region1, &Region1Bytes, &Region2, &Region2Bytes, 0);
	
	s16 *Sample = (s16*)Region1;
	s16 *Buffer = (s16*)Input;	
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
		PlayBackIndex += 1;
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
		PlayBackIndex += 1;
	}

    Speaker->SoundBuffer->Unlock(Region1, Region1Bytes, Region2, Region2Bytes);	
}

void RecordMic(Microphone *Mic)
{
	void *Region1, *Region2;
	DWORD Region1Bytes, Region2Bytes;
	DWORD ReadCursor, CaptureCursor;
	DWORD LockByte, RegionSize;

	Mic->SoundBuffer->GetCurrentPosition(&CaptureCursor, &ReadCursor);
	if(CaptureCursor >= ReadCursor)
	{
		LockByte = CaptureCursor + 4;
		RegionSize = Mic->BufferSize - LockByte + ReadCursor;
	}
	else
	{
		LockByte = CaptureCursor + 4;
		RegionSize = ReadCursor - LockByte;
	}

	Mic->SoundBuffer->Lock(LockByte, RegionSize, &Region1, &Region1Bytes, &Region2, &Region2Bytes, 0);

	s16 *MicIn  = (s16*)Region1;
	s16 *Buffer = (s16*)Mic->Raw;
	for(int i=0;i<Region1Bytes/4;i++)
	{
		*Buffer++ = *MicIn++;
		*Buffer++ = *MicIn++;		
		Mic->RecordedSampleCount += 1;
	}

	MicIn = (s16*)Region2;
	for(int i=0;i<Region2Bytes/4;i++)
	{
		*Buffer++ = *MicIn++;
		*Buffer++ = *MicIn++;
		Mic->RecordedSampleCount += 1;		
	}
	
	Mic->SoundBuffer->Unlock(Region1, Region1Bytes, Region2, Region2Bytes);
}

void Record(Microphone *Mic)
{
	Mic->SoundBuffer->Start(DSCBSTART_LOOPING);
}

void Play(Speakers *Speaker)
{
	Speaker->SoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
}
