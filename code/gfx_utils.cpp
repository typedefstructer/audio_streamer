void Win32BlitBitmap(HDC DeviceContext, win32_bitmap_buffer *Buffer,
					 int WindowWidth, int WindowHeight)
{
	int OffSetX = (WindowWidth - Buffer->Width)/2;
	int OffSetY = (WindowHeight- Buffer->Height)/2;

	PatBlt(DeviceContext, 0, 0, OffSetX, WindowHeight, BLACKNESS);
	PatBlt(DeviceContext, OffSetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);

	PatBlt(DeviceContext, 0, 0, WindowWidth, OffSetY, BLACKNESS);
	PatBlt(DeviceContext, 0, OffSetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
	
	StretchDIBits(DeviceContext, OffSetX, OffSetY, Buffer->Width, Buffer->Height,
				  0, 0, Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info,
				  DIB_RGB_COLORS, SRCCOPY);
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
