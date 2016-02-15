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

// math library (trig functions)
#include <math.h>

// Some functions to help with writing/reading the audio ports when using interrupts.
#include <helper_functions_ISR.h>

// PI defined here for use in your code 
#define PI 3.141592653589793

// Singegen lookup table size
#define SINE_TABLE_SIZE 256 

// Size of delay buffer
#define BUFSIZE 12
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
/* Sampling frequency in HZ. Must only be set to 8000, 16000, 24000
32000, 44100 (CD standard), 48000 or 96000  */ 
int sampling_freq = 8000;

//Iterator used to iterate through sine table
float iterator = 0;

// Holds the value of the current sample 
short sample = 0; 

/* Left and right audio channel gain values, calculated to be less than signed 32 bit
 maximum value. */
Int32 L_Gain = 2100000000;
Int32 R_Gain = 2100000000;

// Sinegen lookup table
float table[SINE_TABLE_SIZE];

/* Use this variable in your code to set the frequency of your sine wave 
   be carefull that you do not set it above the current nyquist frequency! */
float sine_freq = 1000.0; 

double x[BUFSIZE];
 /******************************* Function prototypes ********************************/
void init_hardware(void);     
void init_HWI(void);          
void ISR_AIC(void);        
float sinegen(void);
void sine_init(void);
/********************************** Main routine ************************************/
void main(){      

 
	// initialize board and the audio port
  init_hardware();
	
  /* initialize hardware interrupts */
  init_HWI();
  
  sine_init();
  	 		
  /* loop indefinitely, waiting for interrupts */  					
  while(1) 
 	{
 		// Calculate next sample
 		//sample = sinegen();
 		
     	/* Send a sample to the audio port if it is ready to transmit.
           Note: DSK6713_AIC23_write() returns false if the port if is not ready */

      /*  //  send to LEFT channel (poll until ready)
        while (!DSK6713_AIC23_write(H_Codec, ((Int32)(sample * L_Gain))))
        {}; 
        
		// send same sample to RIGHT channel (poll until ready)
        while (!DSK6713_AIC23_write(H_Codec, ((Int32)(sample * R_Gain))))
        {};
        */
        
        
		// Set the sampling frequency. This function updates the frequency only if it 
		// has changed. Frequency set must be one of the supported sampling freq.
		//set_samp_freq(&sampling_freq, Config, &H_Codec);	
	}
  
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
	IRQ_map(IRQ_EVT_XINT1,4);		// Maps an event to a physical interrupt
	IRQ_enable(IRQ_EVT_XINT1);		// Enables the event
	IRQ_globalEnable();				// Globally enables interrupts
} 

/******************** WRITE YOUR INTERRUPT SERVICE ROUTINE HERE***********************/  

/*
 * Rectifies and outputs the sinewave
 */
 
void ISR_AIC(void)
{	
	
/*
 * Since we have to write a 16 bit value, we cast to `short`, since there is no half 
 * precision floating point number type in C. However, since our sample will be between 
 * -1 and 1, casting to a short would mean we would only ever get the values 0, 1 and
 * -1. So, we scale the value up by  2^15-1 (since 2^15 would cause overflow and 
 * incorrect output) before casting to ensure we don't lose information in the cast.
*/

	int i;
	for (i = BUFSIZE; i > 0; i--)
		x[i] = x[i-1];
	x[0] = mono_read_16Bit();
		
	mono_write_16Bit(sample);
	
	
}
/********************************** sinegen() ***************************************/   

/* 
 * Controls output frequency by controlling rate at which samples are chosen from 
 * lookup table.
 * Iterating through the lookup table with larger step size forms higher frequency
 * output and vice versa. 
 */
 
float sinegen(void)
{
	
	float step_size;
	step_size = (sine_freq/(float)sampling_freq)*SINE_TABLE_SIZE;
	
	iterator += step_size;
	
	// Wrapping around the sine table when the iterator exceeds its bounds
    if(iterator >= (SINE_TABLE_SIZE))
    {
       	iterator -= SINE_TABLE_SIZE;   
    }
	
	// Typecast to int as array index needs to be integer type
	return table[(int)iterator];  
} 

/********************************** sine_init() ***************************************/ 

/* 
 * Initialises the lookup table with samples of a sine wave
 */
 
void sine_init(void)
{
	int i;
	for(i=0; i < SINE_TABLE_SIZE; i++)
	{
		table[i] = sin(((float)i/SINE_TABLE_SIZE)*2*PI);
	}
}
  
