#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#define RS 6		//RS=PD6 управляющий сигнал ЖКИ
#define E  7		//E=PD7  управляющий сигнал ЖКИ

#define F_CPU 16000000
#include <util/delay.h>

#define form_array_size 63

////////// IO ///////////
unsigned char matrix[4][4]={{'*','0','#','D'},
                            {'7','8','9','C'},
                            {'4','5','6','B'},
                            {'1','2','3','A'}};

char current_counter=0;
char current[10];
volatile char interrupt=0;
/////////////////////////

////////// PWM ////////////
int counter=0;
unsigned char form_array[form_array_size];
float ampl_rate;
volatile unsigned long time_tick=0;

/* ======form-array======= */
unsigned char sinus[] = { 100, 109, 119, 129, 138, 147, 156, 164, 171, 178, 184, 188, 193, 196, 198
, 199, 199, 199, 197, 194, 191, 186, 181, 174, 168, 160, 152, 143, 134, 124, 114
, 104, 95, 85, 75, 65, 56, 47, 39, 31, 25, 18, 13, 8, 5, 2, 0, 0, 0, 1, 3, 6, 11
, 15, 21, 28, 35, 43, 52, 61, 70, 80, 90, 99};

unsigned char triangle[] = { 100, 106, 112, 118, 124, 130, 136, 142, 148, 154, 160, 166, 172, 178
, 184, 190, 200, 194, 188, 182, 176, 170, 164, 158, 152, 146, 140, 134, 128, 122
, 116, 110, 104, 98, 92, 86, 80, 74, 68, 62, 56, 50, 44, 38, 32, 26, 20, 14, 8
, 6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90};

unsigned char hammer[] = { 1, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20, 24, 29, 35, 43, 52,
 62, 76, 91, 110, 133, 161, 195, 196, 197, 198, 199, 200, 199, 198, 197, 196, 195, 161, 133, 110, 91, 76, 62, 52, 43, 35,
 29, 24, 20, 16, 13, 11, 9, 7, 6, 5, 4, 3, 3, 2, 2, 1, 1};
/* ======================= */

//////////////////////////



//Обмен с ЖКИ
void transfer (unsigned char lcd)
{
 PORTD |= (1<<E);
  PORTB = (lcd>>4);	                //Выводим старшую тетраду команды
  _delay_us(1);	                //Задержка
  PORTD &=~(1<<E);	        //Сигнал записи команды
  _delay_us(1);
  PORTD |= (1<<E);
  PORTB = (lcd&0x0F);	                   //Выводим младшую тетраду команды
  _delay_us(1);	                   //Задержка
  PORTD &=~(1<<E);	           //Сигнал записи команды

  _delay_ms(1);	        //Пауза для выполнения команды
}


//Передача команд в ЖКИ
void lcd_com (unsigned char lcd)
{
  PORTD &=~(1<<RS);		//RS=0 – это команда
  transfer(lcd);
}


//Запись данных в ЖКИ
void lcd_dat (unsigned char lcd)
{
  PORTD =(1<<RS);		//RS=1 – это данные
  transfer(lcd);
}

//Инициализация ЖКИ
void lcd_init (void)
{
  lcd_com(0x28);   //4-проводный интерфейс, 5x8 размер символа
 _delay_ms(5);
 lcd_com(0x0C);		//Показать изображение, курсор не показывать
 _delay_ms(5);
 lcd_com(0x01);		//Очистить DDRAM и установить курсор на 0x00
 _delay_ms(5);
}


//Запись строки
void output (char *str)
{
	int size=strlen(str);
	for (int i=0; i<size; i++) lcd_dat(str[i]);
}


//Запись текущей строки
void out_current (void)
{
	lcd_com(0xA9);
	if (current_counter!=0)
    for(char i=0; i<current_counter; i++) lcd_dat(current[i]);
	for(char i=current_counter; i<11; i++) lcd_dat(' ');
}

int str_to_int (void)
{
    char cycle[10];
	int v=0;
    for (int i=current_counter-1; i>=0; i--) {cycle[v]=current[i]; v++;}
    int tmp=0;
    int k=1;
	int n;
    for(int i=0; i<current_counter; i++)
    {
		n=0;
        for (char j='0'; j<='9'; j++)
		{
            if (cycle[i]==j) tmp+=n*k;
			n++;
		}
        k*=10;
    }
    return tmp;
}

//Конфигурирование
int conf (char *message, int low, int high)
{
    int temp;
    while (1)
    {
		current_counter=0;
        lcd_com(0x01);
        _delay_ms(2);
        output(message);
        lcd_com(0xc0);	//Move cursor on second line
        sei();
        while (interrupt==0) _delay_us(1);
        cli();
		interrupt=0;

        temp=str_to_int();


        if ((temp>=low)&&(temp<=high)) break;
        else
        {
                lcd_com(0x01);		//Clear DDRAM and move cursor on 0x00
                _delay_ms(2);
                output("Invalid value");
                _delay_ms(3000);
        }
    }

    return temp;
}


int main (void)
{

	while(1)
    {
		// Energyless
        DDRD=0x30;
        OCR1AH=120;
        OCR1AL=120;
        OCR1BH=120;
        OCR1BL=120;
        TCCR1A = 1<<COM1A1 | 1<<COM1B1 | 1<<COM1B0 | 1<<WGM10;
        TCCR1B = 1<<WGM12 | 1<<CS10;
		sei();
		_delay_ms(2000);
		cli();
		TCCR1A=0;
		TCCR1B=0;
		
	
	
		DDRB=0;
		PORTB=0;
	
		time_tick=0;

    //////// IO ///////////

    //LCD
    DDRB=0x0F;
    DDRD=0xC0;
    _delay_ms(50);
    lcd_init();
        //Keyboard
        TIMSK=1<<TOIE0;
        TCCR0=1<<CS00 | 1<<CS02;

        int form=conf("Form:", 1, 3);
        int frequency=conf("Frequency:", 10, 2000);
        int amplitude=conf("Amplitude:", 1, 3);
        int time=conf("Time:", 1, 300);
		
        lcd_com(0x01);
		_delay_ms(2);
        output("Working");

        //Timer-0 stop
        TIMSK=0;
        TCCR0=0;

        /////////////////////////

        /////////// PWM /////////

        //Choose form_array
        if (form==1) for (int i=0; i<=form_array_size; i++) form_array[i]=sinus[i];
        if (form==2) for (int i=0; i<=form_array_size; i++) form_array[i]=triangle[i];
        if (form==3) for (int i=0; i<=form_array_size; i++) form_array[i]=hammer[i];


        // Amplitude configure
        if (amplitude==1) ampl_rate=1;
        if (amplitude==2) ampl_rate=1.125;
        if (amplitude==3) ampl_rate=1.25;

        //Time configure
        unsigned long max_time_tick=50*(unsigned long)time*(unsigned long)frequency*(unsigned long)form_array_size;

        //Frequency configure

        TIMSK=(1<<OCIE2); //Timer-2

        unsigned int how_many_tacts=F_CPU/(frequency*form_array_size);
        unsigned char tick_period;
        unsigned int cycle=0; //Overflow cycles
        while(1)
        {
            if ((unsigned int)how_many_tacts/256<=cycle)
            {
                if (cycle==0) //Prescaler - 1
                {
                    TCCR2=1<<CS00;
                    tick_period=how_many_tacts;
                    break;
                }
                if ((cycle>1) && (cycle<=8)) //Prescaler - 8
                {
                    TCCR2=1<<CS01;
                    tick_period=how_many_tacts/8;
                    break;
                }
                if ((cycle>8) && (cycle<=32)) //Prescaler - 32
                {
                    TCCR2=1<<CS01 | 1<<CS00;
                    tick_period=how_many_tacts/32;
                    break;
                }
                if ((cycle>64) && (cycle<=64)) //Prescaler - 64
                {
                    TCCR2=1<<CS02;
                    tick_period=how_many_tacts/64;
                    break;
                }
            }
            cycle++;
        }

        OCR2=tick_period;



        // PWM
        DDRD=0x30;
        OCR1AH=0;
        OCR1AL=0;
        OCR1BH=0;
        OCR1BL=0;
        TCCR1A = 1<<COM1A1 | 1<<COM1B1 | 1<<COM1B0 | 1<<WGM10;
        TCCR1B = 1<<WGM12 | 1<<CS10;
		
		DDRB|=1<<4;
		PORTB|=1<<4;

        sei();

        ////////////////////////////////////////////////////////


        while(time_tick<max_time_tick) _delay_us(1);
		cli();
		TIMSK=0;
		TCCR1A=0;
		TCCR1B=0;
		TCCR2=0;
    }
}


ISR (TIMER0_OVF_vect)
{
	DDRC=0xC3;
    PORTC=0;
    DDRA=0;
    PORTA=0xF0;
    _delay_ms(1);
    unsigned char k=4;
    if ((PINA&0x10)==0) k=0;
    if ((PINA&0x20)==0) k=1;
    if ((PINA&0x40)==0) k=2;
    if ((PINA&0x80)==0) k=3;

    DDRC=0;
    PORTC=0xC3;
    DDRA=0xF0;
    PORTA=0;
    _delay_ms(1);
    unsigned char i=4;
    if ((PINC&0x01)==0) i=0;
    if ((PINC&0x02)==0) i=1;
    if ((PINC&0x40)==0) i=2;
    if ((PINC&0x80)==0) i=3;



    if ((i!=4)&&(k!=4))
    {
        while(PINC!=0xC3) _delay_us(1);

        if ((matrix[i][k]>='0')&&(matrix[i][k]<='9'))
		{
			current[current_counter]=matrix[i][k];
			if (current_counter<10) current_counter++;
		}

        if (matrix[i][k]=='D') if (current_counter>0) current_counter--;
        if (matrix[i][k]=='C') current_counter=0;
        if (matrix[i][k]=='A') interrupt=1;
        out_current();
    }
}


ISR(TIMER2_COMP_vect)
{
	if (counter<form_array_size) counter++;
    else counter=0;
    OCR1AH=form_array[counter]/ampl_rate;
    OCR1AL=form_array[counter]/ampl_rate;
    OCR1BH=form_array[counter]*ampl_rate;
    OCR1BL=form_array[counter]*ampl_rate;
	time_tick++;
}


