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
