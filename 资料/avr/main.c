//written by JY-MCU,QQ:179383020

//Contact:QQ:179383020,Website:http://jy-mcu.taobao.com
#include	<avr/io.h>
#include	<util/delay.h>
#include	<tm1650.h>

void port_init(void)
{
	DDRD|=(1<<SDA)|(1<<SCL);
	PORTD|=(1<<SDA)|(1<<SCL);
}
int main(void)
{
	unsigned char i,j;
	port_init();
	Write_DATA(0x48,0x31);
	while(1)
		{
			for(i=0;i<16;i++)
				{
					for(j=0;j<4;j++)
						{
							Write_DATA(0x68+2*j,tab[(j+i)%16]|0x80);
						}
					_delay_ms(500);
				}
		}
}