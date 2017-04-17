/*            - IORD2.H -

   Special header for the T89C51RD2 processors.
       
   Version 1.00 [FLOS]

*/

#pragma language=extended

/*===============================*/
/* Predefined SFR Byte Addresses */
/*===============================*/

sfr   P0        = 0x80;
sfr   P1        = 0x90;
sfr   P2        = 0xA0;
sfr   P3        = 0xB0;
sfr   P4        = 0xC0;
sfr   P5        = 0xE8;
sfr   PSW       = 0xD0;
sfr   ACC       = 0xE0;
sfr   B         = 0xF0;
sfr   SP        = 0x81;
sfr   DPL       = 0x82;
sfr   DPH       = 0x83;
sfr   PCON      = 0x87;
sfr   TCON      = 0x88;
sfr   TMOD      = 0x89;
sfr   TL0       = 0x8A;
sfr   TL1       = 0x8B;
sfr   TH0       = 0x8C;
sfr   TH1       = 0x8D;
sfr   IE        = 0xA8;
sfr   IP        = 0xB8;
sfr   SCON0     = 0x98;
sfr   SBUF0     = 0x99;
sfr   T2CON     = 0xC8;
sfr   T2MOD     = 0xC9;
sfr   RCAP2L    = 0xCA;
sfr   RCAP2H    = 0xCB;
sfr   TL2       = 0xCC;
sfr   TH2       = 0xCD;
sfr   IPL0      = 0xB8;
sfr   IPH0      = 0xB7;

sfr   AUXR      = 0x8E;
sfr   AUXR1     = 0xA2;
sfr   CKCON0    = 0x8F;
sfr   CKCON1    = 0xAF; 
sfr   WDTRST    = 0xA6;
sfr   WDTPRG    = 0xA7;
sfr   SADDR     = 0xA9;
sfr   IPH       = 0xB7;
sfr   SADEN     = 0xB9;
sfr   FCON      = 0xD1;
sfr   EECON     = 0xD2;
sfr   EETIM     = 0xD3;
sfr   CCON      = 0xD8;
sfr   CMOD      = 0xD9;
sfr   CCAPM0    = 0xDA;
sfr   CCAPM1    = 0xDB;
sfr   CCAPM2    = 0xDC;
sfr   CCAPM3    = 0xDD;
sfr   CCAPM4    = 0xDE;
sfr   CL        = 0xE9;
sfr   CCAP0L    = 0xEA;
sfr   CCAP1L    = 0xEB;
sfr   CCAP2L    = 0xEC;
sfr   CCAP3L    = 0xED;
sfr   CCAP4L    = 0xEE;
sfr   CH        = 0xF9;
sfr   CCAP0H    = 0xFA;
sfr   CCAP1H    = 0xFB;
sfr   CCAP2H    = 0xFC;
sfr   CCAP3H    = 0xFD;
sfr   CCAP4H    = 0xFE;

/*==============================*/
/* Predefined SFR Bit Addresses */
/*==============================*/
  
/*========PSW========*/

bit   CY        = 0xD7;
bit   AC        = 0xD6;
bit   F0        = 0xD5;
bit   RS1       = 0xD4;
bit   RS0       = 0xD3;
bit   OV        = 0xD2;
bit   P         = 0xD0;

/*========TCON=======*/

bit   TF1       = 0x8F;
bit   TR1       = 0x8E;
bit   TF0       = 0x8D;
bit   TR0       = 0x8C;
bit   IE1       = 0x8B;
bit   IT1       = 0x8A;
bit   IE0       = 0x89;
bit   IT0       = 0x88;

/*========IE=========*/

bit   EA        = 0xAF;
bit   EPCA      = 0xAE;
bit   ET2       = 0xAD;
bit   ES0       = 0xAC;
bit   ET1       = 0xAB;
bit   EX1       = 0xAA;
bit   ET0       = 0xA9;
bit   EX0       = 0xA8;

/*========IP=========*/

bit   PT2       = 0xBD;
bit   PS        = 0xBC;
bit   PT1       = 0xBB;
bit   PX1       = 0xBA;
bit   PT0       = 0xB9;
bit   PX0       = 0xB8;

/*========P3=========*/

bit   RD        = 0xB7;
bit   WR        = 0xB6;
bit   T1        = 0xB5;
bit   T0        = 0xB4;
bit   INT1      = 0xB3;
bit   INT0      = 0xB2;
bit   TXD       = 0xB1;
bit   RXD       = 0xB0;

/*========SCON========*/

bit   SM0       = 0x9F;
bit   SM1       = 0x9E;
bit   SM2       = 0x9D;
bit   REN0      = 0x9C;
bit   TB8       = 0x9B;
bit   RB8       = 0x9A;
bit   TI0       = 0x99;
bit   RI0       = 0x98;

/*========T2CON=======*/

bit   TF2       = 0xCF;
bit   EXF2      = 0xCE;
bit   RCLK      = 0xCD;
bit   TCLK      = 0xCC;
bit   EXEN2     = 0xCB;
bit   TR2       = 0xCA;
bit   C_T2      = 0xC9;
bit   CP_RL2    = 0xC8;


/*========CCON=========*/

bit   CF        = 0xDF;
bit   CR        = 0xDE;

bit   CCF4      = 0xDC;
bit   CCF3      = 0xDB;
bit   CCF2      = 0xDA;
bit   CCF1      = 0xD9;
bit   CCF0      = 0xD8;
/*==============================*/
/* Interrupt Vector Definitions */
/*==============================*/

interrupt [0x03] void EX0_int (void);       /* External Interrupt 0 */

interrupt [0x0B] void T0_int (void);        /* Timer 0 Overflow */

interrupt [0x13] void EX1_int (void);       /* External Interrupt 1 */

interrupt [0x1B] void T1_int (void);        /* Timer 1 Overflow */

interrupt [0x23] void SCON0_int (void);      /* Serial Port */

interrupt [0x2B] void T2_int (void);        /* Timer 2 Overflow */

interrupt [0x2B] void T2EX_int (void);      /* Negative Transition on T2EX */

interrupt [0x33] void PCA_int (void);       /* PCA Interrupt */
