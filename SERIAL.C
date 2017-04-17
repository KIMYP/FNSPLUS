/**********************************************************
*       CPU         : DALAS 80C320                        *
*       Object      : Serial Function sub-routine         *
*       Date        : 2001. 01. 09.                       *
*       Version     : 1.10                                *
**********************************************************/

/*--------   Communication Command  ---------*/

#define     CR                      0x0d        /* Carriage Return */
#define     LF                      0x0a        /* Line Feed */
#define     SP                      0x20        /* Space */
#define     ESC                     0x1b        /* Escape */
#define     BR1200                  0
#define     BR2400                  1
#define     BR4800                  2
#define     BR9600                  3
#define     BR19200                 4
#define     BUFSIZE                 30

#define     MIN_1                   0
#define     MIN_10                  1
#define     MIN_30                  2
#define     HOUR_1                  3
#define     HOUR_6                  4
#define     HOUR_12                 5
#define     HOUR_24                 6

typedef struct
{
    Int     Head;
    Int     Tail;
    Char    Empty;
    char    Buff[BUFSIZE + 1];
}ComQueue;

#pragma memory = xdata

ComQueue    Buf;
char        Buffer[20];
Char        Pos=0;
Char        Comm_OK;
#pragma memory = default

void serial_ini( unsigned char b_rate )
{                                           /* clock : 18.432MHz */
    TMOD |= 0x20;                           /* timer #1 : timer mode 2 */
    SCON0 = 0x50;                           /* serial mode #1,receive enable */
    PCON = 0;                               /* SMOD = 0, asynchronous comm. */
    switch( b_rate )
    {
        case BR1200 :   TH1 = 0xd8;         break;
        case BR2400 :   TH1 = 0xec;         break;
        case BR4800 :   TH1 = 0xf6;         break;
        case BR9600 :   TH1 = 0xfb;         break;
        case BR19200:   TH1 = 0xfb;
                        PCON = 0x80;        break;
    }
    TR1 = 1;            ES0 = 1;            /* timer #1 run in TCON reg. */
}

void SerialPutc( unsigned char ch )
{
    ES0 = 0;                                SBUF0 = ch;
    while( TI0 == 0 );

    TI0 = 0;                                /* Off TX flag */
    ES0 = 1;
}

void SerialPuts( unsigned char *s )
{
    while( *s )                             SerialPutc( *s++ );
}

unsigned char serial_getc(void)
{
    while( RI0 == 0 );

    RI0 = 0;                                /* Off RX flag */
    return( SBUF0 );
}

char GetQueue( ComQueue *q )
{
    signed char    Data = -1;

    if( q->Head == q->Tail )                return  Data;

    Data = q->Buff[ q->Tail++ ];            q->Tail %= BUFSIZE;

    return    Data;
}


void IntAscii( Char dp, Long value, Char sign, Char fill, Char digit )
{
#pragma memory = xdata

    Long a = 1;
    char i, t, u = 1;
    Char null;

#pragma memory = default

    for( i=0 ; i<digit ; i++ )              a *= 10;

    value %= a;

    for( i=0 ; i<digit ; i++ )
    {
        a /= 10;                            t = value / a;

        if( !t && u && ( digit - ( dp + 1 )) != i )
        {
            if( !fill )                     SerialPutc( '0' );
            else if( fill == 1 )            SerialPutc( ' ' );
            else                            null = 0;
        }
        else if( i == ( digit - 1 ) && !t ) SerialPutc( '0' );
        else
        {
            u = 0;
            if( !sign )
            {
                sign = 1;                   SerialPutc( '-' );
            }
            SerialPutc( t + '0' );

            if( dp && ( digit - ( dp + 1 ) == i ))
                SerialPutc( '.' );
        }
        value %= a;
    }
}

char GetRtc( unsigned char name, unsigned char digit )
{
    char result;                /* BCD Format 00-99 -> digit0,1 */
     
    result = RTC[name];

    if( digit )                             result &= 0x0f;
    else                                    result >>= 4;
    result %= 10;                           result += 0x30;
    return result;
}    

void PutRtc( Char name, Char ten, Char one )
{
    char a = 0;
    
    a = ( ten - 0x30 ) << 4;                a += ( one - 0x30 );
    RTC[name] = a;
}

void SendDate( Char df )
{ 
    if( df == 1 )                           /* USA Format */
    {
        SerialPutc( GetRtc( rMONTH, 0 ));   SerialPutc( GetRtc( rMONTH, 1 ));
        SerialPutc( '/' );
        SerialPutc( GetRtc( rDAY, 0 ));     SerialPutc( GetRtc( rDAY, 1 ));
        SerialPutc( '/' );
        SerialPutc( GetRtc( rCENTURY, 0 )); SerialPutc( GetRtc( rCENTURY, 1 ));
        SerialPutc( GetRtc( rYEAR, 0 ));    SerialPutc( GetRtc( rYEAR, 1 ));             
    }
    else if( df == 2 )                      /* Eur Format */
    {
        SerialPutc( GetRtc( rDAY, 0 ));     SerialPutc( GetRtc( rDAY, 1 ));
        SerialPutc( '/' );
        SerialPutc( GetRtc( rMONTH, 0 ));   SerialPutc( GetRtc( rMONTH, 1 ));
        SerialPutc( '/' );
        SerialPutc( GetRtc( rCENTURY, 0 )); SerialPutc( GetRtc( rCENTURY, 1 ));
        SerialPutc( GetRtc( rYEAR, 0 ));    SerialPutc( GetRtc( rYEAR, 1 ));             
    }
    else                                    /* Korea Format */          
    {
        SerialPutc( GetRtc( rCENTURY, 0 )); SerialPutc( GetRtc( rCENTURY, 1 ));
        SerialPutc( GetRtc( rYEAR, 0 ));    SerialPutc( GetRtc( rYEAR, 1 ));             
        SerialPutc( '/' );
        SerialPutc( GetRtc( rMONTH, 0 ));   SerialPutc( GetRtc( rMONTH, 1 ));
        SerialPutc( '/' );
        SerialPutc( GetRtc( rDAY, 0 ));     SerialPutc( GetRtc( rDAY, 1 ));
    }
}

void SendTime( void )
{
    SerialPutc( GetRtc( rHOUR, 0 ));        SerialPutc( GetRtc( rHOUR, 1 ));
    SerialPutc( ':' );
    SerialPutc( GetRtc( rMIN, 0 ));         SerialPutc( GetRtc( rMIN, 1 ));
    SerialPutc( ':' );
    SerialPutc( GetRtc( rSEC, 0 ));         SerialPutc( GetRtc( rSEC, 1 ));
}

void Set_time( void )
{
    Char dis[16];
    Char    En_pos = 1;
    Char x = 0, i, d, ini=0;

    RTC[11] = 0x82;

    dis[0] = GetRtc( rCENTURY, 0 );         dis[1] = GetRtc( rCENTURY, 1 );
    dis[2] = GetRtc( rYEAR, 0 );            dis[3] = GetRtc( rYEAR, 1 );
    dis[4] = '/';
    dis[5] = GetRtc( rMONTH, 0 );           dis[6] = GetRtc( rMONTH, 1 );
    dis[7] = '/';
    dis[8] = GetRtc( rDAY, 0 );             dis[9] = GetRtc( rDAY, 1 );
    dis[10] = ' ';
    dis[11] = GetRtc( rHOUR, 0 );           dis[12] = GetRtc( rHOUR, 1 );
    dis[13] = ':';
    dis[14] = GetRtc( rMIN, 0 );            dis[15] = GetRtc( rMIN, 1 );
 
    LCD_pos( 2, 2 );
    for( i=0 ; i<16; i++ )                  LCD_putc( dis[i] );
    LCD_pos( 2, 2 );                        LCD_cursorblink( ON );
    for(;;)
    {
        if( En_pos )
        {
            sync_data( LCDpos );            d = dis[x] - '0';
            En_pos = 0;
        }

        if( kUp )
        {
            beep();                         LCD_cursorblink( OFF );
            En_pos = 1;
            d++;                            d %= 10;
            dis[x] = d +'0';                LCD_putc( dis[x] );
            ini = 1;
            key_ready( Up );                LCD_cursorblink( ON );
        }
        else if( kDown )
        {
            beep();                         LCD_cursorblink( OFF );
            En_pos = 1;
            if( !d )                        d = 10;
            d--;
            dis[x] = d +'0';                LCD_putc( dis[x] );
            ini = 1;
            key_ready( Down );              LCD_cursorblink( ON );
        }
        else if( kShift )
        {
            beep();                         ini = 1;
            En_pos = 1;
            LCDpos++;                       x++;
            if( dis[x] == ' ' || dis[x] == '/' || dis[x] == ':' )
            {
                LCDpos++;                   x++;
            }
            if( x == 16 )
            {
                LCD_pos( 2, 2 );            x = 0;
            }
            key_ready( Shift );
        }
        else if( kMode )
        {
            beep();                         ini = 1;
            En_pos = 1;
            dis[x] = '0';                   LCD_putc( dis[x] );
            key_ready( Mode );
        }
        else if( kEnter )
        {
            beep();                         LCD_cursorblink( OFF );
            PutRtc( rCENTURY, dis[0], dis[1] );
            PutRtc( rYEAR, dis[2], dis[3] );
            PutRtc( rMONTH, dis[5], dis[6] );
            PutRtc( rDAY, dis[8], dis[9] );
            PutRtc( rHOUR, dis[11], dis[12] );
            PutRtc( rMIN, dis[14], dis[15] );
            if( ini )                       PutRtc( rSEC, '0', '0' );
            RTC[11] = 0x02;
            key_ready( Enter );             break;
        }
    }
}

