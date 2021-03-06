#define IISFIFO	(*(volatile unsigned int *)0x55000010)
#define IISCON	(*(volatile unsigned int *)0x55000000)
#define IISMOD	(*(volatile unsigned int *)0x55000004)
#define IISFCON	(*(volatile unsigned int *)0x5500000C)
#define IISPSR	(*(volatile unsigned int *)0x55000008)

#define GPECON	(*(volatile unsigned int *)0x56000040)

void IIS_init(void)
{
	// GPE0,1,2,3,4
	// [9:8]	10 = I2SDO 
	// [7:6]	10 = I2SDI   
	// [5:4]	10 = CDCLK   
	// [3:2]	10 = I2SSCLK 
	// [1:0]	10 = I2SLRCK  
	GPECON |= 1<<1 | 1<<3 | 1<<5 | 1<<7 | 1<<9;
	
	// IIS init
	// IIS interface [0] 	0 = Disable (stop)
	//			1 = Enable  (start)
	IISCON |= 1<<0;
	
	// Transmit/receive mode select
	// [7:6] 00 = No transfer      01 = Receive mode
	//	 10 = Transmit mode    11 = Transmit and receive mode	
	IISMOD |= 1<<7;
	
	// Transmit FIFO [13] 0 = Disable     1 = Enable
	IISFCON |= 1<<13;

	// Master clock frequency select
	//	[2] 0 = 256fs        1 = 384fs
	//	(fs: sampling frequency)
	IISMOD |= 1<<2;	
	
	// IIS prescaler [1] 0 = Disable	1 = Enable
	IISCON |= 1<<1;	
	
	// PCLK / (N+1) = master clock
	// master clock = 384 * 22Khz
	// PCLK = 50M = 50000 Khz
	// N+1 = 50000Khz/ (384*22Khz) = 5.91 = 6
	IISPSR = 5<<5 | 5<<0;		// 22Khz
	// IISPSR = 2<<5 | 2<<0;	// 44Khz
	
	// Serial data bit per channel
	//	[3] 	0 = 8-bit         1 = 16-bit
	IISMOD |= 1<<3;
	
	// Serial bit clock frequency select
	//	[1:0] 	00 = 16fs        01 = 32fs
	//		10 = 48fs        11 = N/A
	IISMOD |= 1<<0;	
}

void IIS_trans_data(short * pdata, int size)
{
	while (size > 0)
	{
		while((IISCON & (1<<7)) == (1<<7))
			;
		
		IISFIFO = *pdata;	
		
		pdata++;
		
		size -= 2;
	}
	
	return;
}

void IIS_trans_data_dma(short * pdata, int size)
{
	//init IIS = DMA mode
	//Transmit FIFO access mode select [15] 0 = Normal 1 = DMA
	IISFCON |= 1<<15;
	//init dma mem->IISFIFO
	//Transmit DMA service request [5] 0 = Disable 1 = Enable
	IISCON |= 1<<5;
	
#define DISRC2 		(*(volatile unsigned int *)0x4B000080)
#define DISRCC2 	(*(volatile unsigned int *)0x4B000084)
#define DIDST2 		(*(volatile unsigned int *)0x4B000088)
#define DIDSTC2 	(*(volatile unsigned int *)0x4B00008C)
#define DCON2 		(*(volatile unsigned int *)0x4B000090)
#define DMASKTRIG2 	(*(volatile unsigned int *)0x4B0000A0)
	//start dma
	DISRC2 = (int)pdata;
	DISRCC2 = 0x0;
	DIDST2 = (int)0x55000010;
	DIDSTC2 = 1<<1 | 1<<0;
	DCON2 = (unsigned int)size/2;
	DCON2 |= 1<<23 | 1<<20 | 1<<22;
	DMASKTRIG2 = 1<<1;
	
	return;
}


short * get_wavdata(int wav_addr, int * size)
{		
	int offset;
	char c0, c1, c2, c3;
	short *p_wavdata;
	char * p_datasize;

	offset = *(int *)(wav_addr + 0x10);
	if (offset == 0x10)
	{	
		p_datasize = (char *)(wav_addr + 0x24+4);
		p_wavdata = (short *)(wav_addr+0x24+4+4);
	}
	else
	{
		p_datasize = (char *)(wav_addr + 0x26+4);
		p_wavdata = (short *)(wav_addr+0x26+4+4);
	}
	
	c0 = *(p_datasize+0);
	c1 = *(p_datasize+1);
	c2 = *(p_datasize+2);
	c3 = *(p_datasize+3);
	
	*size = (c0<<0) | (c1<<8) | (c2<<16) | (c3<<24);
	
	putint_hex(size);
	
	return p_wavdata;
}

void IIS_playwav_dma(int wav_addr)
{
	short * p_wavdata;	
	int size = 0x100000;
	
	p_wavdata = get_wavdata(wav_addr, &size);
	
	IIS_trans_data_dma(p_wavdata, size);
}

void IIS_playwav(int wav_addr)
{	
	short * p_wavdata;	
	int size = 0x100000;
	
	p_wavdata = get_wavdata(wav_addr, &size);
	
	IIS_trans_data(p_wavdata, size);
}