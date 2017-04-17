/**********************************************************
*       CPU         : DALAS 80C320                        *
*       Object      : 410 LCD sub-routine                 *
*       Display     : LCD( 20 Charactor * 2 line )        *
*       Date        : 1999. 07. 15.                       *
*       Version     : 1.00                                *
**********************************************************/

/*--------   LCD Display  ----------*/

#define     OFF                     0
#define     ON                      1
#define     BLANK                   "                    "

#pragma memory = data
Char        LCDpos;

#pragma memory = default

void Enable_Pulse(void)
{                                           /* enable pulse */
    DIS0 = 0;                               
    DIS0 = 0;                               
	DIS0 = 1;
	DIS0 = 1;
    delay( 20 );                            
	DIS0 = 0;
	DIS0 = 0;
}

void sync_data( unsigned char value )
{
    Char i;                         /* Transferring serial data to parallel */
                                    /* bus at 4094B for driving LCD */
    DIS1 = 0;                       /* RS Pin */
    DIS1 = 0;

    for( i=0 ; i<8 ; i++ )
    {
        CLK = 0;
		CLK = 0;

        if( value & 0x80 )     {  DI = 1; DI = 1; }
        else                   {  DI = 0; DI = 0; }

        CLK = 1;
		CLK = 1;
        value <<= 1;
    }
    Enable_Pulse();
}

void LCD_clear(void)
{
    sync_data( 0x01 );
    delay( 500 );
}

void LCD_cursorblink( unsigned char state )
{
    if( state )             sync_data( 0x0f );  /* cursor blink */
    else                    sync_data( 0x0c );  /* cursor off */

    delay( 500 );
}

void LCD_putc( unsigned char chr )
{
    Char i;

    DIS1 = 1;
	DIS1 = 1;

    for( i=0 ; i<8 ; i++ )
    {
        CLK = 0;
		CLK = 0;

        if( chr & 0x80 )       {  DI = 1;  DI = 1; }
        else                   {  DI = 0;  DI = 0; }

        CLK = 1;
        CLK = 1; 
        chr <<= 1;
    }
    Enable_Pulse();                         

	DIS1 = 0;
	DIS1 = 0;
}

void LCD_puts( unsigned char *s )
{
    while( *s )
        LCD_putc( *s++ );
}

void LCD_pos( unsigned char line, unsigned char digit )
{
    switch( line )
    {
        case 1 :    LCDpos = 0x80 + digit;
                    break;
        case 2 :    LCDpos = 0xc0 + digit;
                    break;
    }
    sync_data( LCDpos );                    
	delay( 500 );
}

#if     DisMode == VFD_DISPLAY
void VFD_adjust( char mode )        /* brightness adjustment */
{
    switch( mode%4 )
    {
		case 0: 	mode = 0x38;	break;		/* 100 % */
		case 1: 	mode = 0x3b;	break;		/*	25 % */
		case 2: 	mode = 0x3a;	break;		/*	50 % */
		case 3: 	mode = 0x39;	break;		/*	75 % */
    }
    sync_data( mode );              
	delay( 10000 );
}
#endif

void LCD_ini(void)
{
#pragma memory = code

#if     DisMode == VFD_DISPLAY

    char    degree[8] = { 0x1c, 0x14, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00 };

#elif   DisMode == LCD_DISPLAY

    char    two[8]    = { 0x0c, 0x12, 0x04, 0x08, 0x1e, 0x00, 0x00, 0x00 };
    char    three[8]  = { 0x1c, 0x02, 0x1c, 0x02, 0x1c, 0x00, 0x00, 0x00 };

#endif

#pragma memory = xdata

    Char    i;

#pragma memory = default



    delay( 30000 );

    sync_data( 0x30 );      delay( 1000 );      /* function set */
    sync_data( 0x30 );      delay( 500 );       /* function set */
    sync_data( 0x30 );      delay( 200 );       /* function set */

    sync_data( 0x38 );      delay( 500 );       /* 8 bit, 1/16 duty, 5x7 dot */
    sync_data( 0x0c );      delay( 500 );       /* display on */
    sync_data( 0x01 );      delay( 1000 );      /* dis-clear & cursor home */
    sync_data( 0x06 );      delay( 500 );       /* cursor shift & dis-shift inhibited  */

    delay( 50000 );

#if     DisMode == VFD_DISPLAY

    VFD_adjust( VFD_mode );                     /* brightness adjustment */

    sync_data( 0x48 );      delay( 500 );       /* CGRAM Address 0x01 */
    for( i=0 ; i<8 ; i++ )
        LCD_putc( degree[i] );                  /* degree font */

#elif   DisMode == LCD_DISPLAY

    sync_data( 0x48 );      delay( 500 );       /* CGRAM Address 0x01 */
    for( i=0 ; i<8 ; i++ )
        LCD_putc( two[i] );                     /* "2" font */

    sync_data( 0x50 );      delay( 500 );       /* CGRAM Address 0x02 */
    for( i=0 ; i<8 ; i++ )
        LCD_putc( three[i] );                   /* "3" font */

#endif
}

void LCD_int( char dp, char sign, Long value, char digit )
{
    Long a = 1;
    unsigned char ch, u = 1, s = 1, dec, i;

    for( i = 0 ; i < digit ; i++ )          a *= 10;

    value %= a;                             dec = digit - 1;

    if( dp || !sign )
    {
        if( dp )                            LCDpos--;
        if( !sign )                         LCDpos--;
        sync_data( LCDpos );                delay( 50 );
    }

    for( i=0 ; i<digit ; i++ )
    {
        a /= 10;                            ch = value / a;

        if(( digit - dp ) == i )
        {
            LCD_putc( '.' );                delay( 30 );
        }

        if( u && !ch && dec-dp >i )         ch = ' ';
        else
        {
            if( !sign && s )
            {
                s = 0;
                LCD_putc( '-' );            delay( 30 );
            }
            u = 0;                          ch += '0';
        }

        LCD_putc( ch );                     delay( 30 );
        value %= a;
    }
}
/*
void Option_check( void )
{
    switch( ext_key_load() & 0x7f )
    {
        case 0x7f  : LCD_puts( "    BASIC OPTION    " );     break;
        case 0x3f  : LCD_puts( "   ANALOG OPTION    " );     break;
        case 0x5f  : LCD_puts( "   REMOTE OPTION    " );     break;
        case 0x4f  : LCD_puts( "    COMM. OPTION    " );     break;
        case 0x2f  : LCD_puts( " ANAL & COMM. OPTION" );     break;
		case 0x1f  : LCD_puts( " REM. & COMM. OPTION" );     break;
        default    : LCD_puts( "        ERROR       " );     break;
    }
}
*/
void Option_check( void )
{
    switch( ext_key_load() & 0xf0 )
    {
        case 0xf0  : LCD_puts( "    BASIC OPTION    " );     break;
        case 0xe0  : LCD_puts( "   ANALOG OPTION    " );     break;
        case 0xd0  : LCD_puts( "   REMOTE OPTION    " );     break;
		case 0xc0  : LCD_puts( " ANAL & REM. OPTION " );     break;
        case 0xb0  : LCD_puts( "    COMM. OPTION    " );     break;
        case 0xa0  : LCD_puts( " ANAL & COMM. OPTION" );     break;
		case 0x90  : LCD_puts( " REM. & COMM. OPTION" );     break;
		case 0x80  : LCD_puts( "ANAL.REM.COMM.OPTION" );     break;
        default    : LCD_puts( "        ERROR       " );     break;
    }
}

