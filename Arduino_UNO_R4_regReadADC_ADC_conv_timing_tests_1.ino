/*  Arduino UNO R4 test code to measure the RA4M1 chip die-temperature
 *  with register level ADC analog-read... 
 *  and ADC conversion timing measurments using AGT1
 *
 *  Susan Parker - 4th November 2023.
 *    First code - much pondering of datasheet!
 *
 *  Susan Parker - 5th November 2023.
 *    ... more head scratching, think I have the numbers in ball-park BUT no guarantees
 *
 *  Susan Parker - 11th November 2023.
 *    Add hijack of AGT0 for mS interrupts
 *    Add internal timing method for neasuring code block exercution times using AGT1 counter
 *
 *  Susan Parker - 18th November 2023.
 *    AGTx reload register data to AGTx counter happens automatically on underflow.
 *    Add InLine version to compare ADC_complete interrrupt overhead = c. 1.8uS
 *    Add ADC averaging over printing sample period e.g. every 100mS give average over 100 readings.
 *
 *  Warning: 
 *    There are a LOT of #defines in this code to select various options. 
 *    Not all combinations will work!
 *
 *  Note: ADC is operating at 14bits, the reference is given as a 12bit value
 *
 *  This code is "AS IS" without warranty or liability. 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

// ARM-developer - Accessing memory-mapped peripherals
// https://developer.arm.com/documentation/102618/0100

#define ICUBASE 0x40000000 // ICU Base - See 13.2.6 page 233
// 32 bits - 
#define IELSR 0x6300 // ICU Event Link Setting Register n
#define ICU_IELSR00 ((volatile unsigned int *)(ICUBASE + IELSR))            //
#define ICU_IELSR01 ((volatile unsigned int *)(ICUBASE + IELSR + ( 1 * 4))) // 
#define ICU_IELSR02 ((volatile unsigned int *)(ICUBASE + IELSR + ( 2 * 4))) // 
#define ICU_IELSR03 ((volatile unsigned int *)(ICUBASE + IELSR + ( 3 * 4))) // 
#define ICU_IELSR04 ((volatile unsigned int *)(ICUBASE + IELSR + ( 4 * 4))) // 
#define ICU_IELSR05 ((volatile unsigned int *)(ICUBASE + IELSR + ( 5 * 4))) // 
#define ICU_IELSR06 ((volatile unsigned int *)(ICUBASE + IELSR + ( 6 * 4))) // 
#define ICU_IELSR07 ((volatile unsigned int *)(ICUBASE + IELSR + ( 7 * 4))) // 
#define ICU_IELSR08 ((volatile unsigned int *)(ICUBASE + IELSR + ( 8 * 4))) // 
#define ICU_IELSR09 ((volatile unsigned int *)(ICUBASE + IELSR + ( 9 * 4))) // 
#define ICU_IELSR10 ((volatile unsigned int *)(ICUBASE + IELSR + (10 * 4))) // 
#define ICU_IELSR11 ((volatile unsigned int *)(ICUBASE + IELSR + (11 * 4))) // 
#define ICU_IELSR12 ((volatile unsigned int *)(ICUBASE + IELSR + (12 * 4))) // 
#define ICU_IELSR13 ((volatile unsigned int *)(ICUBASE + IELSR + (13 * 4))) // 
#define ICU_IELSR14 ((volatile unsigned int *)(ICUBASE + IELSR + (14 * 4))) // 
#define ICU_IELSR15 ((volatile unsigned int *)(ICUBASE + IELSR + (15 * 4))) // 
#define ICU_IELSR16 ((volatile unsigned int *)(ICUBASE + IELSR + (16 * 4))) // 
#define ICU_IELSR17 ((volatile unsigned int *)(ICUBASE + IELSR + (17 * 4))) // 
#define ICU_IELSR18 ((volatile unsigned int *)(ICUBASE + IELSR + (18 * 4))) // 
#define ICU_IELSR19 ((volatile unsigned int *)(ICUBASE + IELSR + (19 * 4))) // 
#define ICU_IELSR20 ((volatile unsigned int *)(ICUBASE + IELSR + (20 * 4))) // 
#define ICU_IELSR21 ((volatile unsigned int *)(ICUBASE + IELSR + (21 * 4))) // 
#define ICU_IELSR22 ((volatile unsigned int *)(ICUBASE + IELSR + (22 * 4))) // 
#define ICU_IELSR23 ((volatile unsigned int *)(ICUBASE + IELSR + (23 * 4))) // 
#define ICU_IELSR24 ((volatile unsigned int *)(ICUBASE + IELSR + (24 * 4))) // 
#define ICU_IELSR25 ((volatile unsigned int *)(ICUBASE + IELSR + (25 * 4))) // 
#define ICU_IELSR26 ((volatile unsigned int *)(ICUBASE + IELSR + (26 * 4))) // 
#define ICU_IELSR27 ((volatile unsigned int *)(ICUBASE + IELSR + (27 * 4))) // 
#define ICU_IELSR28 ((volatile unsigned int *)(ICUBASE + IELSR + (28 * 4))) // 
#define ICU_IELSR29 ((volatile unsigned int *)(ICUBASE + IELSR + (29 * 4))) // 
#define ICU_IELSR30 ((volatile unsigned int *)(ICUBASE + IELSR + (30 * 4))) // 
#define ICU_IELSR31 ((volatile unsigned int *)(ICUBASE + IELSR + (31 * 4))) // 

#define ICU_SELSR0  ((volatile unsigned short  *)(ICUBASE + 0x6200))         // SYS Event Link Setting Register

#define NVICBASE 0xE0000000 // NVIC Interrupt Controller
#define NVICIPR  0xE400     // Interrupt Priority Register
#define NVIC_IPR04_BY  ((volatile unsigned char  *)(NVICBASE + NVICIPR +  4 ))     // AGT for millis() etc = 0x80
#define NVIC_IPR05_BY  ((volatile unsigned char  *)(NVICBASE + NVICIPR +  5 ))     // 
#define NVIC_IPR06_BY  ((volatile unsigned char  *)(NVICBASE + NVICIPR +  6 ))     // 

// Low Power Mode Control - See datasheet section 10
#define SYSTEM 0x40010000 // System Registers
#define SYSTEM_SBYCR   ((volatile unsigned short *)(SYSTEM + 0xE00C))      // Standby Control Register
#define SYSTEM_MSTPCRA ((volatile unsigned int   *)(SYSTEM + 0xE01C))      // Module Stop Control Register A
#define SYSTEM_SCKDIVCR  ((volatile unsigned int *)(SYSTEM + 0xE020))  // System Clock Division Control Register

#define MSTP 0x40040000 // Module Registers
#define MSTP_MSTPCRB   ((volatile unsigned int   *)(MSTP + 0x7000))      // Module Stop Control Register B
#define MSTPB2   2 // CAN0
#define MSTPB8   8 // IIC1
#define MSTPB9   9 // IIC0
#define MSTPB18 18 // SPI1
#define MSTPB19 19 // SPI0
#define MSTPB22 22 // SCI9
#define MSTPB29 29 // SCI2
#define MSTPB30 30 // SCI1
#define MSTPB31 31 // SCI0

#define MSTP_MSTPCRC   ((volatile unsigned int   *)(MSTP + 0x7004))      // Module Stop Control Register C
#define MSTP_MSTPCRD   ((volatile unsigned int   *)(MSTP + 0x7008))      // Module Stop Control Register D
#define MSTPD2   2 // AGT1   - Asynchronous General Purpose Timer 1 Module
#define MSTPD3   3 // AGT0   - Asynchronous General Purpose Timer 0 Module
#define MSTPD5   5 // GPT320 and GPT321 General 32 bit PWM Timer Module
#define MSTPD6   6 // GPT162 to GPT167 General 16 bit PWM Timer Module
#define MSTPD14 14 // POEG   - Port Output Enable for GPT Module Stop
#define MSTPD16 16 // ADC140 - 14-Bit A/D Converter Module
#define MSTPD19 19 // DAC8   -  8-Bit D/A Converter Module
#define MSTPD20 20 // DAC12  - 12-Bit D/A Converter Module
#define MSTPD29 29 // ACMPLP - Low-Power Analog Comparator Module
#define MSTPD31 31 // OPAMP  - Operational Amplifier Module

// The Mode Control bits are read as 1, the write value should be 1.
// Bit value 0: Cancel the module-stop state 
// Bit value 1: Enter the module-stop state.


// =========== ADC14 ============
// 35.2 Register Descriptions
#define ADCBASE 0x40050000 /* ADC Base */

#define ADC140_ADCSR   ((volatile unsigned short *)(ADCBASE + 0xC000)) // A/D Control Register
#define ADC140_ADANSA0 ((volatile unsigned short *)(ADCBASE + 0xC004)) // A/D Channel Select Register A0
#define ADC140_ADANSA1 ((volatile unsigned short *)(ADCBASE + 0xC006)) // A/D Channel Select Register A1
#define ADC140_ADADS0  ((volatile unsigned short *)(ADCBASE + 0xC008)) // A/D-Converted Value Addition/Average Channel Select Register 0
#define ADC140_ADADS1  ((volatile unsigned short *)(ADCBASE + 0xC00A)) // A/D-Converted Value Addition/Average Channel Select Register 1
#define ADC140_ADCER   ((volatile unsigned short *)(ADCBASE + 0xC00E)) // A/D Control Extended Register 
#define ADC140_ADSTRGR ((volatile unsigned short *)(ADCBASE + 0xC010)) // A/D Conversion Start Trigger Select Register
#define ADC140_ADEXICR ((volatile unsigned short *)(ADCBASE + 0xC012)) // A/D Conversion Extended Input Control Register
#define ADC140_ADANSB0 ((volatile unsigned short *)(ADCBASE + 0xC014)) // A/D Channel Select Register B0
#define ADC140_ADANSB1 ((volatile unsigned short *)(ADCBASE + 0xC016)) // A/D Channel Select Register B1
#define ADC140_ADTSDR  ((volatile unsigned short *)(ADCBASE + 0xC01A)) // A/D conversion result of temperature sensor output
#define ADC140_ADOCDR  ((volatile unsigned short *)(ADCBASE + 0xC01C)) // A/D result of internal reference voltage
#define ADC140_ADRD    ((volatile unsigned short *)(ADCBASE + 0xC01E)) // A/D Self-Diagnosis Data Register

#define ADC140_ADDR00 ((volatile unsigned short *)(ADCBASE + 0xC020))      // A1 (P000 AN00 AMP+)
#define ADC140_ADDR01 ((volatile unsigned short *)(ADCBASE + 0xC020 +  2)) // A2 (P001 AN01 AMP-) 
#define ADC140_ADDR02 ((volatile unsigned short *)(ADCBASE + 0xC020 +  4)) // A3 (P002 AN02 AMPO) 
#define ADC140_ADDR05 ((volatile unsigned short *)(ADCBASE + 0xC020 + 10)) // Aref (P010 AN05 VrefH0)
#define ADC140_ADDR09 ((volatile unsigned short *)(ADCBASE + 0xC020 + 18)) // A0 (P014 AN09 DAC)
#define ADC140_ADDR21 ((volatile unsigned short *)(ADCBASE + 0xC040 + 10)) // A4 (P101 AN21 SDA) 
#define ADC140_ADDR22 ((volatile unsigned short *)(ADCBASE + 0xC040 + 12)) // A5 (P100 AN20 SCL) 
#define ADC140_ADTSDR ((volatile unsigned short *)(ADCBASE + 0xC01A))      // Internal Temperature Sensor
#define ADC140_ADOCDR ((volatile unsigned short *)(ADCBASE + 0xC01C))      // Internal Voltage Reference

#define ADC140_ADHVREFCNT ((volatile unsigned char  *)(ADCBASE + 0xC08A)) // A/D High-Potential/Low-Potential Reference Voltage Control Register
#define ADC140_ADADC      ((volatile unsigned char  *)(ADCBASE + 0xC00C)) // A/D-Converted Value Addition/Average Count Select Register

#define ADC140_ADSSTR00 ((volatile unsigned char *)(ADCBASE + 0xC0E0))      // AN00 A/D Sampling State Register
#define ADC140_ADSSTRT  ((volatile unsigned char *)(ADCBASE + 0xC0DE))      // AN00 A/D Sampling State Register

// Temperature Sensor
//   The sensor outputs a voltage directly proportional to the die temperature
//   The relationship between the die temperature and the output voltage is (aproximatly) linear.
//   Note: The temperature scale is inverted i.e. colder is a bigger number
#define TSN_TSCDRH   ((volatile unsigned char *)(0x407EC229))             // Temperature Sensor Calibration Data Register H
#define TSN_TSCDRL   ((volatile unsigned char *)(0x407EC228))             // Temperature Sensor Calibration Data Register L

// 12-Bit D/A Converter
#define DACBASE 0x40050000          // DAC Base - DAC output on A0 (P014 AN09 DAC)
#define DAC12_DADR0    ((volatile unsigned short *)(DACBASE + 0xE000))      // D/A Data Register 0 
#define DAC12_DACR     ((volatile unsigned char  *)(DACBASE + 0xE004))      // D/A Control Register
#define DAC12_DADPR    ((volatile unsigned char  *)(DACBASE + 0xE005))      // DADR0 Format Select Register
#define DAC12_DAADSCR  ((volatile unsigned char  *)(DACBASE + 0xE006))      // D/A A/D Synchronous Start Control Register
#define DAC12_DAVREFCR ((volatile unsigned char  *)(DACBASE + 0xE007))      // D/A VREF Control Register


// =========== Ports ============
// 19.2.5 Port mn Pin Function Select Register (PmnPFS/PmnPFS_HA/PmnPFS_BY) (m = 0 to 9; n = 00 to 15)
#define PORTBASE 0x40040000 /* Port Base */

#define P000PFS 0x0800  // Port 0 Pin Function Select Register
#define PFS_P000PFS ((volatile unsigned int *)(PORTBASE + P000PFS))            // 
#define PFS_P001PFS ((volatile unsigned int *)(PORTBASE + P000PFS + ( 1 * 4))) // 
#define PFS_P002PFS ((volatile unsigned int *)(PORTBASE + P000PFS + ( 2 * 4))) // 
#define PFS_P003PFS ((volatile unsigned int *)(PORTBASE + P000PFS + ( 3 * 4))) // 
#define PFS_P004PFS ((volatile unsigned int *)(PORTBASE + P000PFS + ( 4 * 4))) // 
#define PFS_P005PFS ((volatile unsigned int *)(PORTBASE + P000PFS + ( 5 * 4))) // 
#define PFS_P006PFS ((volatile unsigned int *)(PORTBASE + P000PFS + ( 6 * 4))) // 
#define PFS_P007PFS ((volatile unsigned int *)(PORTBASE + P000PFS + ( 7 * 4))) // 
#define PFS_P008PFS ((volatile unsigned int *)(PORTBASE + P000PFS + ( 8 * 4))) // 
// #define PFS_P009PFS ((volatile unsigned int *)(PORTBASE + P000PFS + ( 9 * 4))) // Does not exist
#define PFS_P010PFS ((volatile unsigned int *)(PORTBASE + P000PFS + (10 * 4))) // 
#define PFS_P011PFS ((volatile unsigned int *)(PORTBASE + P000PFS + (11 * 4))) // 
#define PFS_P012PFS ((volatile unsigned int *)(PORTBASE + P000PFS + (12 * 4))) // 
#define PFS_P013PFS ((volatile unsigned int *)(PORTBASE + P000PFS + (13 * 4))) // N/C
#define PFS_P014PFS ((volatile unsigned int *)(PORTBASE + P000PFS + (14 * 4))) // N/A
#define PFS_P015PFS ((volatile unsigned int *)(PORTBASE + P000PFS + (15 * 4))) // N/A

#define PFS_P100PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843))   // 8 bits - A5
#define PFS_P101PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + ( 1 * 4))) // A4
#define PFS_P102PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + ( 2 * 4))) // D5
#define PFS_P103PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + ( 3 * 4))) // D4
#define PFS_P104PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + ( 4 * 4))) // D3
#define PFS_P105PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + ( 5 * 4))) // D2
#define PFS_P106PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + ( 6 * 4))) // D6
#define PFS_P107PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + ( 7 * 4))) // D7
#define PFS_P108PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + ( 8 * 4))) // SWDIO
#define PFS_P109PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + ( 9 * 4))) // D11 / MOSI
#define PFS_P110PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + (10 * 4))) // D12 / MISO
#define PFS_P111PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + (11 * 4))) // D13 / SCLK
#define PFS_P112PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x0843 + (12 * 4))) // D10 / CS
#define PFS_P300PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x08C3))            // SWCLK (P300)
#define PFS_P301PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x08C3 + (01 * 4))) // D0 / RxD (P301)
#define PFS_P302PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x08C3 + (02 * 4))) // D1 / TxD (P302) 
#define PFS_P303PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x08C3 + (03 * 4))) // D9
#define PFS_P304PFS_BY ((volatile unsigned char  *)(PORTBASE + 0x08C3 + (04 * 4))) // D8


// ====  Asynchronous General Purpose Timer (AGT) =====
#define AGTBASE 0x40084000
#define AGT0_AGT    ((volatile unsigned short *)(AGTBASE))         // AGT Counter Register
#define AGT1_AGT    ((volatile unsigned short *)(AGTBASE + 0x100))
#define AGT0_AGTCMA ((volatile unsigned short *)(AGTBASE + 0x002)) // AGT Compare Match A Register
#define AGT1_AGTCMA ((volatile unsigned short *)(AGTBASE + 0x102))
#define AGT0_AGTCMB ((volatile unsigned short *)(AGTBASE + 0x004)) // AGT Compare Match B Register
#define AGT1_AGTCMB ((volatile unsigned short *)(AGTBASE + 0x104))

// 8 bit registers
#define AGT0_AGTCR    ((volatile unsigned char  *)(AGTBASE + 0x008))  // AGT Control Register
#define AGT1_AGTCR    ((volatile unsigned char  *)(AGTBASE + 0x108))  //
#define AGTCR_TSTART 0  // R/W - AGT Count Start; 1: Count starts, 0: Count stops   
#define AGTCR_TCSTF  1  // R   - AGT Count Status Flag; 1: Count in progress, 0: Count is stopped   
#define AGTCR_TSTOP  2  // W   - AGT Count Forced Stop; 1: The count is forcibly stopped, 0: Writing 0 is invalid!!!   
#define AGT0_AGTMR1   ((volatile unsigned char  *)(AGTBASE + 0x009))  // AGT Mode Register 1
#define AGT1_AGTMR1   ((volatile unsigned char  *)(AGTBASE + 0x109))  //
#define AGT0_AGTMR2   ((volatile unsigned char  *)(AGTBASE + 0x00A))  // AGT Mode Register 2
#define AGT1_AGTMR2   ((volatile unsigned char  *)(AGTBASE + 0x10A))  //
#define AGT0_AGTIOC   ((volatile unsigned char  *)(AGTBASE + 0x00C))  // AGT I/O Control Register
#define AGT1_AGTIOC   ((volatile unsigned char  *)(AGTBASE + 0x10C))  //
#define AGTIOC_TOE   2  // AGTOn Output Enable   
#define AGT0_AGTISR   ((volatile unsigned char  *)(AGTBASE + 0x00D))  // AGT Event Pin Select Register
#define AGT1_AGTISR   ((volatile unsigned char  *)(AGTBASE + 0x10D))  //
#define AGT0_AGTCMSR  ((volatile unsigned char  *)(AGTBASE + 0x00E))  // AGT Compare Match Function Select Register
#define AGT1_AGTCMSR  ((volatile unsigned char  *)(AGTBASE + 0x10E))  //
#define AGT0_AGTIOSEL ((volatile unsigned char  *)(AGTBASE + 0x00F))  // AGT Pin Select Register
#define AGT1_AGTIOSEL ((volatile unsigned char  *)(AGTBASE + 0x10F))  //



// === Local Defines

// #define ADC_EXT_AREF        // Use external ADC Aref source
#define ADC_14BIT
#define ADC_INLINE          // Do blocking ADC conversion - comment out to use ADC_complete interrupt

#define ADC_TEMP            // Enable Internal Die-Temperature reading, otherwise read board's A1 input
#ifdef ADC_TEMP
#define PRINT_TEMP          // Calculate actual temperature, else show ADC timings and value
#endif

#ifndef ADC_TEMP            // Note ADC_AVARAGE not available for ADC_TEMP mode
#define ADC_AVARAGE         // Enable ADC module averaging i.e. up to four sucessive conversions
#define ADC_AVARAGE_TIMES 0x03   // 4-time conversion (three additions)
// #define ADC_AVARAGE_TIMES 0x02  // 3-time conversion (two additions)
// #define ADC_AVARAGE_TIMES 0x01  // 2-time conversion (one addition)
// #define ADC_AVARAGE_TIMES 0x00  // 1-time conversion (no addition: same as normal conversion)
#endif 

#define ADSSTR              // Enable Sampling State Register change from default
                            // See Table 35.8 Note 2 for temperature conversion
#ifdef ADC_TEMP             // When the temperature sensor is converted, set the sampling time to more than 5 μs.
#define ADSSTRT_VAL 0xAF    // A/D Sampling State Register Temp; 0xAF = 5.1uS conversion time
#else
#define ADSSTR00_VAL 0x3F    // A/D Sampling State Register 0 - Default is 0x0D with standard IDE settings; 
#endif

#define ADC_LOOP_MAX  100

#ifdef ADC_EXT_AREF
#define ADC_REF_V     (float) 4.096  // Volts - External Aref fitted
#define ADC_24C_VAL    4150
#else
#define ADC_REF_V     (float) 4.731  // Volts - For this value measure Vcc on board
#define ADC_24C_VAL    3600
#endif

#define ADC_AVERAGE 32
#define ADC_RES_12            4096
#define ADC_RES_14           16383
#define TEMP_DEG_C_REF         125

uint16_t temp_cal125;
float temp_V1, temp_V2;
float temp_Vm;
float temp_slope;

bool tick_tock = false;

#define AGT0_RELOAD   2999      // AGT0 value to give 1mS / 1kHz - with 48.0MHz/2 PCLKB; refine as needed
// #define AGT0_RELOAD   5999     // AGT0 value to give  2mS / 500Hz 
// #define AGT0_RELOAD  29999     // AGT0 value to give 10mS / 100Hz 

volatile uint64_t agt_count;   // Max 64bit count is 18,446,744,073,709,551 seconds = 584,942,417 Years
uint16_t adc_val_16;

#define MS_DELAY_PRINT 1000     // Time xxx milliseconds between printing readings
#define AVERAGE_MS_DATA

#define PCLKB_nS 41.6666667
#define PCLKB_MHz 24

volatile uint16_t  adc_agt1_count;
volatile bool adc_data_ready = false;

void setup()
  {
  *ICU_IELSR04 = 0x014;                                 // Reasign mS timer Slot 04 IELSR04 to non-active IRQ e.g DMAC3

  attachInterrupt(15, agt0UnderflowInterrupt, FALLING); // This IRQ will be asigned to Slot 05 IELSR05 as 0x001 PORT_IRQ0 - Table 13.4
  *ICU_IELSR05 = 0x01E;                                 // HiJack Slot 05 IELSR05 for AGT0_AGTI
  *PFS_P000PFS = 0x00000000;                            // Clear A1/D15 ISEL pin assigned Interrupt Enable
	asm volatile("dsb");                                  // Data bus Synchronization instruction
  *NVIC_IPR05_BY = 0x40;                                // Bounce the priority up from 0x80 to 0x60
// Warning: Cannot use   delay(); etc mS functions.

#ifndef ADC_INLINE                                      // IRQ call is a 1.8uS overhead
  attachInterrupt(16, adcCompleteInterrupt, RISING);    // This IRQ will be asigned to Slot 06 IELSR06 as 0x002 PORT_IRQ1 - Table 13.4
  *PFS_P001PFS = 0x00000000;                            // Clear A2/D16 ISEL pin assigned Interrupt Enable
  *ICU_IELSR06 = 0x029;                                 // Assign Slot 06 IELSR06 for ADC140_ADI Interrupt
	asm volatile("dsb");                                  // Data bus Synchronization instruction
  *NVIC_IPR06_BY = 0x40;                                // Bounce the priority up from 0xC0 to 0x40
#endif

  *PFS_P107PFS_BY = 0x04;                               // Set D7 output low - IRQ time flag pin

  Serial.begin(115200);
  while (!Serial){};

  setup_adc();
  setup_agt1();

  *AGT0_AGT = AGT0_RELOAD;           //  Set value for mS counter - varies with different clock frequencies

#ifdef PRINT_SYSCLKS
//  SysClocks = 0x10010100 : 
//  [b15]; PCKA[2:0] = 1x; [b11]; PCKB[2:0] = 1/2x; [b7]; PCKC[2:0] = 1x; [b3]; PCKD[2:0] = 1x
//  [b31]; FCK[2:0] = 1/2x; [b27]; ICK[2:0] = 1x; [b23 to b19]; [b18 to b16] = PCKB[2:0]
  Serial.print("SysClocks = ");            // Check System Clock settings
  Serial.println(*SYSTEM_SCKDIVCR, HEX);
  Serial.print("AD Ctrl Reg = ");            // ADC settings
  Serial.println(*ADC140_ADCSR, HEX);
  Serial.print("AD Aver Reg = ");            // ADC settings
  Serial.println(*ADC140_ADADC, HEX);
#endif

#ifdef ADC_TEMP
  temp_cal125 = (*TSN_TSCDRH << 8) + *TSN_TSCDRH;    // My RA4M1 chip gives ADC value of 771 - yours will be different
  temp_V1 = (3.3 * temp_cal125) / ADC_RES_12;        // Example: 0.19276 V when temp_cal125 = 771 for 125°C
  temp_V2 = (ADC_REF_V * ADC_24C_VAL) / ADC_RES_14;  //         est at +24°C for Internal or ext V aref 
  temp_slope = (temp_V2 - temp_V1) / (24 - 125);     //   = (1.04 - 0.62) / (24 - 125) =  (0.42 / -101) = -0.0041158       
  temp_Vm = ADC_REF_V / ADC_RES_14;                  //  0.000287 V per 14bit digit, or 0.00025V with 4.096V Aref

  Serial.print("Temp Cal 12bit = ");
  Serial.println(temp_cal125);
  Serial.print("CAL T V1  = ");
  Serial.println(temp_V1);
  Serial.print("ADC uVm = ");
  Serial.println(temp_Vm * 1000000);
  Serial.print("CAL T V2  = ");
  Serial.println(temp_V2);
  Serial.print("Slope x 1000 = ");
  Serial.println(temp_slope * 1000);
#endif
  }


void loop()
  {
  static  int16_t mS_delay_count = MS_DELAY_PRINT; 
  static uint32_t analog_average_acc = 0;
  
  static bool     mS_flag = false;
  float analog_average_result;
  float agt1_delay_us = 0.00;

/*  // Alternative code for mS ticks 
  static uint32_t agt_count_last;     // Note use static variable, otherwise reset to 0 each loop()

  if(agt_count_last != agt_count)
    {
    *PFS_P103PFS_BY = 0x05;      // Pulse on D4 for scope 
    *PFS_P103PFS_BY = 0x04;      //  
    agt_count_last = agt_count; 
    mS_flag = true;
    } 
*/

  if(adc_data_ready == true)
    {
    adc_data_ready = false;
#ifdef AVERAGE_MS_DATA
    analog_average_acc += adc_val_16;    //  
#endif
    mS_flag = true;
    }

  if(mS_flag == true)
    {
    *PFS_P103PFS_BY = 0x05;      // Flag on D4 to measure code time
    mS_flag = false;

    if(--mS_delay_count <= 0)       // Only print value every xxx "milliseconds"
      {
      mS_delay_count = MS_DELAY_PRINT;

#ifndef PRINT_TEMP
      uint16_t agt1_t_counts = 0xFFFF - adc_agt1_count;
      agt1_delay_us = (float)agt1_t_counts / PCLKB_MHz;
      Serial.print(agt1_t_counts);    // Use this to see number of AGT counts taken
//      Serial.print(adc_agt1_count);   // Use this to see number of AGT counts remaining of the 65535 start value
      Serial.print("\t");
      Serial.print(agt1_delay_us);
#endif   

#ifdef AVERAGE_MS_DATA
      analog_average_result = (float)analog_average_acc / MS_DELAY_PRINT;  // 
#ifdef PRINT_TEMP
      float temp_volts = temp_Vm * analog_average_result;
      Serial.print(temp_volts);
#else
      Serial.print("\t");
      Serial.println(analog_average_result);   //  takes c. 870uS with ADC floating point average printing
#endif
      analog_average_acc = 0;
#else
      Serial.print("\t");
#ifdef PRINT_TEMP
      Serial.print(adc_val_16);              //  takes c. 612uS with Interger ADC reading
#else
      Serial.println(adc_val_16);              //  takes c. 612uS with Interger ADC reading
#endif
#endif

#ifdef PRINT_TEMP
      float temperature = (temp_volts - temp_V1 ) / temp_slope + TEMP_DEG_C_REF;
      Serial.print("\t");
      Serial.println(temperature);
#endif

      }
    *PFS_P103PFS_BY = 0x04;      //  
    }
  }

void agt0UnderflowInterrupt(void)    // Note: AGT0 counter updated from reload-reg on underflow
  {
  *AGT1_AGT = 0xFFFF;                // Reset to FFFF - down-counter 
  *ADC140_ADCSR |= (0x01 << 15);     // Next ADC conversion = write to register c. 300nS
  *PFS_P111PFS_BY = 0x05;            // D13 - After the above as I/O operations cross internal busses = jitter
  agt_count++;                       // 
#ifdef ADC_INLINE
  uint16_t conv_wait_count = 0;
  while(true)
    {
    uint16_t analog_reg_value = *ADC140_ADCSR;         // Register read takes 83nS
    if((analog_reg_value &= (0x01 << 15) ) == 0x0000)  // This if() takes 83nS
      break;
    if(conv_wait_count++ > ADC_LOOP_MAX)            // TimeOut counter - for safety
      break;
    }
  adc_agt1_count = *AGT1_AGT;
#ifdef ADC_TEMP
  adc_val_16 = *ADC140_ADTSDR;       //  
#else
  adc_val_16 = *ADC140_ADDR00;       //  
#endif
   adc_data_ready = true;
#endif
  *PFS_P111PFS_BY = 0x04;            //  
  }


#ifndef ADC_INLINE
void adcCompleteInterrupt(void)
  {
  adc_agt1_count = *AGT1_AGT;
  *PFS_P107PFS_BY = 0x05;            // D7 
#ifdef ADC_TEMP
  adc_val_16 = *ADC140_ADTSDR;       //  
#else
  adc_val_16 = *ADC140_ADDR00;       //  
#endif
   adc_data_ready = true;
  *PFS_P107PFS_BY = 0x04;            //  
  }
#endif


void setup_adc(void)
  {
  *MSTP_MSTPCRD &= ~(0x01 << MSTPD16);  // Enable ADC140 module
  *ADC140_ADCSR  = 0x0000;           // Make sure reg is at 0

#ifdef ADC_EXT_AREF
  *ADC140_ADHVREFCNT = 0x01;         // Set External Aref = analogReference(AR_EXTERNAL);      
#endif

#ifdef ADC_14BIT
  *ADC140_ADCER = 0x06;              // 14 bit mode, clear ACE bit 5
#else
  *ADC140_ADCER = 0x00;              // 12 bit mode, clear ACE bit 5
#endif

#ifdef ADC_TEMP
  *ADC140_ADANSA0 = 0x0000;          // set the ADANSA0 register to 0000h to deselect all analog input channels.
  *ADC140_ADEXICR = 0x0101;          // set TSSAD and TSSA bits
#else
  *ADC140_ADANSA0 |= (0x01 << 0);    // Selected ANSA00 = A1 as DAC is on A0

#ifdef ADC_AVARAGE                   // Don't enable ADC module averaging for Temp
  *ADC140_ADADC    = ADC_AVARAGE_TIMES;  
  *ADC140_ADADC   |= (0x1 << 7);     // Set b7 to enable averaging
  *ADC140_ADADS0  |= (0x01 << 0);    // Enable Averaging for ANSA00 channel
#endif

#endif

#ifdef ADSSTR
#ifdef ADC_TEMP
  *ADC140_ADSSTRT = ADSSTRT_VAL;
#else
  *ADC140_ADSSTR00 = ADSSTR00_VAL;   // A/D Sampling State Register 0 - Default is 0x0D
#endif
#endif
  }

/*  DAC not used at present
void setup_dac(void)       // Note make sure ADC is stopped before setup DAC
  {
  *MSTP_MSTPCRD &= ~(0x01 << MSTPD20);  // Enable DAC12 module
  *DAC12_DADPR    = 0x00;        // DADR0 Format Select Register - Set right-justified format
//  *DAC12_DAADSCR  = 0x80;        // D/A A/D Synchronous Start Control Register - Enable
  *DAC12_DAADSCR  = 0x00;        // D/A A/D Synchronous Start Control Register - Default
// 36.3.2 Notes on Using the Internal Reference Voltage as the Reference Voltage
  *DAC12_DAVREFCR = 0x00;        // D/A VREF Control Register - Write 0x00 first - see 36.2.5
  *DAC12_DADR0    = 0x0000;      // D/A Data Register 0 
   delayMicroseconds(10);        
  *DAC12_DAVREFCR = 0x01;        // D/A VREF Control Register - Select AVCC0/AVSS0 for Vref
//  *DAC12_DAVREFCR = 0x03;        // D/A VREF Control Register - Select Internal reference voltage/AVSS0
//  *DAC12_DAVREFCR = 0x06;        // D/A VREF Control Register - Select External Vref; set VREFH&L pins used for LEDs
  *DAC12_DACR     = 0x5F;        // D/A Control Register - 
   delayMicroseconds(5);         // 
  *DAC12_DADR0    = 0x0800;      // D/A Data Register 0 
  *PFS_P014PFS   = 0x00000000;   // Make sure all cleared
  *PFS_P014PFS  |= (0x1 << 15);  // Port Mode Control - Used as an analog pin
  }
*/

void setup_agt1(void)
  {
  *MSTP_MSTPCRD &= ~(0x01 << MSTPD2);  // Enable AGT1 module
  *AGT1_AGTCR = 0x01;
  }

