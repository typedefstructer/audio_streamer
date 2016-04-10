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

		int fh = n/2-i;
		int fl = i - n/2;
		
		// if(fh > 0 && ( fh > 1800))
			// Out[i] = 0;
	  
		// if(fl > 0 && ( fl > 1800))
			// Out[i] = 0;
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
		//if(i<n/3 + n/9 + n/27 || i > 2*n/3 -n/9-n/27)
		// if(i < (n/3 + n/9) || i > (2*n/3 - n/9))
		// if(i<n/3 || i>2*n/3)
		int fh = n/2-i;
		int fl = i - n/2;

		if(fh > 0 && ( fh > 1800))						
			A_re[i] = A_im[i] = 0;

		if(fl > 0 && ( fl > 1800))			
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
