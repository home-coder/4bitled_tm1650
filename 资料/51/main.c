//written by JY-MCU,QQ:179383020

//Contact:QQ:179383020,Website:http://jy-mcu.taobao.com
#include <REGX51.H>
#include <tm1650.h>

void _delay_ms(unsigned int i)
{
 	unsigned char j,k;
	while(i--)
		{
			for(j=0;j<4;j++)
				for(k=0;k<250;k++);
		}
}

int main(void)
{
	unsigned char i,j;
	Write_DATA(0x48,0x01);
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