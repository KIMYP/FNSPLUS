        /**********************************************************
        /           410 series flow computer header file          /
        /           Update : 1999/10/11                           /
        **********************************************************/

#include ".\iord2.h"
#include "c:\icc8051\include\stdio.h"
#include "c:\icc8051\include\stdlib.h"
#include "c:\icc8051\include\string.h"
#include "c:\icc8051\include\math.h"

#define     RTC                 (   ( char * ) 0x014000 )
#define     NVRAM               (   ( char * ) 0x01400e )
#define		NVCHAR				(   ( char * ) 0x018000 )
#define		NVLONG				(   ( long * ) 0x018200 )

/**********************************************************
*           Flowmeter Signal Selection                    *
**********************************************************/

#define     Meter               PULSE_FLOW
#define     PULSE_FLOW          0
#define     ANALOG_FLOW         1

#define     DisMode             LCD_DISPLAY
#define     LCD_DISPLAY         0
#define     VFD_DISPLAY         1

#define     Compensation        NONE
#define     NONE                0
#define     LIQUID              1

#define     TempType            MA
#define     MA                  0
#define     RTD                 1

/**********************************************************
*           Variables in the Nonvolatile Memory           *
**********************************************************/

#define     Temp_unit           NVRAM[0]    /* 0: Celsius, 1: Fahrenheit */
#define     Pre_unit            NVRAM[1]    /* 0: kPa, 1: bar, 2: MPa */
#define     Press_type          NVRAM[2]    /* 0: absolute, 1: guage */
#define     Inter_use           NVRAM[3]    /* 0: disable, 1: enable */
#define     Total_unit          NVRAM[4]    /* overall total unit */
#define     Signal              NVRAM[5]    /* comm.signal type */
#define     Baud                NVRAM[6]    /* comm. baud_rate */
#define     CommType            NVRAM[7]    /* logging type-0:pc, 1:printer */
#define     PrintMode           NVRAM[8]    /* 0: key, 1: time interval */
#define     Print_unit          NVRAM[9]    /* 0: none, 1: print its unit */
#define     DateFormat          NVRAM[10]   /* date format, USA,EUP,KOR */
#define     ID_A                NVRAM[11]   /* Identification */
#define     ID_B                NVRAM[12]
#define     TimeBase            NVRAM[13]
#define     Total_DP            NVRAM[14]
#define     Rate_DP             NVRAM[15]
#define     AutoReturn          NVRAM[16]   /* 0:disable, 1:enable */
#define     Correct             NVRAM[17]
#define     Filter              NVRAM[18]   /* Digital Filtering constant 1-99 */
#define     Warning             NVRAM[19]   /* Warning Enable */
#define     VFD_mode            NVRAM[20]   /* brightness */
#define     CV_A                NVRAM[21]   /* Cutvalue */
#define     CV_B                NVRAM[22]
#define     Op_mode             NVRAM[23]
#define     Start_mode          NVRAM[24]
#define     Over_flag           NVRAM[25]
#define     Total_flag          NVRAM[26]
#define     Count_flag          NVRAM[27]

#define     Char                unsigned char
#define     Int                 unsigned int
#define     Long                unsigned long
#define     MINUS               0
#define     PLUS                1
/*
#define     Basic               ( ext_key_load() & 0x7f ) == 0x7f
#define     Basic_A             ( ext_key_load() & 0x7f ) == 0x3f
#define     Basic_R             ( ext_key_load() & 0x7f ) == 0x5f
#define     Basic_C             ( ext_key_load() & 0x7f ) == 0x4f
#define     Basic_AC            ( ext_key_load() & 0x7f ) == 0x2f
#define     Basic_RC            ( ext_key_load() & 0x7f ) == 0x1f

#define     Basic_NR            ( ext_key_load() & 0x2f ) == 0x2f
*/
#define     Basic               ( ext_key_load() & 0xf0 ) == 0xf0
#define     Basic_A             ( ext_key_load() & 0xf0 ) == 0xe0
#define     Basic_R             ( ext_key_load() & 0xf0 ) == 0xd0
#define     Basic_C             ( ext_key_load() & 0xf0 ) == 0xb0
#define     Basic_AC            ( ext_key_load() & 0xf0 ) == 0xa0
#define     Basic_RC            ( ext_key_load() & 0xf0 ) == 0x90
#define     Basic_AR            ( ext_key_load() & 0xf0 ) == 0xc0
#define     Basic_ARC           ( ext_key_load() & 0xf0 ) == 0x80

#define     Basic_NR            ( ext_key_load() & 0xa0 ) == 0xa0

#define     FLOW1               0x00    /* SEL2_OFF  SEL1_OFF  SEL0_OFF  */
#define     FLOW2               0x01    /* SEL2_OFF  SEL1_OFF  SEL0_ON  */
#define     TEMP1               0x02    /* SEL2_OFF  SEL1_ON   SEL0_OFF  */
#define     TEMP2               0x03    /* SEL2_OFF  SEL1_ON   SEL0_ON   */
#define     RTD1                0x04    /* SEL2_ON   SEL1_OFF  SEL0_OFF  */
#define     RTD2                0x05    /* SEL2_ON   SEL1_OFF  SEL0_ON   */
#define     PRESS1              0x06    /* SEL2_ON   SEL1_ON   SEL0_OFF  */
#define     PRESS2              0x07    /* SEL2_ON   SEL1_ON   SEL0_ON   */
#define     SEL_CLR             0x08

#define     MIN                 819
#define     MAX                 4094
#define     RANGE               ( MAX - MIN )

#define     PrestartTime        Const_C[0]  /* Relay 2 START Time */
#define     OutTime             Const_C[1]  /* Flow OUT Time      */
#define     EndTime             Const_C[2]  /* Batch END Time     */
#define     AutoStart           Const_C[3]  /* Batch Restart Time */

#define     ZeroAlarm           Option[0]   /* analog out zero */
#define     SpanAlarm           Option[1]   /* analog out span */
#define     BatchVolume         Option[2]   /* Set Batch Quantity */
#define     BatchLimit          Option[3]   /* Set Batch Limit */
#define     PrestartQ           Option[4]   /* Relay 2 Startp Quantity */
#define     PrestopQ            Option[5]   /* Relay 2 Stop Quantity */
#define     OverQ               Option[6]   /* Overrun Quantity */
#define     Factor1             Option[7]   /* K-Factor / Flow_Span */
#define     Factor2             Option[8]   /* Flow_Zero */



#define     EndPulse_H          EOUT = 0
#define     EndPulse_L          EOUT = 1
#define     OutPulse_H          POUT = 0
#define     OutPulse_L          POUT = 1

#define     RLY1_ON             RLY1 = 0
#define     RLY1_OFF            RLY1 = 1
#define     RLY2_ON             RLY2 = 0
#define     RLY2_OFF            RLY2 = 1

#define     Buzz_ON             BUZZ = 0
#define     Buzz_OFF            BUZZ = 1

#define     SEL0_ON             SEL0 = 1
#define     SEL0_OFF            SEL0 = 0
#define     SEL1_ON             SEL1 = 1
#define     SEL1_OFF            SEL1 = 0
#define     SEL2_ON             SEL2 = 1
#define     SEL2_OFF            SEL2 = 0

#define     ADC_ON              ADC = 1
#define     ADC_OFF             ADC = 0

/*

#define     EndPulse_H          WRPort_A = ( RDPort_A & 0xfe )
#define     EndPulse_L          WRPort_A = ( RDPort_A | 0x01 )
#define     ErrPulse_H          WRPort_A = ( RDPort_A & 0xfe )
#define     ErrPulse_L          WRPort_A = ( RDPort_A | 0x01 )
#define     OutPulse_H          WRPort_A = ( RDPort_A & 0xfd )
#define     OutPulse_L          WRPort_A = ( RDPort_A | 0x02 )

#define     RLY1_ON             WRPort_A = ( RDPort_A & 0xfb )
#define     RLY1_OFF            WRPort_A = ( RDPort_A | 0x04 )
#define     RLY2_ON             WRPort_A = ( RDPort_A & 0xf7 )
#define     RLY2_OFF            WRPort_A = ( RDPort_A | 0x08 )

#define     Buzz_ON             WRPort_A = ( RDPort_A & 0xef )
#define     Buzz_OFF            WRPort_A = ( RDPort_A | 0x10 )

#define     SEL0_ON             WRPort_B = ( RDPort_B | 0x01 )
#define     SEL0_OFF            WRPort_B = ( RDPort_B & 0xfe )
#define     SEL1_ON             WRPort_B = ( RDPort_B | 0x02 )
#define     SEL1_OFF            WRPort_B = ( RDPort_B & 0xfd )
#define     SEL2_ON             WRPort_B = ( RDPort_B | 0x04 )
#define     SEL2_OFF            WRPort_B = ( RDPort_B & 0xfb )

#define     ADC_ON              WRPort_B = ( RDPort_B | 0x80 )
#define     ADC_OFF             WRPort_B = ( RDPort_B & 0x7f )

*/
/**********************************************************
*           Description of Key in Front Display           *
**********************************************************/
#define     kMode               key_load() == 0xf0
#define     kShift              key_load() == 0xe8
#define     kUp                 key_load() == 0xd8
#define     kDown               key_load() == 0xb8
#define     kEnter              key_load() == 0x78
#define     kSetting            key_load() == 0xa8

#define     N_kMode             key_load() != 0xf0
#define     N_kShift            key_load() != 0xe8
#define     N_kUp               key_load() != 0xd8
#define     N_kDown             key_load() != 0xb8
#define     N_kEnter            key_load() != 0x78
#define     N_kSetting          key_load() != 0xa8

#define     Mode                0xf0
#define     Shift               0xe8
#define     Up                  0xd8
#define     Down                0xb8
#define     Enter               0x78
#define     Setting             0xa8


/**********************************************************
*           Description of Remote Key Operation           *
**********************************************************/

#define     R_Start             ext_key_load1() == 0x0e
#define     R_Stop              ext_key_load1() == 0x0d
#define     R_Interlock         ext_key_load2() == 0x00
#define     R_Reset             ext_key_load1() == 0x07

#define     R_IStart            ext_key_load1() == 0x0a
#define     R_IStop             ext_key_load1() == 0x09
#define     R_IReset            ext_key_load1() == 0x03

#define     NR_Start            ext_key_load1() != 0x0e
#define     NR_Stop             ext_key_load1() != 0x0d
#define     NR_Interlock        ext_key_load2() != 0x00
#define     NR_Reset            ext_key_load1() != 0x07

#define     NR_IStart           ext_key_load1() != 0x0a
#define     NR_IStop            ext_key_load1() != 0x09
#define     NR_IReset           ext_key_load1() != 0x03

#define     Start               0x0e
#define     Stop                0x0d
#define     Interlock           0x0b
#define     Reset               0x07

#define     IStart              0x0a
#define     IStop               0x09
#define     IReset              0x03
/**********************************************************
*           Description of RTC Address                    *
**********************************************************/

#define     rCENTURY            50
#define     rYEAR               9
#define     rMONTH              8
#define     rDAY                7
#define     rHOUR               4
#define     rMIN                2
#define     rSEC                0

/**********************************************************
*           Description of IO Port                        *
**********************************************************/
#define     CLK                 P5.0
#define     DI                  P5.1
#define     DO                  P1.7  /* switch4  imsi use */
#define     EEP                 P3.5
#define     DIS0                P5.3
#define     DIS1                P5.4
#define     SWC                 P5.5
#define     SWD                 P5.2
#define     TXE                 P1.0

#define     EOUT                P4.1
#define     POUT                P4.0
#define     RLY1                P1.4
#define     RLY2                P1.5
#define     BUZZ                P4.7
#define     ADC                 P5.6
#define     DAC                 P5.7
#define     SEL0                P4.2
#define     SEL1                P4.3
#define     SEL2                P4.4

#define     EXSWC               P4.5
#define     EXSWD               P4.6
#define     Pulse_W             MS10
#define     MS10                0
#define     MS100               1
#define     MS200               2

#pragma memory = no_init

Char        Const_C[4];
Long        Option[9];
Int         Zero;
Int         Span;
Int        Temp_4;
Int        Temp_20;

Long        GrossTotal;
Long        GrossAccTotal;
float       GrossRemain;
float       GrossAccRemain;

#pragma memory = xdata

Char        ID[2];
Char        FSound = 1;
Long        Total;

#if     Meter == PULSE_FLOW
float       Factor;             /* Meter Factor = 1 / K-Factor */
Int         Rate[100];

Int			intercheck = 0;

Int			NowTotal;



#elif   Meter == ANALOG_FLOW
Int         Cutvalue;
float       CutConst;
Long        Rate[100];

#endif

#pragma memory = idata

Char        DisTimer = 0;       /* Display timer for return */
Char        T = 0;
Char        AutoTimer = 0;
/*Char        DisCount;*/           /* 10mSec Interrupt Counter */
Char        DisEn = 0;          /* Display Enable Flag 260mSec */
Char        Dis[10];            /* Display Code Ascii Table */
Char        FilterCounter;      /* Rate Filter Counter */
Char        Timer = 0;
Char        DC ;
Char        tm = 0;
Long        RATE;               /* Rate Counter */
float       Rate_Fac;           /* Meter Factor * TimeBase */
float       RateHalf;
float       Total_Fac;

Int			Disc;

#if     Meter == ANALOG_FLOW

float       Input_Span;
float       Input_Zero;
#elif   Meter == PULSE_FLOW
/*Int         NowTotal;*/
Int         DisTotal = 0;
Int         TempTotal = 0;
Int         Offset = 0;

#endif

#pragma memory = data

char        Data;
Char        Pulse = 1;
Char        PulseFlag = 1;
Long        OldTotal;
Long        DisRate;
float       Analog;             /* analog output range */

#if     Meter == PULSE_FLOW
/*
Int         NowTotal;
Int         DisTotal = 0;
Int         TempTotal = 0;
Int         Offset = 0;
*/
#elif   Meter == ANALOG_FLOW

float       AnlTotal = 0;
float       Flowrate;
float       ANL;                /* analog input Value */
float       RateTotal = 0;      /* Rate Total Per 1sec */

#endif

#pragma memory = default

/**********************************************************
*                     Common   Function                   *
**********************************************************/

Int Ipow( Char p, Char dp )
{
    Char i;
    Int k = 1;

    for( i=0 ; i<dp ; i++ )                 k *= p;
    return k;
}

void delay( Int k )
{
    Int i;

    for( i=0 ; i<k ; i++ );

}

void beep(void)
{
    Pulse = 0;          Buzz_ON;            Pulse = 1;

    delay( 7500 );

    Pulse = 0;          Buzz_OFF;           Pulse = 1;
}

void bell(void)
{
    beep();                                 delay( 2500 );
    beep();
}

Char key_load(void)
{
    Char value = 0, i;

    SWC = 0;                                /* key load */
    CLK = 0;                                CLK = 1;
    SWC = 1;

    value = SWD;

    for( i=0 ; i<7 ; i++ )
    {
        value <<= 1;
        CLK = 0;                            CLK = 1;
        if( SWD )                           value |= 1;
    }

    CLK = 0;                                value &= 0xf8;

    return value;
}

Char ext_key_load(void)
{
    Char value = 0, i;

    EXSWC = 0;                                /* key load */
    EXSWC = 0;
	CLK = 0;
	CLK = 0;
	CLK = 1;
	CLK = 1;
    EXSWC = 1;
	EXSWC = 1;

    value = EXSWD;
	value = EXSWD;

    for( i=0 ; i<7 ; i++ )
    {
        value <<= 1;
        CLK = 0;                            
		CLK = 0;
        CLK = 1;                            
		CLK = 1;

        if( EXSWD )                         value |= 1;
    }

    CLK = 0;   
	CLK = 0;
	value &= 0xf0;

    return value;
}

Char ext_key_load1(void)
{
    Char value = 0, i;

    EXSWC = 0;                                /* key load */
    EXSWC = 0;
	CLK = 0;
	CLK = 0;
	CLK = 1;
	CLK = 1;
    EXSWC = 1;
	EXSWC = 1;

    value = EXSWD;
	value = EXSWD;	

    for( i=0 ; i<7 ; i++ )
    {
        value <<= 1;
        CLK = 0;                            
		CLK = 0;
        CLK = 1;                            
		CLK = 1;

        if( EXSWD )                         value |= 1;
    }

    CLK = 0;   
	CLK = 0;
	value &= 0x0b;

    return value;
}

Char ext_key_load2(void)
{
    Char value = 0, i;

    EXSWC = 0;                                /* key load */
    EXSWC = 0;
	CLK = 0;
	CLK = 0;
	CLK = 1;
	CLK = 1;
    EXSWC = 1;
	EXSWC = 1;

    value = EXSWD;
	value = EXSWD;	

    for( i=0 ; i<7 ; i++ )
    {
        value <<= 1;
        CLK = 0;                            
		CLK = 0;
        CLK = 1;                            
		CLK = 1;

        if( EXSWD )                         value |= 1;
    }

    CLK = 0;   
	CLK = 0;
	value &= 0x04;

    return value;
}

void key_ready( Char value )
{
    delay( 1500 );                          while( key_load() == value );
    delay( 2500 );
}

void remote_ready( Char value )
{
    delay( 1500 );                          
	while( ext_key_load1() == value );
    delay( 2500 );
}

Int ad_converter( Char ch )
{
#pragma memory = xdata
    Int result = 0;
    Char i;
#pragma memory = default

    if (ch == 0)       { SEL2 = 0;   SEL1 = 0;   SEL0 = 0;  }   /* FLOW1  SELECT */
	else if (ch == 1)  { SEL2 = 0;   SEL1 = 0;   SEL0 = 1;  }   /* FLOW2  SELECT */
	else if (ch == 2)  { SEL2 = 0;   SEL1 = 1;   SEL0 = 0;  }   /* TEMP1  SELECT */
    else if (ch == 3)  { SEL2 = 0;   SEL1 = 1;   SEL0 = 1;  }   /* TEMP2  SELECT */
    else if (ch == 4)  { SEL2 = 1;   SEL1 = 0;   SEL0 = 0;  }   /* RTD1   SELECT */
    else if (ch == 5)  { SEL2 = 1;   SEL1 = 0;   SEL0 = 1;  }   /* RTD2   SELECT */
    else if (ch == 6)  { SEL2 = 1;   SEL1 = 1;   SEL0 = 0;  }   /* PRESS1 SELECT */
    else if (ch == 7)  { SEL2 = 1;   SEL1 = 1;   SEL0 = 1;  }   /* PRESS2 SELECT */
	delay( 3000 );

    ADC_OFF;
    for( i=0; i<4 ; i++ )
    {
        CLK = 0;                            delay( 10 );
        if( i == 2 )                        DI = 0;
        else                                DI = 1;
        CLK = 1;                            delay( 10 );
    }
    CLK = 0;                                delay( 10 );
    CLK = 1;                                delay( 10 );

    for( i=0; i<12 ; i++ )
    {
        CLK = 0;                            delay( 10 );
        if( DO )                            result |= 1;
        CLK = 1;                            delay( 10 );
        if( i < 11 )                        result <<= 1;
        else                                ADC_ON;
    }
    return result;
}

        /***************************************************************
        *                        Analog Output                         *
        ***************************************************************/
void DA( Int value )
{
    Char i;

    DAC = 1;
    for( i=0 ; i<4 ; i++ )                  value <<= 1;
    for( i=0 ; i<12 ; i++ )
    {
        CLK = 0;

        if( value & 0x8000 )                DI = 1;
        else                                DI = 0;

        value <<= 1;                        CLK = 1;
    }
    DAC = 0;
    delay(10);
    DAC = 1;
}

void itoa_converter( long value, Char digit, Char dp  )
{
        /*----------------------------------------------*/
        /*       count from left, 1,2,3,4,5,6,7...      */
        /*  if dp > 0, then decimal point is exist      */
        /*           and dot position is at dp.         */
        /*  value -> int or long                        */
        /*  digit -> digit with number (0~9)            */
        /*----------------------------------------------*/
    Char    t, i, j=0, dis[10];
    long    a = 1;

    for( i=0 ; i<digit-1 ; i++ )            a *= 10;

    value %= a*10;

    for( i=0; i<10 ; i++ )                  dis[i] = ' ';
    if( dp )                                j = 1;

    for( i=0 ; i<digit+j ; i++ )
    {
        if( j && i == dp-1 )                dis[i] = '.';
        else
        {
            t = value / a;                  dis[i] = t+'0';
            value %= a;                     a /= 10;
        }
    }

    for( i=0; i<10 ; i++ )                  Dis[i] = dis[i];
}

