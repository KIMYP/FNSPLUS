/************************************************************************
*                                                                       *
*           Batch Controller BC410 & 420 Series                         *
*                                                                       *
*       Oscillator  : 18.4320MHz                                        *
*       CPU         : DALLAS 80C320                                     *
*       RTC         : DALLAS 12C887( Y2K Compatible )                   *
*       Display     : 410 - 20 Charactor * 2 line LCD with EL Backlight *
*                     420 - 20 Charactor * 2 line VFD                   *
*                                                                       *
*       Company     : Flos Korea Co.                                    *
*       Date        : 2003. 09. 22.                                     *
*       Version     : 1.33                                              *
*                                                                       *
*           Ver 1.30 : Only Inter-Lock Opreation is at remote option    *
*           Ver 1.31 : Preset & Batch Status Insert (yesman0 )          *
*           Ver 1.32 : RS485 Com Bug Change                             *
*                      Added Communication Function                     *
*           Ver 1.33 : For Analog input model (flow check error bug)lbi *
*           Ver 1.35 : Printing item added..                            *
*           Ver 1.50 : Run, Stop, EOB condition display                 *
*			Ver 1.55 : 단위통합(추가된단위 cc)							*
*           Ver 4.00 : Density B/D 프로그램 삽입                        *
************************************************************************/
#include ".\410B.h"

#if     DisMode == VFD_DISPLAY
#define     MODEL                   "       BC-420       "
#elif   DisMode == LCD_DISPLAY
#define     MODEL                   "      BC-410-R      "
#endif

#define     VERSION                 "     REV - 4.00     "

#pragma memory = no_init

Char        BatchStatus;
#if     Compensation == LIQUID
int         Value[6];
#endif
Char		ResetMethod;

#pragma memory = xdata

#if     Compensation == LIQUID
float       PRE1_RANGE;
float       TMP1_RANGE;
#endif

Char        r_err = 0;
Char        t_err = 0;
Char        f_err = 0;

Char        Check_Comm = 0;

#pragma memory = data

Char        error = 0;
Char        Change_Status = 0;

#pragma memory = code

#if     Compensation == LIQUID

    Char *Punit[3] = {  "kPa",      "bar",      "kg/cm2" };

    Char *Tunit[2] = {  "C",        "F"  };

#endif

#if     DisMode == VFD_DISPLAY

    Char *Totaldis[12] = { " ml", " ltr", " Ml", " in?, " ft?, " m?, " cc", " kg", " ton", " gal", " " }; /* 0xb3 179 */

#elif   DisMode == LCD_DISPLAY

    Char *Totaldis[12] = { " ml", " ltr", " Ml", " in", " ft", " m", " cc",  " kg",  " ton", " gal", " " };     /* 0x02 */

#endif

    Char *prnunit[12] = { "    ml","   ltr","    Ml","   in3","   ft3","    m3", "    cc","    kg","   ton", "   gal", "      " };

    Char *Rateunit[4] = {  "/s", "/m", "/h", "/d" };

#pragma memory = default

#include ".\lcd410.c"
#include ".\serial.c"


void set_ini( void )
{
#pragma memory = xdata
    float   constant = 0;
    char    i = 0;
#pragma memory = default

#if     Meter == PULSE_FLOW

    /**********************************************************
    *             Initial Parameter for Pulse Flow            *
    **********************************************************/
    if( !Factor1 )                          Factor1 = 10000;

    Factor = 1. / ( 0.0001 * Factor1 );

    Total_Fac = Ipow( 10, Total_DP ) * Factor;

    if( TimeBase < 3 )                      Rate_Fac = Ipow( 60, TimeBase );
    else                                    Rate_Fac = 24 * 3600;
    Rate_Fac *= Ipow( 10, Rate_DP ) * Factor;

#elif   Meter == ANALOG_FLOW

    /**********************************************************
    *             Initial Parameter for Analog Flow           *
    **********************************************************/
    Input_Zero = 0.0001 * Factor2;
    Input_Span = ( 0.0001 * Factor1 ) - Input_Zero;

    constant = Cutvalue * 0.0001;

    if( !Correct )
        CutConst = RANGE * constant + MIN;
    else
        CutConst = RANGE * constant * constant + MIN;

    if( TimeBase < 3 )                      Rate_Fac = Ipow( 60, TimeBase );
    else                                    Rate_Fac = 24 * 3600;

    Total_Fac = ( 1. / Rate_Fac ) * Ipow(10, Total_DP) / 40.;

    Rate_Fac = Ipow( 10, Rate_DP ) / 40.;

#endif

    RateHalf = 1. / ( Ipow( 10, Rate_DP ) * 2. );

    /**********************************************************
    *            Initial Parameter for Temperature            *
    **********************************************************/
#if     Compensation == LIQUID

#if     TempType == MA

    Temp_Zero = Temp_4 * 0.1;
    Temp_Span = ( Temp_20 * 0.1 ) - Temp_Zero;

#endif

#endif

    /**********************************************************
    *              Initial Analog Output Factor               *
    **********************************************************/
    Analog = Span - Zero;                   Analog /= SpanAlarm - ZeroAlarm;

    if( !Filter )                           Filter = 4;
    for( i=0 ; i<Filter ; i++ )             Rate[i] = 0;
}

void ResetValue( Char i )
{

#if     Meter == PULSE_FLOW
    NowTotal = 0;                           DisTotal = 0;
    Offset = 0;

#elif   Meter == ANALOG_FLOW
    AnlTotal = 0;

#endif

    if( i )         /* Total value Reset */
    {
        GrossAccTotal += GrossTotal;        GrossAccRemain += GrossRemain;
        Total = GrossAccRemain;             GrossAccRemain -= Total;
        GrossAccTotal += Total;             GrossAccTotal %= 10000000;
    }
    else            /* Accumulated Total value Reset */
    {
        GrossAccTotal = 0;                  GrossAccRemain = 0;
    }

    GrossTotal = 0;                         GrossRemain = 0;
    OldTotal = 0;
}
    
void Printing( void )
{

    if( Signal )
    {
       TXE = 1;
       if( Signal == 2 )       REN0 = 0;
    }

	/** controller environment printing *********************/
    SerialPuts( "\n\rUnit ID                             " );
    SerialPutc( ID[0] );                    SerialPutc( ID[1] );
    SerialPuts( "\n\rDate                        " );      
    SendDate( DateFormat );
    SerialPuts( "\n\rTime                          " );      
    SendTime();                           
     
    /************* Rate printing ****************/
    SerialPuts( "\n\n\rRate            " );
    if( Total_unit == 10 || !Print_unit )
        SerialPuts( "        " );
    else
    {
        SerialPuts( *(prnunit + Total_unit) );
        SerialPuts( *(Rateunit + TimeBase) );
    }

    SerialPuts( "      " );
    IntAscii( Rate_DP, DisRate, PLUS, 1, 7 );


	/*********** PRESET printing ****************/
    SerialPuts( "\n\rPRESET Qt.      " );

    if( Total_unit == 10 || !Print_unit )
        SerialPuts( "      " );
    else
        SerialPuts( *(prnunit + Total_unit) );
    SerialPuts( "         " );

    IntAscii( Total_DP, BatchVolume, PLUS, 1, 7 );
	
	/*********** Total printing ****************/
    SerialPuts( "\n\rTotal           " );

    if( Total_unit == 10 || !Print_unit )
        SerialPuts( "      " );
    else
        SerialPuts( *(prnunit + Total_unit) );
    SerialPuts( "         " );

    IntAscii( Total_DP, GrossTotal, PLUS, 1, 7 );

	/********** Acc Total Printing *************/
    SerialPuts( "\n\rAcc. Total      " );
    if( Total_unit == 10 || !Print_unit )
        SerialPuts( "      " );
    else
        SerialPuts( *(prnunit + Total_unit) );
    SerialPuts( "         " );
    IntAscii( Total_DP, GrossTotal + GrossAccTotal, PLUS, 1, 7 );

	/********** Controll code printing **********/
    SerialPuts( "\n\r\n\n\n\n\n\n\n" );
    SerialPutc( 0x1b );
    SerialPutc( 0x50 );
    SerialPutc( 0x01 );

    if( Signal )
    {
        TXE = 0;
        if( Signal == 2 )       REN0 = 1;
    }

}

void communication( void )
{
    Char k, a;
    long l;

    while( Buf.Head != Buf.Tail )
    {
		if( Buf.Buff[Buf.Tail] == '\r' )
        {
            Buffer[Pos++] = '\r';

            if( Buffer[0] == 'I' && Buffer[1] == ID[0] && Buffer[2] == ID[1] )
            {
                if( Signal )
                {
                    TXE = 1;
                    if( Signal == 2 )       REN0 = 0;
                }

                SerialPutc( ID[0] );        SerialPutc( ID[1] );
                SerialPutc( ' ' );
                     
                if( Buffer[3] == 'R' )
                {
                    if( Buffer[4] == '0' && Buffer[5] == '1' )
                        IntAscii( Total_DP, GrossTotal, PLUS, 1, 7 );
                    else if( Buffer[4] == '0' && Buffer[5] == '2' )
                        IntAscii( Total_DP, GrossTotal + GrossAccTotal, PLUS, 1, 7 );
                    else if( Buffer[4] == '0' && Buffer[5] == '3' )
                        IntAscii( Rate_DP, DisRate, PLUS, 1, 7 );
                    else if( Buffer[4] == '2' && Buffer[5] == '8' )
                        SendDate( 0 );
                    else if( Buffer[4] == '2' && Buffer[5] == '9' )
                        SendTime();
                    else if( Buffer[4] == '3' && Buffer[5] == '0' )
                        IntAscii( Total_DP, BatchVolume, PLUS, 1, 7 );
                    else if( Buffer[4] == '3' && Buffer[5] == '1' )
                        IntAscii( 0, BatchStatus, PLUS, 0, 2 );
                    else
                        SerialPuts( "Out Of Range" );
                    SerialPuts( "\r\n" );
                }
                else if( Buffer[3] == 'W' )
                {
                    if( Buffer[4] == '0' && Buffer[5] == '2'
                      && Buffer[6] == '0' && Buffer[7] == '\r' )
                    {
                        ResetValue( 1 );
                        ResetValue( 0 );
                        IntAscii( Total_DP, GrossTotal + GrossAccTotal, PLUS, 1, 7 );
                    }                       
                    else if( Buffer[4] == '2' && Buffer[5] == '8' )
                    {
                        if( Buffer[10] == '/' && Buffer[13] == '/' && Buffer[16] == '\r' )
                        { 
                            RTC[11] = 0x82;
                            PutRtc( rCENTURY, Buffer[6], Buffer[7] );
                            PutRtc( rYEAR, Buffer[8], Buffer[9] );
                            PutRtc( rMONTH, Buffer[11], Buffer[12] );
                            PutRtc( rDAY, Buffer[14], Buffer[15] );
                            RTC[11] = 0x02;
                            SendDate( 0 );
                        }
                        else                SerialPuts( "Invalid Format" );
                    }
                    else if( Buffer[4] == '2' && Buffer[5] == '9' )
                    {
                        if( Buffer[8] == ':' && Buffer[11] ==':' && Buffer[14] =='\r' )
                        { 
                            RTC[11] = 0x82;
                            PutRtc( rHOUR, Buffer[6], Buffer[7] );
                            PutRtc( rMIN, Buffer[9], Buffer[10] );
                            PutRtc( rSEC, Buffer[12], Buffer[13] );
                            RTC[11] = 0x02;
                            SendTime();                            
                        }
                        else                SerialPuts( "Invalid Format" );     
                    }   
                    else if( Buffer[4] == '3' && Buffer[5] == '0' )
                    {
                        l = 1;              a = 0;

                        for( k = 6 ;; k++ )
                        {
                            if( Buffer[k] == '\r' )
                            {
                                if( !a )    a = k - 1;
                                break;
                            }
                            else if( Buffer[k] == '.' )
                                a = k;
                            else            l *= 10;
                        }

                        if(( k - a ) != Total_DP + 1 )
                            SerialPuts( "Decimal Point Error" );
                        else
                        {
                            BatchVolume = 0;
                            l /= 10;

                            for( k = 6 ;; k++ )
                            {
                                if( Buffer[k] == '\r' )
                                    break;
                                else if( Buffer[k] >= '0' && Buffer[k] <= '9' )
                                {
                                    BatchVolume += ( Buffer[k] - '0' ) * l;
                                    l /= 10;
                                }
                            }
                            IntAscii( Total_DP, BatchVolume, PLUS, 1, 7 );
                        }
                    }
                    else if( Buffer[4] == '3' && Buffer[5] == '2' )
                    {
                        Change_Status = 0;
                        Change_Status = ( Buffer[6] - '0' ) * 10;
                        Change_Status += Buffer[7] - '0';

                        SerialPutc( '1' );
                    }
                    else
                        SerialPuts( "Invalid Format" ); 
                    SerialPuts( "\r\n" );
                }
                else
                {
                    SerialPuts( "Invalid Format" ); 
                    SerialPuts( "\r\n" );
                }

                if( Signal )
                {
                    TXE = 0;
                    if( Signal == 2 )       REN0 = 1;
                }
            }
            else if( Buffer[0] == 'I' && Buffer[1] == '0' && Buffer[2] == '0' )
            {
                if( Buffer[3] == 'W' )
                {
                    if( Buffer[4] == '0' && Buffer[5] == '2'
                      && Buffer[6] == '0' && Buffer[7] == '\r' )
                    {
                        ResetValue( 1 );
                        ResetValue( 0 );
                    }                       
                    else if( Buffer[4] == '2' && Buffer[5] == '8' )
                    {
                        if( Buffer[10] == '/' && Buffer[13] == '/' && Buffer[16] == '\r' )
                        { 
                            RTC[11] = 0x82;
                            PutRtc( rCENTURY, Buffer[6], Buffer[7] );
                            PutRtc( rYEAR, Buffer[8], Buffer[9] );
                            PutRtc( rMONTH, Buffer[11], Buffer[12] );
                            PutRtc( rDAY, Buffer[14], Buffer[15] );
                            RTC[11] = 0x02;
                        }
                    }
                    else if( Buffer[4] == '2' && Buffer[5] == '9' )
                    {
                        if( Buffer[8] == ':' && Buffer[11] ==':' && Buffer[14] =='\r' )
                        { 
                            RTC[11] = 0x82;
                            PutRtc( rHOUR, Buffer[6], Buffer[7] );
                            PutRtc( rMIN, Buffer[9], Buffer[10] );
                            PutRtc( rSEC, Buffer[12], Buffer[13] );
                            RTC[11] = 0x02;
                        }
                    }   
                }
            }
        }

        Data = GetQueue( &Buf );

        if( Data == -1 )                    return;
        else if( Data == '\r' )
        {
            Buffer[0] = 0x00;               /*Buffer[1] = 0x00;*/
            Pos = 0;
            return;
        }
        else if( Signal == 2 && Data == '\n' )
        {
            Buf.Tail = 0;                   Buf.Head = 0;
            Buffer[0] = 0x00;               Buffer[1] = 0x00;
            Pos = 0;                        return;
        }

        Buffer[ Pos++ ] = Data;

        if( Pos > 20 )
        {
            Pos = 0;                        return;
        }
    }
} 

void Parameter_liquid( Char item )
{
#pragma memory = xdata

    Char    x=0, first=0, i, d, digit=7, dis[11];
    Char    En_pos = 1;

#pragma memory = code

    Char DIS[][21] = {    " TEMPERATURE at 4mA ",           /* 0 */
                          " TEMPERATURE at 20mA",           /* 1 */
                          "     RTD OFFSET     ",           /* 2 */
                          "   IDENTIFICATION   ",           /* 3 */
                          "    FILTER FACTOR   " };         /* 4 */
#pragma memory = default

    LCD_pos( 1, 0 );                        LCD_puts( DIS[item] );
    LCD_pos( 2, 0 );                        LCD_puts( BLANK );

#if     Compensation == LIQUID

    if( item < 3 )
    {
        LCD_pos( 2, 12 );                   LCD_puts( *(Tunit+Temp_unit) );
    }

#endif
    for( i=0 ; i<11 ; i++ )                 dis[i] = ' ';

    switch( item )
    {
#if     Compensation == LIQUID
        case 0:
        case 1: Value[item] %= 100000;
                if( Value[item] >= 0 )      dis[0] = '+';
                else
                {
                    dis[0] = '-';           Value[item] *= -1;
                }

                itoa_converter( Value[item], 5, 5 );
                LCD_pos( 2, 5 );

                for( i=1; i<digit ; i++ )   dis[i] = Dis[i-1];
                break;

        case 2: Value[item-2] %= 100000;
                if( Value[item-2] >= 0 )    dis[0] = '+';
                else
                {
                    dis[0] = '-';           Value[item-2] *= -1;
                }

                digit = 5;
                itoa_converter( Value[item-2], 3, 3 );
                LCD_pos( 2, 7 );

                for( i=1; i<digit ; i++ )   dis[i] = Dis[i-1];
                break;
#endif

        case 3: digit = 2;
                dis[0] = ID[0];             dis[1] = ID[1];
                LCD_pos( 2, 9 );
                break;
        case 4: digit = 2;                  Filter %= 100;
                dis[0] = Filter/10 + '0';   dis[1] = Filter%10 + '0';
                LCD_pos( 2, 9 );
                break;
    }

    for( i=0; i<digit ; i++ )               LCD_putc( dis[i] );
    
    LCD_cursorblink( ON );                  key_ready( Enter );

    for(;;)
    {
        if( En_pos )
        {
            sync_data( LCDpos );
            if( ( item > 2 ) || x )         d = dis[x] - '0';
            En_pos = 0;
        }

        if( kUp )
        {
            beep();                         En_pos = 1;
            LCD_cursorblink( OFF );         first = 1;

            if( !x && item < 3 )
            {
                if( dis[x] == '+' )         dis[x] = '-';
                else                        dis[x] = '+';
            }
            else if( x == 1 && item < 2 )
            {
                d++;                        d %= 2;
                dis[x] = d + '0';
            }
            else
            {
                d++;                        d %= 10;
                dis[x] = d + '0';
            }
            LCD_putc( dis[x] );             key_ready( Up );
            LCD_cursorblink( ON );
        }
        else if( kDown )
        {
            beep();                         En_pos = 1;
            LCD_cursorblink( OFF );         first = 1;

            if( !x && item < 3 )
            {
                if( dis[x] == '+' )         dis[x] = '-';
                else                        dis[x] = '+';
            }
            else if( x == 1 && item < 2 )
            {
                d++;                        d %= 2;
                dis[x] = d + '0';
            }
            else
            {
                if( !d )                    d = 10;
                d--;                        dis[x] = d +'0';
            }
            LCD_putc( dis[x] );             key_ready( Down );
            LCD_cursorblink( ON );
        }
        else if( kShift )
        {
            beep();                         first = 1;
            En_pos = 1;
            LCDpos++;                       x++;
            if( dis[x] == '.' )
            {
                LCDpos++;                   x++;
            }
            if( x == digit )
            {
                x = 0;

                switch( item )
                {
                    case 0:
                    case 1: LCD_pos( 2, 5 );break;
                    case 2: LCD_pos( 2, 7 );break;
                    case 3:
                    case 4: LCD_pos( 2, 9 );break;
                }
            }
            key_ready( Shift );
        }
        else if( kMode )
        {
            beep();                         En_pos = 1;
            LCD_cursorblink( OFF );
            if( !first )
            {
                if( item < 3 )              dis[0] = '+';
                else
                {
                    dis[0] = '0';
                    if( item == 4 )         dis[1] = '1';
                }

                LCD_putc( dis[0] );

                if( item == 4 )             LCD_putc( dis[1] );
                else
                {
                    for( i=1; i<digit; i++ )
                    {
                        if( dis[i] != '.' )
                            dis[i] = '0';
                        LCD_putc( dis[i] );
                    }
                }
            }
            else
            {
                if( !x && item<3 )          dis[x] = '+';
                else                        dis[x] = '0';
                LCD_putc( dis[x] );
            }
            key_ready( Mode );              LCD_cursorblink( ON );
        }
        else if( kEnter )
        {
            beep();                         LCD_cursorblink( OFF );
            switch( item )
            {
#if     Compensation == LIQUID
                case 0:
                case 1: Value[item] = 0;
                        for( i=1; i<digit ; i++ )
                        {
                            if( dis[i] != '.' )
                            {
                                Value[item] += dis[i] - '0';
                                if( i != digit-1 )
                                    Value[item] *= 10;
                            }
                        }
                        if( dis[0] != '+' )
                            Value[item] *= -1;
                        break;

                case 2: Value[item-2] = 0;
                        for( i=1; i<digit ; i++ )
                        {
                            if( dis[i] != '.' )
                            {
                                Value[item-2] += dis[i] - '0';
                                if( i != digit-1 )
                                    Value[item-2] *= 10;
                            }
                        }
                        if( dis[0] != '+' )
                            Value[item-2] *= -1;
                        break;
#endif
                case 3: ID[0] = dis[0];     ID[1] = dis[1];
                        ID_A = ID[0];       ID_B = ID[1];
                        break;
                case 4: Filter = ( dis[0] - '0' ) * 10;
                        Filter += dis[1] - '0';
                        if( !Filter )       Filter = 1;
                        break;
            }
            key_ready( Enter );
            set_ini();
            break;
        }
    }
}

void Parameter_flow( Char item )
{
#pragma memory = xdata

    Char    x=0, first=0, i, d, digit, dis[11], error=0;
    Char    En_pos = 1;
    Long    a;
    float   constant=0;

#pragma memory = code

    Char SIGNAL[][21] = { "      K-FACTOR      ",           /* 0 */
                          " FLOW SPAN(at 20mA) ",           /* 1 */
                          " FLOW ZERO(at 4mA)  ",           /* 2 */
                          "    FLOW CUT-OFF    ",           /* 3 */
                          "   OUTPUT at 4mA    ",           /* 4 */
                          "   OUTPUT at 20mA   " };         /* 5 */

#pragma memory = default

    LCD_pos( 1, 0 );

    if( item < 2 )
#if     Meter == PULSE_FLOW
        LCD_puts( SIGNAL[0] );
#elif   Meter == ANALOG_FLOW
        LCD_puts( SIGNAL[item+1] );
#endif
    else                                    LCD_puts( SIGNAL[item] );

    LCD_pos( 2, 0 );                        LCD_puts( BLANK );

#if     Meter == PULSE_FLOW

    if( !item )                     /* if K-Factor Setting */
    {
        if( Total_unit != 10 )
        {
            LCD_pos( 2, 15 );
            LCD_puts( *(Totaldis + Total_unit) );
            LCD_pos( 2, 14 );               LCD_puts( "P/" );
        }
    }
#elif   Meter == ANALOG_FLOW

    if( item < 2 )
    {
        if( Total_unit != 10 )
        {
            LCD_pos( 2, 13 );
            LCD_puts( *(Totaldis + Total_unit) );
            LCD_puts( *(Rateunit + TimeBase) );
        }
    }
    else if( item == 3 )
    {
        LCD_pos( 2, 13 );                    LCD_putc( '%' );
    }

#endif
    else
    {
        LCD_pos( 2, 11 );

        LCD_puts( *(Totaldis + Total_unit) );
        LCD_puts( *(Rateunit + TimeBase) );
    }

    for( i=0 ; i<11 ; i++ )                 dis[i] = ' ';

    switch( item )
    {
        case 0:
        case 1: Option[item+7] %= 1000000000;
                LCD_pos( 2, 3 );            digit = 10;
                itoa_converter( Option[item+7], 9, 6 );
                break;

#if     Meter == ANALOG_FLOW
        case 3: digit = 5;                    Cutvalue %= 10000;
                LCD_pos( 2, 7 );
                itoa_converter( Cutvalue, 4, 3 );
                break;
#endif
        case 4:
        case 5: Option[item-4] %= 10000000;
                if( Rate_DP )
                {
                    LCD_pos( 2, 3 );
                    digit = 8;
                    itoa_converter( Option[item-4], 7, 8-Rate_DP );
                }
                else
                {
                    LCD_pos( 2, 4 );
                    digit = 7;
                    itoa_converter( Option[item-4], 7, 0 );
                }
                break;
    }

    for( i=0; i<digit ; i++ )
    {
        dis[i] = Dis[i];                    LCD_putc( dis[i] );
    }
    
    LCD_cursorblink( ON );                  key_ready( Enter );

    for(;;)
    {
        if( En_pos )
        {
            sync_data( LCDpos );            d = dis[x] - '0';
            En_pos = 0;
        }

        if( kUp )
        {
            beep();                         En_pos = 1;
            LCD_cursorblink( OFF );         first = 1;
            d++;                            d %= 10;
            dis[x] = d +'0';                LCD_putc( dis[x] );
            key_ready( Up );                LCD_cursorblink( ON );
        }
        else if( kDown )
        {
            beep();                         En_pos = 1;
            LCD_cursorblink( OFF );         first = 1;
            if( !d )                        d = 10;
            d--;
            dis[x] = d +'0';                LCD_putc( dis[x] );
            key_ready( Down );              LCD_cursorblink( ON );
        }
        else if( kShift )
        {
            beep();                         first = 1;
            En_pos = 1;
            LCDpos++;                       x++;
            if( dis[x] == '.' )
            {
                LCDpos++;                   x++;
            }
            if( x == digit )
            {
                x = 0;
                switch( item )
                {
                    case 0:
                    case 1:
                    case 2: LCD_pos( 2, 3 );break;
                    case 3: LCD_pos( 2, 7 );break;
                    case 4:
                    case 5: if( Rate_DP )   LCD_pos( 2, 3 );
                            else            LCD_pos( 2, 4 );
                            break;
                }
            }
            key_ready( Shift );
        }
        else if( kMode )
        {
            beep();                         LCD_cursorblink( OFF );
            En_pos = 1;
            if( !first )
            {
                for( i=0; i<digit; i++ )
                {
                    if( dis[i] != '.' )     dis[i] = '0';

                    LCD_putc( dis[i] );
                }
            }
            else
            {
                dis[x] = '0';               LCD_putc( dis[x] );
            }
            key_ready( Mode );              LCD_cursorblink( ON );
        }
        else if( kEnter )
        {
            beep();
            switch( item )
            {
                case 0:
                case 1: Option[item+7] = 0;
                        for( i=0 ; i<digit ; i++ )
                        {
                            if( dis[i] != '.' )
                            {
                                Option[item+7] += dis[i] - '0';
                                if( i != digit-1 )
                                    Option[item+7] *= 10;
                            }
                        }
                        break;

#if     Meter == ANALOG_FLOW
                case 3: Cutvalue = 0;
                        for( i=0; i<digit ; i++ )
                        {
                            if( dis[i] != '.' )
                            {
                                Cutvalue += dis[i] - '0';
                                if( i != digit-1 )
                                    Cutvalue *= 10;
                            }
                        }
                        CV_B = Cutvalue;    CV_A = ( Cutvalue >> 8 );
                        break;
#endif
                case 4:
                case 5: Option[item-4] = 0;
                        for( i=0; i<digit ; i++ )
                        {
                            if( dis[i] != '.' )
                            {
                                Option[item-4] += dis[i] - '0';
                                if( i != digit-1 )
                                    Option[item-4] *= 10;
                            }
                        }

                        if( item == 5 && ZeroAlarm >= SpanAlarm )
                            error = 1;
                        break;
            }
            key_ready( Enter );

            if( error )
            {
                LCD_pos( 2, 0 );            LCD_puts( "     OVER RANGE!    " );
                Buzz_ON;                    delay( 25000 );
                Buzz_OFF;                   delay( 10000 );
                Buzz_ON;                    delay( 25000 );
                Buzz_OFF;                   En_pos = 1;
                error = 0;                  x = 0;
                LCD_pos( 2, 0 );            LCD_puts( BLANK );

                if( item > 3 )
                {
                    LCD_pos( 2, 11 );

                    LCD_puts( *(Totaldis + Total_unit) );
                    LCD_puts( *(Rateunit + TimeBase) );

                    if( Rate_DP )           LCD_pos( 2, 3 );
                    else                    LCD_pos( 2, 4 );

                    for( i=0; i<digit ; i++ )   LCD_putc( dis[i] );
                }
            }
            else
            {
                LCD_cursorblink( OFF );
                set_ini();
                break;
            }
        }
    }
}

void Set_Flow( void )
{
#pragma memory = code

    Char CAL[][21] =  {     "  FLOW TOTAL UNIT   ",           /* 0 */
                            "  FLOW CORRECTION   ",           /* 1 */
                            "   TOTAL DECIMAL    ",           /* 2 */
                            "    RATE DECIMAL    ",           /* 3 */
                            "   FLOW TIME BASE   ",           /* 4 */
                            "  ACC-TOTAL RESET   ",           /* 5 */
                            "DISPLAY AUTO RETURN ",           /* 6 */
                            "  RANGE-OVER CHECK  ",           /* 7 */
                            "     BRIGHTNESS     ",           /* 8 */
                            "   END OF PROCESS   " };         /* 9 */

#if     Meter == ANALOG_FLOW

    Char CORRCT[][21] = {   "       LINEAR       ",           /* 0 */
                            "     SQUARE ROOT    " };         /* 1 */
#endif

    Char DP[][21] =  {      "       00000        ",           /* 0 */
                            "       000.0        ",           /* 1 */
                            "       00.00        ",           /* 2 */
                            "       0.000        " };         /* 3 */

#if     DisMode == VFD_DISPLAY

    Char TOT[][21] =  {     "         ml         ",           /* 0 */
                            "         ltr        ",           /* 1 */
                            "         Ml         ",           /* 2 */
                            "         in?        ",           /* 3 */
                            "         ft?        ",           /* 4 */
                            "         m?         ",           /* 5 */
                            "         cc         ",           /* 6 */
                            "         kg         ",           /* 7 */
                            "         ton        ",           /* 8 */
							"         gal        ",			  /* 9 */
                            "        NONE        " };         /* 10 */
                                                        /* 0xb3 179 */
                                                        /* m^3 */
    Char ADJUST[][21] = {   "       100 %        ",           /* 0 */
                            "        25 %        ",           /* 1 */
                            "        50 %        ",           /* 2 */
                            "        75 %        " };         /* 3 */
#elif   DisMode == LCD_DISPLAY

    Char TOT[][21] =  {     "         ml         ",           /* 0 */
                            "         ltr        ",           /* 1 */
                            "         Ml         ",           /* 2 */
                            "         in        ",           /* 3 */
                            "         ft        ",           /* 4 */
                            "         m         ",           /* 5 */
                            "         cc         ",           /* 6 */
                            "         kg         ",           /* 7 */
                            "         ton        ",           /* 8 */
							"         gal        ",			  /* 9 */
                            "        NONE        " };         /* 10 */
                                                        /* 0x02 */
#endif

    Char TBASE[][21] = {    "       SECOND       ",           /* 0 */
                            "       MINUTE       ",           /* 1 */
                            "        HOUR        ",           /* 2 */
                            "        DAY         " };         /* 3 */

    Char RESET[][21] = {    "         NO         ",           /* 0 */
                            "         YES        " };         /* 1 */

    Char RETURN[][21] = {   "       Disable      ",           /* 0 */
                            "        Enable      " };         /* 1 */
#pragma memory = xdata

    Char    item=0, view=1, yesno=0;

#pragma memory = default

    LCD_pos( 2, 0 );                        LCD_puts( BLANK );
    key_ready( Enter );

    for(;;)
    {
        if( view )
        {
            view = 0;
            LCD_pos( 1, 0 );                LCD_puts( CAL[item] );
            LCD_pos( 2, 0 );
            switch( item )
            {
                case 0: Total_unit %= 11;
                        LCD_puts( TOT[Total_unit] );
                        break;
#if     Meter == ANALOG_FLOW
                case 1: Correct %= 2;
                        LCD_puts( CORRCT[Correct] );
                        break;
#endif
                case 2: Total_DP %= 4;
                        LCD_puts( DP[Total_DP] );
                        break;
                case 3: Rate_DP %= 4;
                        LCD_puts( DP[Rate_DP] );
                        break;
                case 4: TimeBase %= 4;
                        LCD_puts( TBASE[TimeBase] );
                        break;
                case 5: yesno %= 2;
                        LCD_puts( RESET[yesno] );
                        break;
                case 6: AutoReturn %= 2;
                        LCD_puts( RETURN[AutoReturn] );
                        break;
                case 7: Warning %= 2;
                        LCD_puts( RETURN[Warning] );
                        break;
#if     DisMode == VFD_DISPLAY
                case 8: VFD_mode %= 4;
                        LCD_puts( ADJUST[VFD_mode] );
                        VFD_adjust( VFD_mode );
                        break;
#endif
                case 9: LCD_puts( BLANK );
                        break;
            }
        }

        if( kUp )
        {
            beep();                         view = 1;
            switch( item )
            {
                case 0: Total_unit++;       break;
#if     Meter == ANALOG_FLOW
                case 1: Correct++;          break;
#endif
                case 2: Total_DP++;         break;
                case 3: Rate_DP++;          break;
                case 4: TimeBase++;         break;
                case 5: yesno++;            break;
                case 6: AutoReturn++;       break;
                case 7: Warning++;          break;
#if     DisMode == VFD_DISPLAY
                case 8: VFD_mode++;         break;
#endif
            }
            key_ready( Up );
        }
        else if( kEnter )
        {
            beep();                         view = 1;
            switch( item )
            {
                case 0:
#if     Meter == PULSE_FLOW
                        Parameter_flow( 0 );    /* K-Factor */
                        Parameter_liquid( 4 );  /* Filter Factor */
                        item = 1;

#elif    Meter == ANALOG_FLOW
                        Parameter_flow( 0 );    /* Flow Span */
                        Parameter_flow( 1 );    /* Flow Zero */
#endif
                        break;

#if     Meter == ANALOG_FLOW
                case 1: Parameter_flow( 3 );    /* Cut-Off Setting */
                        Parameter_liquid( 4 );  /* Filter Factor */
                        break;
#endif
                case 5: if( yesno )
                        {
                            ResetValue( 1 );
                            ResetValue( 0 );
                        }
                        LCD_pos( 2, 0 );    LCD_puts( BLANK );
                        break;
                case 6:
#if     Meter == PULSE_FLOW
                        item++;
#endif

#if     DisMode == LCD_DISPLAY &&  Meter == ANALOG_FLOW
                        item = 6;
                        break;
#elif   DisMode == LCD_DISPLAY
                        item++;
#endif
                case 7:
#if     DisMode == LCD_DISPLAY
                        item = 8;
#endif
                        break;
                case 9: set_ini();
                        break;
            }
            item++;
            if( item == 10 )                break;
            key_ready( Enter );
        }
    }
}

void Parameter_batch( Char item )
{
/********************************************************/
/*  Const_C[0] : PrestartTime :  Relay 1 START Time     */
/*  Const_C[1] : OutTime      :  Flow OUT Time          */
/*  Const_C[2] : EndTime      :  Batch END Time         */
/*  Const_C[3] : AutoStart    :  Batch Restart Time     */
/*  Option[2]  : BatchVolume  :  Set Batch Quantity     */
/*  Option[3]  : BatchLimit   :  Set Batch Limit        */
/*  Option[4]  : PrestartQ    :  Relay 1 Start Quantity */
/*  Option[5]  : PrestopQ     :  Relay 1 Stop Quantity  */
/*  Option[6]  : OverQ        :  Overrun Quantity       */
/********************************************************/

#pragma memory = xdata

    Char    x=0, first=0, i, d, digit=3, dis[11], error=0;
    Char    En_pos = 1;
    Int     temp=0;

#pragma memory = code

    Char ITEM[][21] = {     "   PRE-START TIME   ",     /* 0 */
                            "FLOW OUT CHECK TIME ",     /* 1 */
                            " END OF BATCH TIME  ",     /* 2 */
                            " AUTO RE-START TIME ",     /* 3 */
                            "  BATCH PRESET Qt.  ",     /* 4 */
                            "BATCH LIMIT QUANTITY",     /* 5 */
                            " PRE-START QUANTITY ",     /* 6 */
                            "  PRE-STOP QUANTITY ",     /* 7 */
                            " MANUAL CORRECT Qt. " };   /* 8 */

#pragma memory = default


    key_ready( Enter );

    LCD_pos( 1, 0 );                        LCD_puts( ITEM[item] );
    LCD_pos( 2, 0 );                        LCD_puts( BLANK );

    if( item < 4 )
    {
        LCD_pos( 2, 11 );                   LCD_puts( "SEC" );
    }
    else
    {
        LCD_pos( 2, 13 );
        LCD_puts( *( Totaldis + Total_unit ) );
    }

    for( i=0 ; i<11 ; i++ )                 dis[i] = ' ';

    switch( item )
    {
        case 0:
        case 1:
        case 2:
        case 3: Const_C[item] %= 256;
                itoa_converter( Const_C[item], 3, 0 );
                LCD_pos( 2, 7 );
                break;
        default:Option[item-2] %= 10000000;
                if( Total_DP )
                {
                    digit = 8;
                    itoa_converter( Option[item-2], 7, 8-Total_DP );
                    LCD_pos( 2, 5 );
                }
                else
                {
                    digit = 7;
                    itoa_converter( Option[item-2], 7, 0 );
                    LCD_pos( 2, 6 );
                }
                break;
    }

    for( i=0; i<digit ; i++ )
    {
        dis[i] = Dis[i];                    LCD_putc( dis[i] );
    }
    
    LCD_cursorblink( ON );                  key_ready( Enter );

    for(;;)
    {
        if( En_pos )
        {
            sync_data( LCDpos );            d = dis[x] - '0';
            En_pos = 0;
        }

        if( kUp )
        {
            beep();                         En_pos = 1;
            LCD_cursorblink( OFF );         first = 1;
            d++;                            d %= 10;
            dis[x] = d +'0';                LCD_putc( dis[x] );
            key_ready( Up );
            LCD_cursorblink( ON );
        }
        else if( kDown )
        {
            beep();                         En_pos = 1;
            LCD_cursorblink( OFF );         first = 1;
            if( !d )                        d = 10;
            d--;
            dis[x] = d +'0';                LCD_putc( dis[x] );
            key_ready( Down );
            LCD_cursorblink( ON );
        }
        else if( kShift )
        {
            beep();                         first = 1;
            En_pos = 1;
            LCDpos++;                       x++;
            if( dis[x] == '.' )
            {
                LCDpos++;                   x++;
            }
            if( x == digit )
            {
                x = 0;

                if( item < 4 )              LCD_pos( 2, 7 );
                else
                {
                    if( Total_DP )          LCD_pos( 2, 5 );
                    else                    LCD_pos( 2, 6 );
                }
            }
            key_ready( Shift );
        }
        else if( kMode )
        {
            beep();                         LCD_cursorblink( OFF );
            En_pos = 1;
            if( !first )
            {
                for( i=0; i<digit; i++ )
                {
                    if( dis[i] != '.')      dis[i] = '0';

                    LCD_putc( dis[i] );
                }
            }
            else
            {
                dis[x] = '0';               LCD_putc( dis[x] );
            }
            key_ready( Mode );              LCD_cursorblink( ON );
        }
        else if( kEnter )
        {
            beep();
            switch( item )
            {
                case 0:
                case 1:
                case 2:
                case 3: temp = 0;
                        for( i=0 ; i<3 ; i++ )
                        {
                            temp += dis[i] - '0';
                            if( i != 2 )
                            {
                                temp *= 10;
                            }
                        }
                        if( temp > 255 )        error = 1;
                        else                    Const_C[item] = temp;
                        break;
                default:Option[item-2] = 0;
                        for( i=0; i<digit ; i++ )
                        {
                            if( dis[i] != '.' )
                            {
                                Option[item-2] += dis[i] - '0';
                                if( i != digit-1 )
                                    Option[item-2] *= 10;
                            }
                        }
                        if(item==4 && ( ((BatchVolume>BatchLimit) && BatchLimit)
                            ||!BatchVolume) )
                            error = 1;
                        break;
            }

            key_ready( Enter );

            if( error )
            {
                LCD_pos( 2, 0 );            LCD_puts( "     OVER RANGE!    " );
                Buzz_ON;                    delay( 25000 );
                Buzz_OFF;                   delay( 10000 );
                Buzz_ON;                    delay( 25000 );
                Buzz_OFF;                   En_pos = 1;
                error = 0;                  x = 0;
                LCD_pos( 2, 0 );            LCD_puts( BLANK );

                if( item < 4 )
                {
                    LCD_pos( 2, 11 );       LCD_puts( "SEC" );
                    LCD_pos( 2, 7 );
                }
                else
                {
                    LCD_pos( 2, 13 );
                    LCD_puts( *( Totaldis + Total_unit ) );

                    if( Total_DP )          LCD_pos( 2, 5 );
                    else                    LCD_pos( 2, 6 );
                }
                for( i=0; i<digit ; i++ )   LCD_putc( dis[i] );
            }
            else
            {
                LCD_cursorblink( OFF );
                set_ini();
                break;
            }
        }
    }
}

void Set_Batch( void )
{
#pragma memory = xdata

    Char    item=0, view = 1;

#pragma memory = code

    Char ITEM[][21] =   {   "   PRE-START MODE   ",     /* 0 */
                            "OVERRUN CORRECT MODE",     /* 1 */
                            "AUTO OPERATION MODE ",     /* 2 */
							"    RESET METHOD    ",     /* 3 */
                            "  BATCH COUNT MODE  ",     /* 4 */
                            "   INTER-LOCK USE   ",     /* 5 */
                            "   END OF PROCESS   " };   /* 6 */

    Char STMODE[][21] = {   "      SET TIME      ",     /* 0 */
                            "    SET QUANTITY    " };   /* 1 */

    Char OVERMODE[][21] = { "        AUTO        ",     /* 0 */
                            "       MANUAL       ",     /* 1 */
                            "       DISABLE      " };   /* 2 */

    Char OPMODE[][21] =  {  "    AUTO RESTART    ",     /* 0 */
                            "    AUTO ZERO SET   ",     /* 1 */
                            "     AUTO RESET     ",     /* 2 */
                            "       DISABLE      " };   /* 3 */

	Char METHOD[][21] = {   "       MANUAL       ",     /* 0 */
	                        "        AUTO        " };   /* 1 */

    Char UPDN[][21]  =  {   "         UP         ",     /* 0 */
                            "        DOWN        " };   /* 1 */

    Char INTER[][21] =  {   "       DISABLE      ",     /* 0 */
                            "        ENABLE      " };   /* 1 */
#pragma memory = default

    LCD_pos( 2, 0 );                        LCD_puts( BLANK );

    Parameter_batch( 5 );                   /* batch limit setting */

    key_ready( Enter );

    for(;;)
    {
        if( view )
        {
            view = 0;
            LCD_pos( 1, 0 );                LCD_puts( ITEM[item] );
            LCD_pos( 2, 0 );

            switch( item )
            {
                case 0: Start_mode %= 2;    LCD_puts( STMODE[Start_mode] );
                        break;
                case 1: Over_flag %= 3;     LCD_puts( OVERMODE[Over_flag] );
                        break;
                case 2: Op_mode %= 4;       LCD_puts( OPMODE[Op_mode] );
                        break;
				case 3: ResetMethod %= 2;   LCD_puts( METHOD[ResetMethod] );
				        break;
                case 4: Count_flag %= 2;    LCD_puts( UPDN[Count_flag] );
                        break;
                case 5: Inter_use %= 2;     LCD_puts( INTER[Inter_use] );
                        break;
                case 6: LCD_puts( BLANK );
                        break;
            }
        }

        if( kUp )
        {
            beep();                         view = 1;
            switch( item )
            {
                case 0: Start_mode++;       break;
                case 1: Over_flag++;
                        if( !Over_flag && !EndTime )
                            Over_flag++;
                        break;
                case 2: Op_mode++;
                        if( ( !Op_mode || Op_mode == 2 ) && !EndTime )
                            Op_mode++;
                        break;
				case 3: ResetMethod++;      break;
                case 4: Count_flag++;       break;
                case 5: Inter_use++;        break;
            }
            key_ready( Up );
        }
        else if( kEnter )
        {
            beep();                         view = 1;
            switch( item )
            {
                case 0: if( !Start_mode )           /* Pre-Start time */
                            Parameter_batch( 0 );
                        else                        /* Pre-Start Quantity */
                            Parameter_batch( 6 );
                        Parameter_batch( 7 );   /* Pre-Stop Quantity Set */
                        Parameter_batch( 1 );   /* Flow Out check time */
                        Parameter_batch( 2 );   /* End of batch time */
                        break;
                case 1: if( Over_flag == 1 )    /* Manual Overrun correction */
                            Parameter_batch( 8 );
                        else                OverQ = 0;
                        break;
                case 2: if( !Op_mode )          /* Auto Re-Start time */
                            Parameter_batch( 3 );
                        break;

                case 4: if( Basic_NR )      item++;
                        break;
            }
            item++;
            if( item == 7 )                 break;
            key_ready( Enter );
        }
    }
}

#if     Compensation == LIQUID

void Set_Steam(void)
{
    Char item=0, view=1;

#pragma memory = code
    Char CAL[][21] =  {   "    SENSOR INPUT    ",           /* 1 */
                          "     TEMP.  UNIT    ",           /* 2 */
                          "    PRESS.  UNIT    ",           /* 3 */
                          "    TEMP. SENSOR    ",           /* 4 */
                          "   PRESS. SENSOR    ",           /* 5 */
                          "  RANGE-OVER CHECK  ",           /* 6 */
                          "   END OF PROCESS   " };         /* 7 */

    Char PUNIT[][21] = {  "         kPa        ",
                          "         bar        ",
                          "       kg/cm^2      " };

    Char TUNIT[][21] = {  "    CELSIUS         ",
                          "    FAHRENHEIT      " };

    Char DISUNIT[][21]={  "        MASS        ",
                          "       ENERGY       " };

    Char MA_EN[][21] =  { "         kg         ",
                          "        ton         ",
                          "         MJ         ",
                          "         GJ         " };

    Char TEMP[][21] =  {  "       4-20mA       ",
                          "     RTD Pt-100     " };

    Char PRESS[][21] = {  "      ABSOLUTE      ",
                          "        GUAGE       " };

    Char INPUT[][21] = {  "        TEMP.       ",
                          "       PRESS.       " };

    Char CHECK[][21] = {  "      Disable       ",
                          "       Enable       " };

#pragma memory = default

    key_ready( Enter );

    for(;;)
    {
        if( view )
        {
            view = 0;
            LCD_pos( 1, 0 );                LCD_puts( CAL[item] );
            LCD_pos( 2, 0 );

            switch( item )
            {
                case 0:
                        break;
                case 1: break;
                case 2: Temp_unit %= 2;
                        LCD_puts( TUNIT[Temp_unit] );
                        LCD_pos( 2, 15 );
#if     DisMode == LCD_DISPLAY
                        LCD_putc( 0xdf );
#elif   DisMode == VFD_DISPLAY
                        LCD_putc( 0xb0 );
#endif
                        LCD_puts( *( Tunit+Temp_unit) );
                        break;
                case 3: Pre_unit %= 3;
                        LCD_puts( PUNIT[Pre_unit] );
                        break;
                case 4: Temp_type %= 2;
                        LCD_puts( TEMP[Temp_type] );
                        break;
                case 5: Press_type %= 2;
                        LCD_puts( PRESS[Press_type] );
                        break;
                case 6: Warning %= 2;
                        LCD_puts( CHECK[Warning] );
                        break;
                case 7: LCD_puts( BLANK );
                        break;
            }
        }

        if( kUp )
        {
            beep();                         view = 1;
            switch( item )
            {
                case 0: break;
                case 1: break;
                case 2: Temp_unit++;        break;
                case 3: Pre_unit++;         break;
                case 4: Temp_type++;        break;
                case 5: Press_type++;       break;
                case 6: Warning++;          break;
            }
            key_ready( Up );
        }
        else if( kEnter )
        {
            beep();                         view = 1;
            switch( item )
            {
                case 0: item = 1;
                        break;
                case 1: Parameter_liquid( 7 );
                        break;
                case 3: break;
                case 4: if( !Temp_type )
                        {
                            Parameter_liquid( 0 );
                            Parameter_liquid( 1 );
                        }
                        else
                            Parameter_liquid( 2 );
                        break;
                case 5: if( Press_type )
                            Parameter_liquid( 4 );

                        Parameter_liquid( 5 );
                        Parameter_liquid( 6 );
                        break;
            }
            item++;
            if( item == 8 )                 break;
            key_ready( Enter );
        }
    }
}
#endif

void Set_Option( void )
{
#pragma memory = code

    Char OPTION[][21] = { "     SIGNAL TYPE    ",           /* 0 */
                          "      BAUD RATE     ",           /* 1 */
                          "    DATA LOGGING    ",           /* 2 */
                          "    PRINT METHOD    ",           /* 3 */
                          "     PRINT UNIT     ",           /* 4 */
                          "     DATE FORMAT    ",           /* 5 */
                          "   DATE&TIME SET    ",           /* 6 */
                          "   END OF PROCESS   " };         /* 7 */


    Char COMM[][21] = {   "       RS-232       ",
                          "       RS-422       ",
                          "       RS-485       " };

    Char BAUD[][21] = {   "      1200 BPS      ",
                          "      2400 BPS      ",
                          "      4800 BPS      ",
                          "      9600 BPS      ",
                          "     19200 BPS      " };

    Char METHOD[][21] = { "      COMPUTER      ",
                          "       PRINTER      " };

    Char method[][21] = { "   BATCH COMPLETED  ",
                          "      NOT USED      " };

    Char DATE[][21] = {   "        KOREA       ",
                          "         USA        ",
                          "       EUROPE       " };

    Char PRINTUNIT[][21]={"        NONE        ",
                          "       DEFALUT      " };
#pragma memory = xdata

    Char    item=0, view=1;

#pragma memory = default

    key_ready( Enter );

    if( Basic_A || Basic_AC || Basic_AR || Basic_ARC )
    {
        Parameter_flow( 4 );                /* Analog Output at 4mA */
        Parameter_flow( 5 );                /* Analog Output at 20mA */

        if( Basic_A || Basic_AR )           item = 7;
        else                                item = 0;
    }

    for(;;)
    {
        if( view )
        {
            view = 0;
            LCD_pos( 1, 0 );                LCD_puts( OPTION[item] );
            LCD_pos( 2, 0 );
            switch( item )
            {
                case 0: Signal %= 3;
                        LCD_puts( COMM[Signal] );
                        break;
                case 1: Baud %= 5;
                        LCD_puts( BAUD[Baud] );
                        break;
                case 2: CommType %= 2;
                        LCD_puts( METHOD[CommType] );
                        break;
                case 3: PrintMode %= 2;
                        LCD_puts( method[PrintMode] );
                        break;
                case 4: Print_unit %= 2;
                        LCD_puts( PRINTUNIT[Print_unit] );
                        break;
                case 5: DateFormat %= 3;
                        LCD_puts( DATE[DateFormat] );
                        break;
                case 6: Set_time();
                        Parameter_liquid( 3 );
                        item = 7;          view = 1;
                        key_ready( Enter );
                        break;
                case 7: LCD_puts( BLANK );
                        break;
            }
        }

        if( kUp )
        {
            beep();                         view = 1;
            switch( item )
            {
                case 0: Signal++;           break;
                case 1: Baud++;             break;
                case 2: CommType++;         break;
                case 3: PrintMode++;        break;
                case 4: Print_unit++;       break;
                case 5 :DateFormat++;       break;
            }
            key_ready( Up );
        }
        else if( kEnter )
        {
            beep();                         view = 1;

            switch( item )
            {
                case 1: serial_ini( Baud ); delay( 600 );
                        break;
                case 2: if( !CommType )
                        {
                            PrintMode = 1;
                            item = 4;
                        }
                        break;
                case 3: break;
            }
            item++;
            if( item == 8 )                 break;
            key_ready( Enter );
        }
    }
}

void Set_Test( void )
{
#pragma memory = code

    Char TEST[][21] =  {  "  FREQUENCY INPUT   ",       /* 0 */
                          "    ANALOG INPUT    ",       /* 1 */
                          " TEMPERATURE INPUT  ",       /* 2 */
                          "  4mA OUTPUT ADJUST ",       /* 3 */
                          " 20mA OUTPUT ADJUST ",       /* 4 */
                          " RELAY1 ON/OFF TEST ",       /* 5 */
                          " RELAY2 ON/OFF TEST ",       /* 6 */
                          "   END OF PROCESS   " };     /* 7 */

    Char RELAY[][21] = {  "     RELAY  OFF     ",
                          "     RELAY  ON      " };

#pragma memory = xdata

    Char    status=0, item=0, view=1, sign=1, filtering;
    Int     Nowtemp, k;
    int     Offtemp;
    float   value;

#pragma memory = default

#if     Meter == PULSE_FLOW
    filtering = Filter;                     Filter = 4;
#elif   Meter == ANALOG_FLOW
    item = 1;
#endif

    key_ready( Enter );

    for(;;)
    {
        if( view )
        {
            view = 0;
            LCD_pos( 1, 0 );                LCD_puts( TEST[item] );

            switch( item )
            {
#if     Meter == PULSE_FLOW
                case 0: Nowtemp = NowTotal;
                        Offtemp = Offset;   EX0 = 1;
                        break;
#endif
                case 3:
                case 4: DA( Nowtemp );      delay( 2500 );
                        break;
                case 5:
                case 6: LCD_pos( 2, 0 );
                        LCD_puts( RELAY[status] );
                        break;
                default:LCD_pos( 2, 0 );
                        LCD_puts( BLANK );
                        break;
            }
        }

        if( !DisEn )
        {
            LCD_pos( 2, 6 );                DisEn = 1;

            switch( item )
            {
#if     Meter == PULSE_FLOW
                case 0: value = ( Rate[0]+Rate[1]+Rate[2]+Rate[3] ) / 4.;
                        LCD_int( 1, PLUS, value*10, 6 );
                        LCD_puts( " Hz" );
                        break;
#elif   Meter == ANALOG_FLOW
                case 1: Offtemp = 16. / RANGE * 100 * ad_converter( FLOW1 );
                        LCD_int( 2, PLUS, Offtemp, 6 );
                        LCD_puts( " mA" );
                        break;
#endif

#if     Compensation == LIQUID

                case 2:
#if     TempType == MA
                        Offtemp = 16. / RANGE * 100 * ad_converter( TEMP1 );
                        LCD_int( 2, PLUS, Offtemp, 6 );
                        LCD_puts( " mA" );
#elif   TempType == RTD
                        Nowtemp = ad_converter( RTD1 );

                        if( Nowtemp < 70 || Nowtemp > 4090 )
                        {
                            LCD_pos( 2, 0 );
                            LCD_puts( "     RTD FAULT      " );
                        }
                        else
                        {
                            value = Sensor_read( RTD1, Nowtemp );

                            Offtemp = value * 10;

                            LCD_pos( 2, 6 );

                            if( Offtemp & 0x8000 )
                                LCD_int( 1, MINUS, ( ~Offtemp+0x01 ) , 6 );
                            else
                                LCD_int( 1, PLUS , Offtemp, 6 );

                            LCD_puts( *( Tunit+Temp_unit ) );
                        }
#endif
                        break;
#endif
                case 3:
                case 4: Nowtemp %= 4096;
                        DA( Nowtemp );              delay( 5000 );
                        break;
                default:break;
            }
        }

        if( kUp )
        {
            beep();
            switch( item )
            {
                case 3:
                case 4: k = 0;
                        LCD_pos( 2, 10 );           LCD_putc( '>' );
                        while( kUp && k<20000 )     k++;

                        if( k>19999 && kUp )
                        {
                            beep();
                            while( kUp )
                            {
                                Nowtemp++;          Nowtemp %= 4096;
                                DA( Nowtemp );      delay( 2500 );
                            }
                        }
                        else                        Nowtemp++;
                        DA( Nowtemp );
                        break;
                case 5:
                case 6: view = 1;
                        status++;                   status %= 2;
                        if( item == 5 )
                        {
                            if( status )            RLY1_ON;
                            else                    RLY1_OFF;
                        }
                        else
                        {
                            if( status )            RLY2_ON;
                            else                    RLY2_OFF;
                        }
                        break;
            }
            key_ready( Up );
        }
        else if( kDown )
        {
            beep();

            if( item == 3 || item == 4 )
            {
                k = 0;
                LCD_pos( 2, 10 );           LCD_putc( '<' );

                while( kDown && k<20000 )   k++;

                if( k > 19999 && kDown )
                {
                    beep();
                    while( kDown )
                    {
                        if( !Nowtemp )      Nowtemp = 4095;
                        else                Nowtemp--;

                        DA( Nowtemp );      delay( 2500 );
                    }
                }
                else
                {
                    if( !Nowtemp )          Nowtemp = 4095;
                    else                    Nowtemp--;
                }
                DA( Nowtemp );
            }
            key_ready( Down );
        }
        else if( kMode )
        {
            if( item == 3 || item == 4 )
            {
                beep();
                if( item == 3 )             Nowtemp = 185;
                else                        Nowtemp = 4025;
            }
            view = 1;
            key_ready( Mode );
        }
        else if( kEnter )
        {
            beep();                         view = 1;
            LCD_pos( 2, 0 );                LCD_puts( BLANK );

            switch( item )
            {
#if     Meter == PULSE_FLOW
                case 0: EX0 = 0;
                        NowTotal = Nowtemp; Offset = Offtemp;
                        Filter = filtering;

                        if( Basic_A || Basic_AC || Basic_AR || Basic_ARC )
                            item = 2;
                        else
                            item = 4;
                        break;

#elif   Meter == ANALOG_FLOW
                case 1: ET0 = 0;
                        if( Basic_A || Basic_AC || Basic_AR || Basic_ARC )
                            item = 2;
                        else
                            item = 4;
                        break;
#endif
                case 2: if( Basic_A || Basic_AC || Basic_AR || Basic_ARC )
                            item = 2;
                        else
                            item = 4;
                        break;

                case 3: Zero = Nowtemp;     Nowtemp = Span;
                        break;

                case 4: Span = Nowtemp;     delay( 1500 );
                        DA( Zero );         delay( 2500 );
                        break;
                case 5:
                case 6:
                        status = 0;
                        RLY1_OFF;           RLY2_OFF;
                        break;
            }
            item++;
            if( item == 3 )                 Nowtemp = Zero;
            else if( item == 8 )
            {
#if     Meter == PULSE_FLOW
                DisTotal = NowTotal;
#endif
                break;
            }
            key_ready( Enter );
        }
    }
}

void NVRAM_combined()
{
    ID[0] = ID_A;                           ID[1] = ID_B;

#if     Meter == ANALOG_FLOW
    Cutvalue = CV_A;
    Cutvalue <<= 8;                         Cutvalue += CV_B;
#endif
}

void Alert( Char ch )
{
#pragma memory = code

    Char ERR_MSG[][21] = { " FLOW SIGNAL ERROR  ",      /* 0 */
                           "  TEMP. RANGE-OVER  ",      /* 1 */
                           " CHECK OVERRUN MODE ",      /* 2 */
                           " CHECK AUTO RESTART ",      /* 3 */
                           "  CHECK AUTO RESET  ",      /* 4 */
                           "  INTER-LOCK ERROR  ",      /* 5 */
                           "      RTD FAULT     ",      /* 6 */
                           " TEMP. SIGNAL ERROR ",      /* 7 */
                           "                    " };    /* 8 */
#pragma memory = default

    LCD_pos( 2, 0 );                        LCD_puts( ERR_MSG[ch] );
}

void Set_Menu(void)
{
#pragma memory = code

    Char SetMenu[][21] = { "   FLOW PARAMETER   ",        /* 0 */
                           "  BATCH PARAMETER   ",        /* 1 */
                           "       OPTION       ",        /* 2 */
                           "        TEST        ",        /* 3 */
                           "        EXIT        " };      /* 4 */
#pragma memory = xdata

    Char view=0, item=0;

#pragma memory = default

    LCD_pos( 1, 0 );                        LCD_puts( MODEL );

    for( view=0; view<6 ; view++ )
    {
        LCD_pos( 2, 0 );
        if( view % 2 )                      LCD_puts( VERSION );
        else                                LCD_puts( BLANK );

        delay( 25000 );                     delay( 25000 );
    }

    while( N_kEnter );                      beep();
    key_ready( Enter );
    LCD_clear();

    while( 1 )
    {
        if( view )
        {
            LCD_pos( 1, 0 );                view = 0;
            LCD_puts( SetMenu[item] );
            LCD_pos( 2, 0 );
            if( item == 4 )                 LCD_puts( "    PROGRAM MENU    " );
            else                            LCD_puts( BLANK );
        }

        if( kUp )
        {
            beep();                         view = 1;
            item++;
            if( ( Basic || Basic_R ) && item == 2 )
                item++;
            item %= 5;
            key_ready( Up );
        }
        else if( kEnter )
        {
            beep();                         view = 1;

            if( item == 4 )
            {
                LCD_clear();
                key_ready( Enter );

                if( (!Over_flag||!Op_mode||Op_mode == 2) && !EndTime )
                {
                    DisTimer = 1;

                    while( N_kEnter )
                    {
                        if( DisTimer % 2 )
                        {
                            if( !Over_flag )
                                Alert( 2 );
                            else if( !Op_mode )
                                Alert( 3 );
                            else
                                Alert( 4 );

                            Buzz_ON;
                        }
                        else
                        {
                            Alert( 8 );     Buzz_OFF;
                        }

                        if( (R_Reset) || R_IReset )                break;
                    }

                    beep();
                    DisTimer = 0;           Set_Batch();
                }
                else
                {
                    DisEn = 0;              break;
                }
            }
            switch( item )
            {
                case 0: Set_Flow();         break;
                case 1: Set_Batch();        break;
                case 2: Set_Option();       break;
                case 3: Set_Test();         break;
            }
            item = 4;                       set_ini();
            key_ready( Enter );
        }
    }
}

void sys_ini(void)
{
	Buzz_OFF;                               
	RLY1_OFF;                               
	RLY2_OFF;

    Buzz_OFF;                               
	RLY1_OFF;                               
	RLY2_OFF;

    /*
	ErrPulse_H;
    OutPulse_H;                             
	PulseFlag = 1;
    */ /* If flow not use */
    AUXR |= 0x0c; 

    TXE = 0;         TXE = 0;
    SWD = 1;         SWD = 1;                           
	DIS0 = 0;        DIS0 = 0;

    /******************************************************
    *                                                     *
    *   WATCHDOG TIMER USING                              *
	*   CPU : AT89C51ED2                                  *
	*   OSC : 60.000 MHz                                  *
	*   Watchdog interval : 0.419 sec                     *
    *   WDTPRG : S2:1, S1:1, S0:1 (2^21-1) * (1/60MHz) * 12 *
    *                                                     *
    ******************************************************/
/*
	WDTPRG = 0x07;
 
	WDTRST = 0x1e;
	WDTRST = 0xe1;
*/
    /******************************************************
    *                                                     *
    *   T2 State : Timer / 16-bit Auto Reload             *
    *   System Oscillator : 60.0 MHz                    *
    *   Interval : 10ms                                   *
    *                                                     *
    ******************************************************/
/**  60MHz : 10ms  */
/*    TH2 = 0x3c;        
	TL2 = 0xb0;

    RCAP2H = 0x3c;     
	RCAP2L = 0xb0;                          */

/*	CKCON0 = 0x01;*/

	/******************************************************
	*													  *
    *   T2 State : Timer / 16-bit Auto Reload             *
    *   System Oscillator : 18.432 MHz                    *
    *   Interval : 10ms                                   *
	*													  *
    ******************************************************/
    TH2 = 0xc4;                             TL2 = 0x00;
    RCAP2H = 0xc4;                          RCAP2L = 0x00;

/*	TH2 = 0x88;							TH2 = 0x88;
	TL2 = 0x00;							TL2 = 0x00;
	RCAP2H = 0x88;						RCAP2H = 0x88;
	RCAP2L = 0x00;						RCAP2L = 0x00;*/

    T2CON = 0x04;                           /* TR2 Set */
    T2CON = 0x04;

    /******************************************************
    *                                                     *
    *   T0 State : Timer mode#1/gate control disable      *
    *   TCON SET : TR0 & EX0 Set / Edge Triger Mode       *
    *   System Oscillator : 60.0 MHz                    *
    *   Interval : 25ms                                   *
    *                                                     *
    ******************************************************/
	/*
    TH0 = 0x69;                             TL0 = 0xf4;
    TMOD = 0x01;                            TCON = 0x11;
  */
    /* D model 의 환경설정 */
/*    TH0 = 0x6a;       TH0 = 0x6a;                                                   
	TL0 = 0x00;       TL0 = 0x00;

    TMOD = 0x01;      TMOD = 0x01;                            
	TCON = 0x15;      TCON = 0x15;
*/
	    /******************************************************
	*													  *
    *   T0 State : Timer mode#1/gate control disable      *
    *   TCON SET : TR0 & EX0 Set / Edge Triger Mode       *
    *   System Oscillator : 18.432 MHz                    *
    *   Interval : 25ms                                   *
	*													  *
    ******************************************************/
	
    TH0 = 0x69;                             TL0 = 0xf4;
    TMOD = 0x01;                            TCON = 0x11;
	/*TCON = 0x00;*/
	TR0 = 0;
    

    /* CEX0 PCA interrupt use */
    /******************************************************
    *                                                     *
    *   PCA Interrupt Setting : CEX0 Input                *
    *   TCON SET : TR0 & EX0 Set / Edge Triger Mode       *
    *   System Oscillator : 50.000 MHz                    *
    *                                                     *
    *                                                     *
    ******************************************************/

/*
    CCAPM0 = 0x11;  *  CEX0 negative trigger interrupt capture mode *
	CCAPM1 = CCAPM2 = CCAPM3 = CCAPM4 =  0x00;

    CMOD = 0x03;    * CPS1:0   CPS0:1   Fph / 2 *
	CMOD = 0x03;


    IPH0 |= 0x40;
	IPL0 |= 0x40;
  */ 
    /******************************************************
    *                                                     *
    *       Interrupt Enable ( IE : 0xb3 )                *
    *   IE.7(EA)    : Global Interrupt Enable             *
    *   IE.5(ET2)   : Enable Timer 2 Interrupt            *
    *   IE.1(ET0)   : Enable Timer 0 Interrupt            *
    *   IE.0(EX0)   : Enable EXT Timer 0 Interrupt        *
    *                                                     *
    ******************************************************/
   /* IE = 0xb3; */

    /******************************************************
	*													  *
    *       Interrupt Enable ( IE : 0xa7 )                *
	*													  *
    *   IE.7(EA)  1 : Global Interrupt Enable             *
    *   IE.6(ES1) 0 : Enable Serial 1 Interrupt           *
    *   IE.5(ET2) 1 : Enable Timer 2 Interrupt            *
    *   IE.4(ES0) 1 : Enable Serial 0 Interrupt           *
    *   IE.3(ET1) 0 : Enable Timer 1 Interrupt            *
    *   IE.2(EX1) 1 : Enable EXT#1 Interrupt              *
    *   IE.1(ET0) 1 : Enable Timer 0 Interrupt            *
    *   IE.0(EX0) 1 : Enable EXT#0 Interrupt              *
	*													  *
    ******************************************************/
	/* D model 의 환경설정 */
    IE = 0xb3;
/*	EX0 = 1;*/
/*	EPCA = 1; 



	PCA_ON; *//*CR = 1;   PCA Interrupt run */

	delay(30000);
	delay(30000);
	delay(30000);
    
    RTC[10] = 0x20;        /* Register A , BCD format   */
    RTC[11] = 0x02;        /* Register B                */
    /******************************************************
    *             Pulse Output Value Initial              *
    ******************************************************/
    OldTotal = GrossTotal;

#if     Meter == PULSE_FLOW
    ET0 = 0;
#elif   Meter == ANALOG_FLOW
    EX0 = 0;
#endif

    /******************************************************
    *          Communication Baud-Rate Setting            *
    ******************************************************/
    serial_ini( Baud );                     delay( 600 );

    /******************************************************
    *              Communication Buffer Clear              *
    ******************************************************/
    memset( &Buf, 0x00, sizeof( ComQueue ));
    Buf.Tail = 0;       Buf.Head = 0;       Buf.Empty = 0;

    /******************************************************
    *              Detection  System  varibles              *
    ******************************************************/
    Total_unit %= 11;                       TimeBase %= 4;
    Total_DP %= 4;                          Rate_DP %= 4;
    PrintMode %= 2;                         CommType %= 2;
    Inter_use %= 2;

#if     Compensation == LIQUID
    Temp_unit %= 2;
#endif
}

void Flow_Calculation( void )
{
#pragma memory = data

#if     Meter == PULSE_FLOW
    Int     sub = 0;
#elif   Meter == ANALOG_FLOW
    Int     ADTotal = 0;
#endif

    Char    i = 0;
    Int     k = 0;
    float   j = 0;

#pragma memory = default

    /***************************************************************
    *                   PULSE TYPE FLOW SIGNAL                     *
    ***************************************************************/

#if     Meter == PULSE_FLOW

    EX0 = 0;                                k = NowTotal;
    EX0 = 1;

    if( k > DisTotal )
    {
        sub = k - DisTotal;                 DisTotal += sub;
    }
    else if( DisTotal > k )
    {
        sub = 65536-DisTotal+k;             DisTotal = k;
    }
    else                                    sub = 0;

    if( sub )
    {
        j = sub * Total_Fac;                GrossRemain += j;
        Total = GrossRemain;                GrossRemain -= Total;
        GrossTotal += Total;                GrossTotal %= 10000000;
    }

    /***************************************************************
    *                   ANALOG TYPE FLOW SIGNAL                    *
    ***************************************************************/

#elif   Meter == ANALOG_FLOW

    ADTotal = ad_converter( FLOW1 );

    /************ Flow Signal Error Cut Point ( 3.5mA ) ***********/
    if( ADTotal < 715 && Warning )          f_err = 1;
    else                                    f_err = 0;

    if( ADTotal <= CutConst )               Flowrate = 0;
    else
    {
		T = 0;
        if( ADTotal >= MAX )                ANL = 1;
        else
        {
            ANL = ADTotal - MIN;            ANL /= RANGE;
        }

        if( !Correct )
            Flowrate = Input_Span * ANL + Input_Zero;
        else
            Flowrate = Input_Span * sqrt(ANL) + Input_Zero;
    }

    TR0 = 0;
    j = AnlTotal;                           AnlTotal = 0;
    TR0 = 1;

    GrossRemain += j;
    Total = GrossRemain;                    GrossRemain -= Total;
    GrossTotal += Total;                    GrossTotal %= 10000000;

#endif

    /***************************************************************
    *                       Rate Caculation                        *
    ***************************************************************/
    RATE = 0;
    for( i=0 ; i<Filter ; i++ )             RATE += Rate[i];

#if     Meter == PULSE_FLOW

    DisRate = ( RATE * Rate_Fac / Filter ) + RateHalf;
            
#elif   Meter == ANALOG_FLOW

    DisRate = ( RATE / Filter ) + RateHalf;

#endif
}

void main( void )
{
#pragma memory = xdata

    Char    dismode=0, com_enable=1, first=0, rly1_flag=0, rly2_flag=0;
    Char    end_flag=0, pause_flag = 0, comp_flag = 0, start_flag = 0,p = 0;
    Int     batch[4];
    Int     Sound = 0;
    Long    MidTotal = 0;
    Char    stop_flag = 0, Tfirst = 0;
    Char    i;
	Char   	tm1 = 0;
#pragma memory = data

    Int     k = 0;

#pragma memory = code

    Char DIS[][21]  =   {   "      ACC-TOTAL     ",
                            "  BATCH PRESET Qt.  " };

#pragma memory = default

    sys_ini();                              NVRAM_combined();
    set_ini();                              LCD_ini();
	
    /***************************************************************
    *                 Automatic Over Run Array Clear               *
    ***************************************************************/
    for( i=0 ; i<4 ; i++ )                  batch[i] = 0;

    /***************************************************************
    *               Auto-mode error check routine                  *
    ***************************************************************/
    if( ( !Over_flag || !Op_mode || Op_mode == 2 ) && !EndTime )
    {
        DisTimer = 1;

        while( N_kEnter )
        {
            if( DisTimer % 2 )
            {
                if( !Over_flag )            Alert( 2 );
                else if( !Op_mode )         Alert( 3 );
                else                        Alert( 4 );

                Buzz_ON;
            }
            else
            {
                Alert( 8 );                 Buzz_OFF;
            }

            if( (R_Reset) || R_IReset )                break;
        }
        beep();                             LCD_clear();

        delay( 30000 );                     delay( 30000 );
        key_ready( Enter );

        Set_Menu();
        key_ready( Enter );                 delay( 25000 );
        dismode = 0;                        DisEn = 0;
    }

    /***************************************************************
    *               Previous Batch Controller Status               *
    ***************************************************************/
    switch( BatchStatus )
    {
                /******** Auto Zero Function Check ********/
        case 0: if( Op_mode == 1 )            BatchVolume = 0;
                break;
        case 1:
        case 2:
        case 3:
        case 4: stop_flag = 1;              start_flag = 1;
                BatchStatus = 1;

                if( !GrossTotal )           pause_flag = 0;
                else
                {
                    if( BatchVolume > GrossTotal )
                    {
                        MidTotal = GrossTotal;
                        pause_flag = 1;
                    }
                }
                break;
        case 5: BatchStatus = 0;
                /******** Auto Zero Function Check ********/
                if( Op_mode == 1 )            BatchVolume = 0;
                break;
        case 6:
        case 7: stop_flag = 1;              start_flag = 1;
                BatchStatus = 1;

                if( !GrossTotal )           pause_flag = 0;
                else
                {
                    if( BatchVolume > GrossTotal )
                    {
                        MidTotal = GrossTotal;
                        pause_flag = 1;
                    }
                }
                break;
    }

    for(;;)
    {
        /***************************************************************
        *                     Auto Return Enable                       *
        ***************************************************************/
        if( dismode && AutoReturn && DisTimer >= 10 )
        {
            dismode = 0;                    LCD_clear();
        }

        /***************************************************************
        *               FLOW CALCULATION( TOTAL & RATE )               *
        ***************************************************************/
        Flow_Calculation();

        if( OldTotal >= 10000000 )
        {
            Pulse = 0;                      OldTotal %= 10000000;
            Pulse = 1;
        }

        /***************************************************************
        *                   Error Message Display                      *
        ***************************************************************/
        if( f_err || t_err || r_err )
        {
            error = 1;

            if( !Tfirst )
            {
                Tfirst = 1;

                RLY1_OFF;                   RLY2_OFF;
                rly1_flag = 0;              rly2_flag = 0;

                if( !GrossTotal )           pause_flag = 0;
                else
                {
                    if( BatchVolume > GrossTotal )
                    {
                        MidTotal = GrossTotal;
                        pause_flag = 1;
                    }
                }

                if( start_flag )            stop_flag = 1;
            }

            Sound++;                        Sound %= 800;

            if( Sound == 1 )
            {
                if( r_err )
                {
#if     Compensation == LIQUID

#if     TempType == MA
                    Alert( 7 );

#elif   TempType == RTD
                    Alert( 6 );
#endif

#endif
                }
                else if( t_err )            Alert( 1 );
                else if( f_err )            Alert( 0 );

                if( FSound )                Buzz_ON;
            }
            else if( Sound == 400 )
            {
                Alert( 8 );                 Buzz_OFF;
            }
        }
        else
        {
            if( error )
            {
                Alert( 8 );                 Buzz_OFF;
                error = 0;                  Tfirst = 0;
                FSound = 1;
            }
        }

        /***************************************************************
        *                   Batch Process Proceeding                   *
        ***************************************************************/
        if( rly2_flag )
        {
            /*******************************************************
            *                     RLY1 Prestart                    *
            *******************************************************/
            if( !rly1_flag && (BatchVolume-PrestopQ) > GrossTotal
                           && (BatchVolume-OverQ) > GrossTotal )
            {
                if( !Start_mode )           /* Prestart mode is Time */
                {
                    if( Timer >= PrestartTime )
                    {
                        BatchStatus = 4;
                        RLY1_ON;            rly1_flag = 1;
                    }
                }
                else                        /* Prestart mode is Quantity */
                {
                    if( ( !pause_flag && GrossTotal > PrestartQ )
                        || ( pause_flag && (GrossTotal-MidTotal) > PrestartQ ) )
                    {
                        BatchStatus = 4; 
                        RLY1_ON;            rly1_flag = 1;
                    }
                }
            }

            /*******************************************************
            *                     RLY1 Prestop                     *
            *******************************************************/
            if( rly1_flag && (BatchVolume-GrossTotal) <= PrestopQ )
            {
                BatchStatus = 3; 
                RLY1_OFF;                   rly1_flag = 0;
            }

            /*******************************************************
            *               Batch End or Manual Stop               *
            *******************************************************/
            if( kDown || R_Stop || R_IStop || Change_Status == 2
                || (BatchVolume-OverQ) <= GrossTotal )
            {
                if( kDown || R_Stop || R_IStop || Change_Status == 2 )
                {
                    Change_Status = 0;
                    stop_flag = 1;          BatchStatus = 1;
                }
                else 					BatchStatus = 5;
				

                beep();

                RLY1_OFF;                   RLY2_OFF;
                rly1_flag = 0;              rly2_flag = 0;

				com_enable = 1;

                if( !GrossTotal )           pause_flag = 0;
                else
                {
                    if( BatchVolume > GrossTotal )
                    {
                        MidTotal = GrossTotal;
                        pause_flag = 1;
                    }
                }

                key_ready( Down );          remote_ready( Stop );
				remote_ready( IStop );
            }

            /*******************************************************
            *                   Flow Error Check                   *
            *******************************************************/
            if( OutTime && T >= OutTime )
            {
                BatchStatus = 6;            /* Flow Alarm detected */

                RLY1_OFF;                   RLY2_OFF;
                rly1_flag = 0;              rly2_flag = 0;

                if( !GrossTotal )           pause_flag = 0;
                else
                {
                    if( BatchVolume > GrossTotal )
                    {
                        MidTotal = GrossTotal;
                        pause_flag = 1;
                    }
                }

                stop_flag = 1;              DisTimer = 1;
                Pulse = 0;

                while( N_kEnter && !(R_Reset || R_IReset) )
                {
                    Flow_Calculation();

                    communication();

                    if( Change_Status == 4 )
                    {
                        Change_Status = 0;    break;
                    }

                    if( DisTimer % 2 )
                    {
                        Alert( 0 );         Buzz_ON;
                    }
                    else
                    {
                        Alert( 8 );         Buzz_OFF;
                    }
                }

                Buzz_OFF;
                BatchStatus = 1;            /* Status is Paused */

                LCD_pos( 2, 0 );            LCD_puts( BLANK );

                beep();

                DisTimer = 0;               DisEn = 0;
                key_ready( Enter );         remote_ready( Reset );
				remote_ready( IReset );
            }
        }

        /***************************************************************
        *                    After Batch Completion                    *
        ***************************************************************/
        if( start_flag && GrossTotal >= ( BatchVolume - OverQ )
            && ( !EndTime || T > EndTime ) )
        {
            /*******************************************************
            *                End of Batch Process                  *
            *******************************************************/
            EndPulse_L;                     end_flag = 1;

            if( !first )
            {
                first = 1;                  start_flag = 0;
                comp_flag = 1;              pause_flag = 0;
                AutoTimer = 0;              com_enable = 1;
                dismode = 0;                DisEn = 0;
                LCD_clear();                BatchStatus = 0;
            }

            /*******************************************************
            *           Overrun Q'ty Automatic Compensation        *
            *******************************************************/
            if( !Over_flag && EndTime && comp_flag )
            {
                batch[p++] = GrossTotal - ( BatchVolume - OverQ );

                OverQ = (batch[0] + batch[1] + batch[2] + batch[3]) / 4. + 0.5;
                p %= 4;                     comp_flag = 0;
            }

            /*******************************************************
            *                    Auto Reset Enable                 *
            *******************************************************/
            if( Op_mode == 2 && EndTime )
            {
                EX0 = 0;                    ET0 = 0;
                ET2 = 0;                    OutPulse_H;

                ResetValue( 1 );            ET2 = 1;

                dismode = 0;                stop_flag = 0;
                LCD_clear();

#if     Meter == PULSE_FLOW
                IE0 = 0;                    EX0 = 1;
#elif   Meter == ANALOG_FLOW
                ET0 = 1;
#endif
            }
        }

        /***************************************************************
        *                   Auto Re-Start Mode Enable                  *
        ***************************************************************/
        if( !Op_mode && AutoStart )
        {
            if( end_flag && !start_flag && AutoTimer >= AutoStart && EndTime )
            {
                EX0 = 0;                    ET0 = 0;
                ET2 = 0;                    OutPulse_H;
                ResetValue( 1 );            ET2 = 1;

#if     Meter == PULSE_FLOW
                IE0 = 0;                    EX0 = 1;
#elif   Meter == ANALOG_FLOW
                ET0 = 1;
#endif
                if( ( !Start_mode && !PrestartTime )
                                  || ( Start_mode && !PrestartQ ) )
                {
                    BatchStatus = 4; 
                    RLY1_ON;                RLY2_ON;
                    rly1_flag = 1;          rly2_flag = 1;
                }
                else
                {
                    BatchStatus = 2;        /* SV Valve On */
                    RLY2_ON;                rly2_flag = 1;
                }

                first = 0;                  stop_flag = 0;
                EndPulse_H;                 end_flag = 0;
                start_flag = 1;

                T = 0;                      Timer = 0;
            }
        }

        /***************************************************************
        *               Process Stop // Main Program Setting           *
        ***************************************************************/
        if( kDown && !rly2_flag && !stop_flag && !end_flag && !start_flag )
        {
            beep();                         k = 0;

            while( kDown && k < 15000 )     k++;

            if( kDown && k > 14999 )
            {
                EX0 = 0;                    ET0 = 0;
                OldTotal = GrossTotal;      beep();

                LCD_pos( 1, 0 );            LCD_puts( MODEL );
                LCD_pos( 2, 0 );            Option_check();

                key_ready( Down );

                DA( Zero );                 delay( 600 );
                Pulse = 0;

                Set_Menu();                 com_enable = 1;

                dismode = 0;                DisEn = 0;
                DisRate = 0;

                EndPulse_H;                 Pulse = 1;

                for( i=0 ; i<Filter ; i++ ) Rate[i] = 0;
                for( i=0 ; i<4 ; i++ )      batch[i] = 0;

                /*******************************************************
                *                    Auto Zero Set                     *
                *******************************************************/
                if( Op_mode == 1 )          BatchVolume = 0;

#if     Meter == PULSE_FLOW
                IE0 = 0;                    EX0 = 1;
#elif   Meter == ANALOG_FLOW
                ET0 = 1;
#endif
                key_ready( Enter );
            }
        }

        /***************************************************************
        *                        Batch Start                           *
        ***************************************************************/
        else if( (kUp || R_Start || R_IStart || Change_Status == 1 ) && !rly2_flag )
        {
            Change_Status = 0;
			

            if( ( (!(Basic_NR) && (!(NR_Interlock) || R_IStart || !Inter_use )) || Basic_NR )
                &&  ( ( ( BatchVolume - OverQ ) > GrossTotal ) || ResetMethod )
                && BatchVolume && !error )
            {

				
				

                if( !start_flag && GrossTotal && !ResetMethod )     bell();
                else
                {
                    beep();
                    if( ResetMethod )
					{
    					if( !rly2_flag && !error )      /* line process stop only */
                        {
                            beep();
    
                            EX0 = 0;                    ET0 = 0;
                            ET2 = 0;                    OutPulse_H;
                            ResetValue( 1 );            ET2 = 1;
                            BatchStatus = 0;

                            /*******************************************************
                            *                    Auto Zero Set                     *
                            *******************************************************/
                            if( Op_mode == 1 && end_flag )
                                BatchVolume = 0;

                            EndPulse_H;                 end_flag = 0;
     
                            dismode = 0;                DisEn = 0;
                            pause_flag = 0;             start_flag = 0;
                            LCD_clear();                stop_flag = 0;

#if     Meter == PULSE_FLOW
                            IE0 = 0;                    EX0 = 1;
#elif   Meter == ANALOG_FLOW
                            ET0 = 1;
#endif
             
                        }
                    }

                    start_flag = 1;         first = 0;
                    T = 0;                  Timer = 0;

                    if( (BatchVolume-OverQ) > GrossTotal
                                && (BatchVolume-PrestopQ) > GrossTotal )
                    {
                        if( ( !Start_mode && !PrestartTime )
                                          || ( Start_mode && !PrestartQ ) )
                        {
                            BatchStatus = 4; 
                            RLY1_ON;        RLY2_ON;
                            rly1_flag = 1;  rly2_flag = 1;
                        }
                        else
                        {
                            BatchStatus = 2;
                            RLY2_ON;        rly2_flag = 1;
                        }
                    }
                    else
                    {
                        BatchStatus = 2; 
                        RLY2_ON;            rly2_flag = 1;
                    }

                    EndPulse_H;             end_flag = 0;
                    key_ready( Up );        remote_ready( Start );
					remote_ready( IStart );
                }
            }
            else                            bell();
        }

        /***************************************************************
        *                     Batch Volume Setting                     *
        ***************************************************************/
        else if( kShift )
        {
            if( !rly2_flag && !end_flag && !stop_flag && !start_flag )
            {
                beep();
                EX0 = 0;                    ET0 = 0;
                OutPulse_H;
                Timer = 0;                  AutoTimer = 0;

                LCD_pos( 1, 0 );            LCD_puts( " BATCH QUANTITY SET " );
                LCD_pos( 2, 0 );            LCD_puts( BLANK );
                key_ready( Shift );
                delay( 30000 );             delay( 30000 );
                delay( 30000 );

                Parameter_batch( 4 );       ResetValue( 1 );

#if     Meter == PULSE_FLOW
                IE0 = 0;                    EX0 = 1;
#elif   Meter == ANALOG_FLOW
                ET0 = 1;
#endif
                for( i=0 ; i<4 ; i++ )      batch[i] = 0;

                LCD_clear();

                first = 0;                  delay( 500 );
                dismode = 0;                DisEn = 0;
                T = 0;                      Timer = 0;
                key_ready( Enter );
            }
        }

        /***************************************************************
        *                     Display Mode Selection                   *
        ***************************************************************/
        else if( kMode )
        {
            beep();

            if( error )                     FSound = 0;

            dismode++;                      dismode %= 3;
            DisEn = 0;                      DisTimer = 0;
            key_ready( Mode );              LCD_clear();
        }

        /***************************************************************
        *                       GrossTotal Reset                       *
        ***************************************************************/
        else if( kEnter || R_Reset || R_IReset || Change_Status == 3 )
        {
            Change_Status = 0;

            if( !rly2_flag && !error )      /* line process stop only */
            {
                beep();

                EX0 = 0;                    ET0 = 0;
                ET2 = 0;                    OutPulse_H;
                ResetValue( 1 );            ET2 = 1;
                BatchStatus = 0;

                /*******************************************************
                *                    Auto Zero Set                     *
                *******************************************************/
                if( Op_mode == 1 && end_flag )
                    BatchVolume = 0;

                EndPulse_H;                 end_flag = 0;

                dismode = 0;                DisEn = 0;
                pause_flag = 0;             start_flag = 0;
                LCD_clear();                stop_flag = 0;

#if     Meter == PULSE_FLOW
                IE0 = 0;                    EX0 = 1;
#elif   Meter == ANALOG_FLOW
                ET0 = 1;
#endif
                key_ready( Enter );         remote_ready( Reset );
                remote_ready( IReset );
            }
        }

        /***************************************************************
        *                        Check InterLock                        *
        ***************************************************************/
        if( !(Basic_NR) && start_flag &&  NR_Interlock && Inter_use /*&& !R_IStart && !R_IStop && !R_IReset*/ )
        {
            beep();

            
            RLY2_OFF;                       delay( 10 );
            RLY1_OFF;                       delay( 10 );
            rly1_flag = 0;                  rly2_flag = 0;

            if( !GrossTotal )               pause_flag = 0;
            else
            {
                if( BatchVolume > GrossTotal )
                {
                    MidTotal = GrossTotal;
                    pause_flag = 1;
                }
            }

            LCD_pos( 1, 0 );                LCD_puts( BLANK );

            DisTimer = 1;                   stop_flag = 1;

            BatchStatus = 7;                /* Interlock Alarm detected */

            while( N_kEnter && !R_Reset && !R_IReset )
            {
                Flow_Calculation();

                communication();

                if( Change_Status == 4 )
                {
                    Change_Status = 0;        break;
                }

                if( DisTimer % 2 )
                {
                    Alert( 5 );             Buzz_ON;
                }
                else
                {
                    Alert( 8 );             Buzz_OFF;
                }
            }

            Buzz_OFF;
            BatchStatus = 1;                /* Status is Pause */

            LCD_pos( 2, 0 );                LCD_puts( BLANK );
            beep();
            DisTimer = 0;                   DisEn = 0;
            OutPulse_H;

            DisRate = 0;
            for( i=0 ; i<Filter ; i++ )     Rate[i] = 0;

            key_ready( Enter );             remote_ready( Reset );
			remote_ready( IReset );
        }

		
        /***************************************************************
        *                  Display Batch Total / Rate                  *
        ***************************************************************/
        if( !DisEn )
        {
            switch( dismode )
            {
                case 0:
                    LCD_pos( 1, 0 );        LCD_puts( "PRESET" );
                    LCD_pos( 1, 7 );
                    LCD_int( Total_DP, PLUS, BatchVolume, 7 );

                    if( Total_unit != 10 )
                        LCD_puts( *(Totaldis + Total_unit) );


                    if( !error && !(BatchStatus ==2 || BatchStatus ==4 || BatchStatus == 3 || BatchStatus == 5) && !BatchStatus && !end_flag)
                    {
                        LCD_pos( 2, 0 );    LCD_puts( "BATCH" );
                        LCD_pos( 2, 7 );

                        if( Count_flag )
                        {
                            if( BatchVolume >= GrossTotal )
                                LCD_int( Total_DP, PLUS,BatchVolume-GrossTotal, 7 );
                            else
                                LCD_int( Total_DP, MINUS, GrossTotal-BatchVolume,7 );
                        }
                        else
                            LCD_int( Total_DP, PLUS, GrossTotal, 7 );

                        if( Total_unit != 10 )
                            LCD_puts( *( Totaldis + Total_unit ) );
                    }
					tm = tm % 2;
					if( (BatchStatus == 2 || BatchStatus == 4 || BatchStatus == 3) /*&& tm1*/)
					{
						
						LCD_pos( 2, 7 );
						if( Count_flag )
                        {
                            if( BatchVolume >= GrossTotal )
                                LCD_int( Total_DP, PLUS,BatchVolume-GrossTotal, 7 );
                            else
                                LCD_int( Total_DP, MINUS, GrossTotal-BatchVolume,7 );
                        }
                        else
                            LCD_int( Total_DP, PLUS, GrossTotal, 7 );

                        if( Total_unit != 10 )
                            LCD_puts( *( Totaldis + Total_unit ) );
						/*LCD_pos( 2, 0 );
						LCD_puts( "     " );*/
						if( tm )
						{
							LCD_pos( 2, 0 );
							LCD_puts( "RUN  " );
						}
						else
						{
							LCD_pos( 2, 0 );
							LCD_puts( "     " );
						}
					
					}
					
					if( BatchStatus == 1)
					{
						
						LCD_pos( 2, 7 );
						if( Count_flag )
                        {
                            if( BatchVolume >= GrossTotal )
                                LCD_int( Total_DP, PLUS,BatchVolume-GrossTotal, 7 );
                            else
                                LCD_int( Total_DP, MINUS, GrossTotal-BatchVolume,7 );
                        }
                        else
                            LCD_int( Total_DP, PLUS, GrossTotal, 7 );

                        if( Total_unit != 10 )
                            LCD_puts( *( Totaldis + Total_unit ) );
						if( tm)
						{
							LCD_pos( 2, 0 );
							LCD_puts( "STOP " );
						}
						else
						{
							LCD_pos( 2, 0 );
							LCD_puts( "     " );
						}
					}
					if( end_flag || BatchStatus == 5)
					{						
						LCD_pos( 2, 7 );
						if( Count_flag )
                        {
                            if( BatchVolume >= GrossTotal )
                                LCD_int( Total_DP, PLUS,BatchVolume-GrossTotal, 7 );
                            else
                                LCD_int( Total_DP, MINUS, GrossTotal-BatchVolume,7 );
                        }
                        else
                            LCD_int( Total_DP, PLUS, GrossTotal, 7 );

                        if( Total_unit != 10 )
                            LCD_puts( *( Totaldis + Total_unit ) );
						LCD_pos( 2, 0 );
						LCD_puts( "STOP " );
					}					
                    break;

                case 1:
                    LCD_pos( 1, 0 );        LCD_puts( DIS[0] );

                    if( !error )
                    {
                        LCD_pos( 2, 6 );
                        LCD_int( Total_DP, PLUS, GrossAccTotal+GrossTotal, 7 );

                        if( Total_unit != 10 )
                           LCD_puts( *(Totaldis + Total_unit) );
                    }
                    break;

                case 2:

                    LCD_pos( 1, 0 );        LCD_puts( "     FLOW RATE      " );
                    LCD_pos( 2, 6 );
                    LCD_int( Rate_DP, PLUS, DisRate, 7 );

                    if( Total_unit != 10 )
                    {
                        LCD_puts( *( Totaldis + Total_unit ) );
                        LCD_puts( *( Rateunit + TimeBase ) );
                    }

                    break;
            }
		
            DisEn = 1;
        }

        /***************************************************************
        *                   Printing & Serial Communication            *
        ***************************************************************/
        if( Basic_C || Basic_AC || Basic_RC || Basic_ARC )
        {   
            if( CommType )                  /* LOGGING BY PRINTER */
            {
                if( !PrintMode && com_enable )
                {                                             
                    EX0 = 0;    ET0 = 0;

                    Printing();

#if     Meter == PULSE_FLOW
                    EX0 = 1;
#elif   Meter == ANALOG_FLOW
                    ET0 = 1;
#endif
                    com_enable = 0;
                }
            }    
            else                            /* LOGGING BY COMPUTER */
                communication();
        }                                                                   

        /***************************************************************
        *                        Analog Output                         *
        ***************************************************************/
        if( Basic_A || Basic_AC || Basic_AR || Basic_ARC )
        {
            if( DisRate < ZeroAlarm )       DA( Zero );
            else if( DisRate > SpanAlarm )  DA( Span );
            else
                DA( ( DisRate - ZeroAlarm ) * Analog + Zero );
        }
    }
}

        /***************************************************************
        *                        Interrupt Routine                     *
        ***************************************************************/

#if     Meter == PULSE_FLOW

interrupt [0x03] void EX0_int ( void )
{
    NowTotal++;                             T = 0;
}
#endif


/*
interrupt [0x0B] void T0_int ( void )       * 25msec interval *
{
    TH0 = 0x6a;                             TL0 = 0x1a;

   * DisCount++;                             DisCount %= 40;
	DC++;									DC %= 20;*
#elif   Meter == ANALOG_FLOW
    AnlTotal += Flowrate * Total_Fac;
    RateTotal += Flowrate * Rate_Fac;

    if( !DisCount )
    {
        Rate[FilterCounter] = RateTotal + RateHalf;
        RateTotal = 0;                      Timer++;
        FilterCounter++;                    FilterCounter %= Filter;
        DisTimer++;							
        T++;                                AutoTimer++;
    }
#endif
}
*/


interrupt [0x2B] void T2_int ( void )       /* 10msec interval */
{
    TF2 = 0;

    if( DisEn )
    {
        DisEn++;                            DisEn %= 26;
    }	

   /* if( WDT )                               WDT = 0;
    else                                    WDT = 1;*/

#if     Meter == PULSE_FLOW
    Disc++;                             Disc %= 100;
   /* DC++;									DC %= 50;*/

    if( !Disc )
    {
        TempTotal = NowTotal;

        if( TempTotal > Offset )
            Rate[FilterCounter] = TempTotal - Offset;
        else if( Offset > TempTotal )
            Rate[FilterCounter] = 65536 - Offset + TempTotal;
        else                                Rate[FilterCounter] = 0;

        Offset = TempTotal;
        FilterCounter++;                    FilterCounter %= Filter;
        Timer++;                            DisTimer++;
        T++;                                AutoTimer++;
		tm++;
		intercheck++;		intercheck %= 10000;
    }	
#endif

    if( Pulse )
    {
        if( OldTotal == GrossTotal )        PulseFlag = 1;
        else
        {
            if( !PulseFlag )
            {
                OldTotal++;
                OutPulse_H;                 PulseFlag = 1;
            }
            else
            {
                OutPulse_L;                 PulseFlag = 0;
            }
        }
    }
	
}

interrupt [0x23] void SCON0_int (void)      /* Serial Port */
{
    Buf.Buff[Buf.Head++] = serial_getc();   
	Buf.Empty = 1;
    Buf.Head %= BUFSIZE;
}
