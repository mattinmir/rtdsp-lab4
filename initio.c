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
#define ODD_COEF_LENGTH BUFSIZE%2

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
short x[BUFSIZE];

short x2[2*BUFSIZE];

// Output of filter
double y = 0;

// Input read from signal generator
short sample = 0;

// Index for most recent piece of data
// Initialised to end of buffer
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
void circ_FIR8_odd(void);
void circ_FIR8_even(void);
void circ_FIR9_even(void);
void circ_FIR9_odd(void);
void circ_FIR10_odd(void);
void circ_FIR10_even(void);
/********************************** Main routine ************************************/
void main(){    
	int q; 
// initialize board and the audio port
 	init_hardware();
	
  /* initialize hardware interrupts */
	init_HWI();  	 		
	
	for(q = 0; q < BUFSIZE; q++)
	{
		x[q] = 0;
		x2[q] = 0;
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
	
	//if(ODD_COEF_LENGTH)
		circ_FIR10_odd();
	//else
	//	circ_FIR8_even();
	  
	mono_write_16Bit((short)y); 
	
}

//Initial non-circular FIR filter implementation
void non_circ_FIR(void)
{
	int i;
	
	// Shift the sampled values down the buffer to implement 
	// the delay
	for (i = BUFSIZE; i >= 0; i--)
		x[i] = x[i-1];
		
	// Insert the newest sample at the beginning of the buffer or 
	// delay line	
	x[0] = sample;
	
	// Perform the MAC operation. Note that the output (y) 
	// must be set to zero before each operation 
	y=0;
	for (i = BUFSIZE; i >= 0; i--)
		y += (x[i] * b[i]);
}

// Normal circle
void circ_FIR1(void)
{
	unsigned int i, j;
	y = 0;
	
	// Read newest sample into buffer
	x[newest] = sample;
	
	// Iterate over data and filter buffers performing MAC
	for (i = 0, j = newest; i < BUFSIZE; i++,j++)
	{
		// Loop index to beginning of array if there is overflow
		if (j >= BUFSIZE)
			j -= BUFSIZE;
			
		 y += (x[j] * b[i]);
	}
	
	// Decrement pointer for next new data
	// If underflow occurs, wrap around to other side of buffer 
	if (--newest < 0)
		newest = BUFSIZE - 1;
}

// Split loop
void circ_FIR2(void)
{
	unsigned int i;
	y = 0;
	
	// Read newest sample into buffer
	x[newest] = sample; 
	
	// Split convoluton into two loops to save overhead of if statement
	for (i = 0; i < BUFSIZE - newest; i++)
		y += (x[newest+i] * b[i]);
	
	// No need to initialise i again as it is already at the correct value
	for (; i < BUFSIZE; i++)
		y += (x[i-(BUFSIZE - newest)] * b[i]);
	
	// Wrap around to other side of buffer
	if (--newest < 0)
		newest = BUFSIZE - 1;
}

// Double size buffer
void circ_FIR3(void)
{
	unsigned int i;
	y = 0;
	
	// Read newest sample into buffer at newest index and offset index
	x2[newest] = x2[newest + BUFSIZE] = sample;
	
	// Since any stretch of BUFSIZE elements will contain all the data values,
	// we can perform the convolution in one pass and not have to worry
	// about accounting for index overflow
	for(i = 0; i < BUFSIZE; i++)
		y += x2[newest +i]*b[i];
		
	// Decrement pointer for next new data
	// If underflow occurs, wrap around to other side of buffer 
	if(--newest < 0)
		newest = BUFSIZE -1;		
}

// Symmetrical circular buffer 
void circ_FIR4(void)
{
	unsigned int i, j;
	int k;
	y = 0;
	
	// Read newest sample into buffer
	x[newest] = sample;
	
	// Filter buffer is symmertical, so only need to use the first half of the values
	// and condense convolution into 
	// b[0]*(x[0]+x[BUFSIZE-1]) + b[1]*(x[1]+x[BUFSIZE-2]) + ...
 	for (i = 0, j = newest, k = newest - 1; i < BUFSIZE/2; i++, j++, k--)
	{
		// Checking for under/overflow of indexes
		if(k < 0)
			k += BUFSIZE;
			
		if (j >= BUFSIZE)
			j -= BUFSIZE;
			
		// Perform MAC
		y += ((x[j] + x[k]) * b[i]);
	}
	
	// If odd number of coeffs, centre value will be missed, so add it once
	if (!(BUFSIZE % 2))
		y += x[j]* b[i];
		
	// Decrement pointer for next new data
	// If underflow occurs, wrap around to other side of buffer 
	if (--newest < 0)
		newest = BUFSIZE - 1;
}

// Optimised verison of circ_FIR1 (normal circular)
void circ_FIR5(void)
{
	unsigned int i;
	
	// Pointer used for bounds check
	short *x_limit = x + BUFSIZE;
	
	short *x_ptr = x + newest;
	double *b_ptr = b;
	
	// Value kept in regster to decrease access times
	// Post-increment the pointers to put them at the correct position for new data
	// Accumulator (temp) is initialised with result from first iteration
	// instead of 0, to save cycles from unnecessary operation
	register double temp =  sample * *b_ptr++;
	*x_ptr++ = sample; 
	
	// Iterate over data and filter buffers performing MAC
	for (i = 1; i < BUFSIZE; i++)
	{
		// Loop pointer to beginning of array if there is overflow
		if (x_ptr >= x_limit)
			x_ptr -= BUFSIZE;
			
		 temp += (*x_ptr++ * *b_ptr++);
	}
	y = temp;
	
	// Decrement pointer for next new data
	// If underflow occurs, wrap around to other side of buffer 
	if (--newest < 0)
		newest = BUFSIZE - 1;
}

//Optimised version of 4 (Symmetrical circular)
/*void circ_FIR6(void)
{
	/*int i, j, k;
	y = 0;
	x[newest] = sample;
	
		
	unsigned int i;
	short *j;
	short *k;
	short *x_limit = x + BUFSIZE;
	short *x_ptr = x + newest;// + newest;
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
}*/

/*
//Optimisation of 2 (Split loop)
void circ_FIR7(void)
{
	unsigned int i;
	short *x_ptr = x + newest;// + newest;
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
*/
/*
// Non pointer optimised of 4 (symmetrical circular)
void circ_FIR8_odd(void)
{
	unsigned int i, j, k;
	y = 0;
	x[newest] = sample; 
	
 	for (i = 0, j = newest, k = newest - 1; j != k; i++, k--, j++)
	{
		if(k < 0)
			k += BUFSIZE;
			
		if (j >= BUFSIZE)
			j -= BUFSIZE;
			
		y += ((x[j] + x[k]) * b[i]);
		
	}
	
	// If odd number of coeffs, centre value will be duplicated, so subtract it once
	y += x[BUFSIZE/2]* b[i];
	
	// Wrap around to other side of buffer
	if (--newest < 0)
		newest = BUFSIZE - 1;
}
// Non pointer optimised of 4 (symmetrical circular)
void circ_FIR8_even(void)
{
	unsigned int i, j, k;
	y = 0;
	x[newest] = sample;
	
	
 	for (i = 0, j = newest, k = newest - 1; i < BUFSIZE/2; i++, k--, j++)
	{
		if(k < 0)
			k += BUFSIZE;
			
		if (j >= BUFSIZE)
			j -= BUFSIZE;
			
		y += ((x[j] + x[k]) * b[i]);
	}
	
	// Wrap around to other side of buffer
	if (--newest < 0)
		newest = BUFSIZE - 1;
} 
*/

// Combination of circ_FIR2 and circ_FIR4 (split-loop and symmetrical circular)
void circ_FIR9_even(void)
{
	unsigned int i, j, k; 
	y = 0;
	x[newest] = sample;
	
	// If newest data is in the second half of the buffer, then j will overflow
	if(newest > BUFSIZE/2)
	{
		// j will overflow at BUFSIZE - newest
		for(i = 0, j = newest, k = newest - 1; i < BUFSIZE - newest; i++, k--, j++)
		 	y += ((x[j] + x[k]) * b[i]);
		
		// When j overflows, set it to the start of the array
		for(j = 0; i < BUFSIZE/2; i++, k--, j++)
			y += ((x[j] + x[k]) * b[i]);
	}
	
	// If newest data is in the first half of the buffer, then k will underflow
	else
	{
		// k will underflow when it decrements past 0
		for(i = 0, j = newest, k = newest - 1; i < newest; i++, k--, j++)
			y += ((x[j] + x[k]) * b[i]);
			
		// When k underflows, set it to the end of the array 
		for(k = BUFSIZE - 1; i < BUFSIZE/2; i++, k--, j++)
			y += ((x[j] + x[k]) * b[i]);
		
	}
	
	// Decrement pointer for next new data
	// If underflow occurs, wrap around to other side of buffer 
	if (--newest < 0)
		newest = BUFSIZE - 1;

		
}

// Combination of circ_FIR2 and circ_FIR4 (split-loop and symmetrical circular)
void circ_FIR9_odd(void)
{
	unsigned int i, j, k; 
	y = 0;
	x[newest] = sample;
	
	// If newest data is in the second half of the buffer, then j will overflow
	if(newest > BUFSIZE/2)
	{
		// j will overflow at BUFSIZE - newest
		for(i = 0, j = newest, k = newest - 1; i < BUFSIZE - newest; i++, k--, j++)
		 	y += ((x[j] + x[k]) * b[i]);
				
		// When j overflows, set it to the start of the array
		for(j = 0; i < BUFSIZE/2; i++, k--, j++)
			y += ((x[j] + x[k]) * b[i]);			
	}
	
	
	// If newest data is in the first half of the buffer, then k will underflow
	else
	{
		// k will underflow when it decrements past 0
		for(i = 0, j = newest, k = newest - 1; i < newest; i++, j++, k--)
			y += ((x[j] + x[k]) * b[i]);
				
		// When k underflows, set it to the end of the array 
		for(k = BUFSIZE - 1; i < BUFSIZE/2; i++, k--, j++)
			y += ((x[j] + x[k]) * b[i]);
	}
	
	// If odd number of coeffs, centre value will be missed, so add it once
	y+= (x[j] * b[BUFSIZE/2]);
	
	// Decrement pointer for next new data
	// If underflow occurs, wrap around to other side of buffer 
	if (--newest < 0)
		newest = BUFSIZE - 1;

}		

// Combination of circ_FIR3 and circ_FIR4 (double buffer and symmetrical circular)
void circ_FIR10_odd(void)
{
	unsigned int i, j, k; 
	y = 0;
	
	// Read newest sample into buffer at newest index and offset index
	x2[newest] = sample;   
	x2[newest + BUFSIZE] = x[newest];

	// Since any stretch of BUFSIZE elements will contain all the data values,
	// we can perform the convolution in one pass and not have to worry
	// about accounting for index overflow
	
	// Filter buffer is symmertical, so only need to use the first half of the values
	// and condense convolution into 
	// b[0]*(x2[0]+x2[BUFSIZE/2-1]) + b[1]*(x2[1]+x2[BUFSIZE/2-2]) + ...
	for(i = 0, j = newest, k = BUFSIZE + newest - 1; i < BUFSIZE/2; i++, j++, k--)
	 	y += ((x2[j] + x2[k]) * b[i]);
		
	// If odd number of coeffs, centre value will be missed, so add it once
	y+= (x2[j] * b[BUFSIZE/2]);
	
	// Decrement pointer for next new data
	// If underflow occurs, wrap around to other side of buffer 
	if (--newest < 0)
		newest = BUFSIZE - 1;
}

void circ_FIR10_even(void)
{
	unsigned int i, j, k; 
	y = 0;
	
	// Read newest sample into buffer at newest index and offset index
	x2[newest] = sample;   
	x2[newest + BUFSIZE] = x[newest];

	// Since any stretch of BUFSIZE elements will contain all the data values,
	// we can perform the convolution in one pass and not have to worry
	// about accounting for index overflow
	
	// Filter buffer is symmertical, so only need to use the first half of the values
	// and condense convolution into 
	// b[0]*(x2[0]+x2[BUFSIZE/2-1]) + b[1]*(x2[1]+x2[BUFSIZE/2-2]) + ...
	for(i = 0, j = newest, k = BUFSIZE + newest - 1; i < BUFSIZE/2; i++, k--, j++)
	 	y += ((x2[j] + x2[k]) * b[i]);
	
	// Decrement pointer for next new data
	// If underflow occurs, wrap around to other side of buffer 
	if (--newest < 0)
		newest = BUFSIZE - 1;
}
