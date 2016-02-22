/*************************************************************************************
			       DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
					   		     IMPERIAL COLLEGE LONDON 

 				      EE 3.19: Real Time Digital Signal Processing
					       Dr Paul Mitcheson and Daniel Harvey

				        		  LAB 3: Interrupt I/O

 				            ********* I N T I O. C **********

  Demonstrates inputing and outputing data from the DSK's audio port using interrupts. 

 *************************************************************************************
 				Updated for use on 6713 DSK by Danny Harvey: May-Aug 2006
				Updated for CCS V4 Sept 10
 ************************************************************************************/
/*
 *	You should modify the code so that interrupts are used to service the 
 *  audio port.
 */
/**************************** Pre-processor statements ******************************/

#include <stdlib.h> 
//  Included so program can make use of DSP/BIOS configuration tool.  
#include "dsp_bios_cfg.h"

/* The file dsk6713.h must be included in every program that uses the BSL.  This 
   example also includes dsk6713_aic23.h because it uses the 
   AIC23 codec module (audio interface). */
#include "dsk6713.h"
#include "dsk6713_aic23.h"

// Some functions to help with writing/reading the audio ports when using interrupts.
#include <helper_functions_ISR.h>

// Filter Coefficients stored in array 'double b[] = {......}'
#include "fir_coef.txt"

// Size of delay buffer = size of coefs array = total bytes/bytes of 1 element
#define BUFSIZE sizeof(b)/sizeof(b[0])
/******************************* Global declarations ********************************/

/* Audio port configuration settings: these values set registers in the AIC23 audio 
   interface to configure it. See TI doc SLWS106D 3-3 to 3-10 for more info. */
DSK6713_AIC23_Config Config = { \
			 /**********************************************************************/
			 /*   REGISTER	            FUNCTION			      SETTINGS         */ 
			 /**********************************************************************/\
    0x0017,  /* 0 LEFTINVOL  Left line input channel volume  0dB                   */\
    0x0017,  /* 1 RIGHTINVOL Right line input channel volume 0dB                   */\
    0x01f9,  /* 2 LEFTHPVOL  Left channel headphone volume   0dB                   */\
    0x01f9,  /* 3 RIGHTHPVOL Right channel headphone volume  0dB                   */\
    0x0011,  /* 4 ANAPATH    Analog audio path control       DAC on, Mic boost 20dB*/\
    0x0000,  /* 5 DIGPATH    Digital audio path control      All Filters off       */\
    0x0000,  /* 6 DPOWERDOWN Power down control              All Hardware on       */\
    0x0043,  /* 7 DIGIF      Digital audio interface format  16 bit                */\
    0x008d,  /* 8 SAMPLERATE Sample rate control             8 KHZ                 */\
    0x0001   /* 9 DIGACT     Digital interface activation    On                    */\
			 /**********************************************************************/
};


// Codec handle:- a variable used to identify audio interface  
DSK6713_AIC23_CodecHandle H_Codec;


// Delay buffer
double x[BUFSIZE];

double x2[2*BUFSIZE];

// Output of filter
double y;

// Input read from signal generator
short sample;

int newest = BUFSIZE - 1;
/******************************* Function prototypes ********************************/
void init_hardware(void);     
void init_HWI(void);          
void ISR_AIC(void);        
void non_circ_FIR(void);
void circ_FIR1(void);
void circ_FIR2(void);
void circ_FIR3(void);
void circ_FIR4(void);
void circ_FIR5(void);
void circ_FIR6(void);
void circ_FIR7(void);

/********************************** Main routine ************************************/
void main(){      
	int i;
// initialize board and the audio port
 	init_hardware();
	
  /* initialize hardware interrupts */
	init_HWI();  	 		
	
	for(i = 0; i < BUFSIZE; i++)
	{
		x[i] = 0;
		x2[i] = 0;
	}
  /* loop indefinitely, waiting for interrupts */  					
	while(1){}
  
}
        
/********************************** init_hardware() **********************************/  
void init_hardware()
{
    // Initialize the board support library, must be called first 
    DSK6713_init();
    
    // Start the AIC23 codec using the settings defined above in config 
    H_Codec = DSK6713_AIC23_openCodec(0, &Config);

	/* Function below sets the number of bits in word used by MSBSP (serial port) for 
	receives from AIC23 (audio port). We are using a 32 bit packet containing two 
	16 bit numbers hence 32BIT is set for  receive */
	MCBSP_FSETS(RCR1, RWDLEN1, 32BIT);	

	/* Configures interrupt to activate on each consecutive available 32 bits 
	from Audio port hence an interrupt is generated for each L & R sample pair */	
	MCBSP_FSETS(SPCR1, RINTM, FRM);

	/* These commands do the same thing as above but applied to data transfers to  
	the audio port */
	MCBSP_FSETS(XCR1, XWDLEN1, 32BIT);	
	MCBSP_FSETS(SPCR1, XINTM, FRM);	
}

/********************************** init_HWI() **************************************/  
void init_HWI(void)
{
	IRQ_globalDisable();			// Globally disables interrupts
	IRQ_nmiEnable();				// Enables the NMI interrupt (used by the debugger)
	IRQ_map(IRQ_EVT_RINT1,4);		// Maps an event to a physical interrupt
	IRQ_enable(IRQ_EVT_RINT1);		// Enables the event
	IRQ_globalEnable();				// Globally enables interrupts
} 

/******************** WRITE YOUR INTERRUPT SERVICE ROUTINE HERE***********************/  

/*
 * Convolves input with filter and outputs
 */
 
void ISR_AIC(void)
{	
	sample = mono_read_16Bit();
	
	circ_FIR4();
	
	  
	mono_write_16Bit((short)y); 
}

void non_circ_FIR(void)
{
	int i;
	for (i = BUFSIZE; i > 0; i--)
		x[i] = x[i-1];
	x[0] = sample;
	
	y=0;
	for (i = BUFSIZE; i > 0; i--)
	{
		y += (x[i] * b[i]);
	}
	
}

// Normal circle
void circ_FIR1(void)
{
	
	int i, j;
	y = 0;
	x[newest] = sample;
	
	for (i = 0, j = newest; i < BUFSIZE; i++,j++)
	{
		if (j >= BUFSIZE)
			j -= BUFSIZE;
			
		 y += (x[j] * b[i]);
	}
	
	// Wrap around to other side of buffer
	if (--newest < 0)
		newest = BUFSIZE - 1;
}

// Split loop
void circ_FIR2(void)
{
	int i;
	y = 0;
	x[newest] = sample; // Read newest sample into buffer
	
	
	// Split convoluton into two loops
	for (i = 0; i < BUFSIZE - newest; i++)
		y += (x[newest+i] * b[i]);
		
	for (; i < BUFSIZE; i++)
		y += (x[i-(BUFSIZE - newest)] * b[i]);
		
	
	// Wrap around to other side of buffer
	if (--newest < 0)
		newest = BUFSIZE - 1;
	
}

// Double size buffer
void circ_FIR3(void)
{
	int i;
	y = 0;
	x2[newest] = x2[newest + BUFSIZE] = sample;
	
	for(i = 0; i < BUFSIZE; i++)
		y += x2[newest +i]*b[i];
	
	if(--newest < 0)
		newest = BUFSIZE -1;		
}

// symmetrical circular buffer 
void circ_FIR4(void)
{
	int i, j, k;
	y = 0;
	x[newest] = sample;
	j = newest;
	
 	for (i = 0, k = newest - 1; i < BUFSIZE/2; i++, k--)
	{
		if(k < 0)
			k += BUFSIZE;
			
		if (j >= BUFSIZE)
			j -= BUFSIZE;
			
		y += ((x[j] + x[k]) * b[i]);
		j++;
	}
	
	// If odd number of coeffs, centre value will be duplicated, so subtract it once
	if (!(BUFSIZE % 2))
	{
		y -= x[j]* b[i];
	}
	// Wrap around to other side of buffer
	if (--newest < 0)
		newest = BUFSIZE - 1;
}

//Optimised verison of 1
void circ_FIR5(void)
{
	
	int i;
	
	double *x_limit = x + BUFSIZE;
	double *x_ptr = x + newest;// + newest;
	double *b_ptr = b;
	register double temp =  sample * *b_ptr++;
	*x_ptr++ = sample;
	
	for (i = 1; i < BUFSIZE; i++)
	{
		if (x_ptr >= x_limit)
			x_ptr -= BUFSIZE;
			
		 temp += (*x_ptr++ * *b_ptr++);
	}
	y = temp;
	// Wrap around to other side of buffer
	if (--newest < 0)
		newest = BUFSIZE - 1;
}

//Optimised version of 4
void circ_FIR6(void)
{
	/*int i, j, k;
	y = 0;
	x[newest] = sample;
	*/
		
	int i;
	double *j;
	double *k;
	double *x_limit = x + BUFSIZE;
	double *x_ptr = x + newest;// + newest;
	double *b_ptr = b;
	register double temp =  sample * *b_ptr++;
	*x_ptr++ = sample;
	
 	for (i = 0, j = x_ptr, k = x_ptr -1; i < BUFSIZE/2; i++, j++, k--)
	{
		if(k < x)
			k += BUFSIZE;
			
		if (j >= x_limit)
			j -= BUFSIZE;
			
		temp += ((*j + *k) * *(b_ptr + i));
	}
	
	// If odd number of coeffs, centre value will be duplicated, so subtract it once
	if (!(BUFSIZE % 2))
	{
		temp -= *j * *(b_ptr + i);
	}
	
	y = temp;
	// Wrap around to other side of buffer
	if (--newest < 0)
		newest = BUFSIZE - 1;
}

//Optimisation of 2
void circ_FIR7(void)
{
	int i;
	double *x_limit = x + BUFSIZE;
	double *x_ptr = x + newest;// + newest;
	double *b_ptr = b;
	register double temp =  sample * *b_ptr++;
	*x_ptr++ = sample;
	
	
	// Split convoluton into two loops
	for (i = 1; i < BUFSIZE - newest; i++)
		temp += (*x_ptr++ * *b_ptr++);
		
	for (; i < BUFSIZE; i++)
		temp += (*(x_ptr++ - BUFSIZE) * *b_ptr++);
		
	y = temp;
	// Wrap around to other side of buffer
	if (--newest < 0)
		newest = BUFSIZE - 1;
	
}
