/*头文件声明区*/
//包含IIC通信,Onewire,超声波传感器，键盘，数码管，串口通信等
#include "Init.h"//系统初始化
#include "LED.h"//led驱动
#include "Key.h"//矩阵键盘
#include "Seg.h"//数码管
#include "iic.h"//IIC通信
#include <Uart.h>// 串口底层驱动专用头文件
#include "intrins.h"
#include "STDIO.h"
#include "STRING.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键状态变量
idata unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据
idata unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据
unsigned char Seg_Pos;//数码管扫描位置
idata unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//
unsigned char Uart_Recv[10];//串口接收数据缓冲区
unsigned char Uart_Recv_Index;//串口接收数据索引
idata unsigned char ucRtc[3] = {23,59,55};//时钟数据
unsigned int Slow_Down;//减速计时器
unsigned int Sys_Tick;//串口计时
bit Seg_Flag,Key_Flag,Uart_Flag;//数码管和按键标志位
bit SegMode;//0-噪音界面 1-参数界面
unsigned int NoisesPara;//噪音参数
unsigned char ParaDB = 65;//分贝参数
unsigned char Time_100ms;//0.1秒计时
bit WarningFlag;//警告标志
bit LedFlag;//Led闪烁标志



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

	if(Key_Down == 12) SegMode ^= 1;
	if(SegMode == 1)
	{
		switch(Key_Down)
		{
			case 16:
				ParaDB = (ParaDB + 5 >= 86)?90:ParaDB + 5;
			break;
			case 17:
				ParaDB = (ParaDB - 5 <= 4)?0:ParaDB - 5;
			break;
		}
			
	}

}

/*信息处理函数*/
//更新数码管显示数据
void Seg_proc(void)
{
	unsigned char i = 6;
	if(Seg_Flag) return;
	Seg_Flag = 1;//设置标志位

	NoisesPara = Ad_Read(0x03) * 180 / 51;
	WarningFlag = (NoisesPara > (ParaDB * 10));
	if(SegMode == 0)
	{
		Seg_Buf[0] = 20;
		Seg_Buf[1] = 1;
		Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
		Seg_Buf[5] = (NoisesPara / 100 % 10 == 0)?10:NoisesPara / 100 % 10;
		Seg_Buf[6] = NoisesPara / 10 % 10;
		Seg_Point[6] = 1;
		Seg_Buf[7] = NoisesPara % 10;
	}
	else
	{
		Seg_Buf[0] = 20;
		Seg_Buf[1] = 2;
		Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = Seg_Buf[5] = 10;
		Seg_Buf[6] = ParaDB / 10 % 10;
		Seg_Point[6] = 0;
		Seg_Buf[7] = ParaDB % 10;
		while(Seg_Buf[i] == 0)
		{
			Seg_Buf[i] = 10;
			if(++i == 7) break;
		}
	}
}

/*其它显示函数*/
//LED显示函数
void LED_Proc(void)
{
	ucLed[0] = (SegMode == 0);
	ucLed[1] = (SegMode == 1);
	ucLed[7] = LedFlag;
}	

void Uart_proc()
{

  if (Uart_Recv_Index == 0)return;
	if (Sys_Tick >= 10)
	{
		Sys_Tick = Uart_Flag = 0;
		if(SegMode == 0)
		{
			if(Uart_Recv[0] == 'R' && Uart_Recv[1] == 'e' && Uart_Recv[2] == 't' && Uart_Recv[3] == 'u' && Uart_Recv[4] == 'r' && Uart_Recv[5] == 'n' )
			{
				printf("Noises:%0.1fdB",(float)NoisesPara / 10.0);
			}
		}
		memset(Uart_Recv, 0, Uart_Recv_Index);
		Uart_Recv_Index = 0; // 重置接收索引
	}
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
	if(++Slow_Down == 10)
	{
		Seg_Flag = Slow_Down = 0; //更新数码管显示标志位
	}
	if(Slow_Down % 10 == 0)
	{
		Key_Flag = 0; // 更新按键处理标志位
	}
  if(Uart_Flag)
	{
    Sys_Tick++;
  }
	if(WarningFlag)
	{
		if(++Time_100ms == 100)
		{
			Time_100ms = 0;
			LedFlag ^= 1;
		}
	}
	else
	{
		Time_100ms = 0;
		LedFlag = 0;
	}
	
		

	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);

}

/* 串口1中断服务函数 */
// 串口1的中断服务函数，用于处理串口接收到的数据。
void Uart1Server() interrupt 4
{
	if (RI == 1) // 检测到串口接收中断
	{
		Uart_Flag = 1;					   // 设置串口标志位
		Sys_Tick = 0;					   // 重置系统时钟
		Uart_Recv[Uart_Recv_Index] = SBUF; // 保存接收到的数据
		Uart_Recv_Index++;// 更新接收索引
		RI = 0;	 // 清除中断标志位
	}
	
	if (Uart_Recv_Index > 10)
		Uart_Recv_Index = 0;
}


/* 主函数 */
// 系统初始化，设置定时器和串口
void main()
{
	System_Init();	// 系统初始化
	Timer1_Init();	// 定时器初始化
    UartInit();		// 初始化串口
	while (1)
	{
	  Key_Proc();	 // 按键处理
	  Seg_Proc();	 // 数码管处理
	  Led_Proc();	 // led处理
      Uart_Proc(); // 处理串口数据
	}
}


