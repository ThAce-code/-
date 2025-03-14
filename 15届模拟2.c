/*头文件声明区*/
//包含IIC通信,Onewire,超声波传感器，键盘，数码管，串口通信等
#include "Init.h"//系统初始化
#include "LED.h"//led驱动
#include "Key.h"//矩阵键盘
#include "Seg.h"//数码管
#include "ds1302.h"//系统时钟
#include "Onewire.h"//温度传感
#include "iic.h"//IIC通信
#include "intrins.h"
#include "STDIO.h"
#include "STRING.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键状态变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据
unsigned char Seg_Pos;//数码管扫描位置
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//
unsigned char Uart_Recv[10];//串口接收数据缓冲区
unsigned char Uart_Recv_Index;//串口接收数据索引
unsigned char ucRtc[3] = {23,59,55};//时钟数据
unsigned int Slow_Down;//减速计时器
bit Seg_Flag,Key_Flag;//数码管和按键标志位
bit SegMode;
bit Mode;
float Tem;
unsigned char Illuminance;
float volt;



/*键盘处理函数*/
//处理按键输入，检测按键上升沿和下降沿，并更新按键状态
void Key_proc(void)
{
	if(Key_Flag) return;
	Key_Flag = 1; //设置标志位，防止重复进入
	
	Key_Val = Key_Read(); //读取按键
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//检测上升沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//检测下降沿
	Key_Old = Key_Val;//更新按键状态
	
	switch(Key_Down)
	{
		case 5:
			SegMode ^= 1;
		break;
		case 4:
			Mode ^= 1;
		break;
	}	

}

/*信息处理函数*/
//更新数码管显示数据
void Seg_proc(void)
{	
	unsigned char i = 5;
	if(Seg_Flag) return;
	Seg_Flag = 1;//设置标志位
	
	Tem = rd_temperature();
	Illuminance = Ad_Read(0x41);
	if (SegMode == 0)
	{
    if (Mode == 0)
    {
        Seg_Buf[0] = 1;
        Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
        Seg_Buf[5] = (unsigned char)Tem / 10 % 10;
        Seg_Buf[6] = (unsigned char)Tem % 10;
        Seg_Buf[7] = (unsigned int)(Tem * 10) % 10;
        Seg_Point[6] = 1;
    }
    else
    {
        Seg_Buf[0] = 2;
        Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
				Seg_Buf[5] = Illuminance / 100 % 10 ;
        Seg_Buf[6] = Illuminance / 10 % 10 ;
        Seg_Buf[7] = Illuminance % 10;
        Seg_Point[6] = 0;
        while (Seg_Buf[i] == 0)
        {
            Seg_Buf[i] = 10;
            if (++i == 7)
                break;
        }
    }
	}
	else
	{
		Seg_Buf[0] = 20;
		Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = Seg_Buf[5] =10;			
		Seg_Buf[6] = (unsigned char)volt;
		Seg_Buf[7] = (unsigned char)(volt * 10) % 10;
		Seg_Point[6] = 1;
	}	
}	

/*其它显示函数*/
//LED显示函数
void LED_Proc(void)
{
	if(Mode == 0)
	{
		if(Tem < 10)
		{
			volt = 1.0;
		}	
		else if(Tem > 40)
		{
			volt = 5.0;
		}	
		else
		{
			volt = (4.0 / 30) * (Tem - 10) + 1;
		}	
	}	
	else
	{
		if(Illuminance < 10)
		{
			volt = 1.0;
		}	
		else if(Illuminance > 240)
		{
			volt = 5.0;
		}	
		else
		{
			volt = (4.0 / 230) * (Illuminance - 10) + 1;
		}	
	}	
	Da_Write(volt * 51);
		
	ucLed[0] = (Mode == 0);
	ucLed[1] = (Mode == 1);

}	



void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;
	EA = 1;
}


/*定时器1中断服务函数*/
//定时器1的中断服务函数，用于更新系统时钟，处理按键和数码管显示
void Timer1_Isr(void) interrupt 3
{
	if (++Slow_Down == 10)
	{
		Seg_Flag = Slow_Down = 0; //更新数码管显示标志位
	}
	if (Slow_Down % 10 == 0)
	{
		Key_Flag = 0; // 更新按键处理标志位
	}
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
}

void Delay750ms(void)	//@12.000MHz
{
	unsigned char data i, j, k;

	_nop_();
	_nop_();
	i = 35;
	j = 51;
	k = 182;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}



/* 主函数 */
// 系统初始化，设置定时器和串口
void main()
{
	Tem = rd_temperature();
	Delay750ms();
	System_Init();	// 系统初始化
	Timer1_Init();	// 定时器初始化
	Set_Rtc(ucRtc); // 时钟初始化
	while (1)
	{
		Key_Proc();	 // 按键处理
		Seg_Proc();	 // 数码管处理
		Led_Proc();	 // led处理
	}
}




