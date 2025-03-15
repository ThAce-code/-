/*头文件声明区*/
//包含IIC通信,Onewire,超声波传感器，键盘，数码管，串口通信等
#include "Init.h"//系统初始化
#include "LED.h"//led驱动
#include "Key.h"//矩阵键盘
#include "Seg.h"//数码管
#include "Onewire.h"//温度传感
#include "intrins.h"
#include "STDIO.h"
#include "STRING.h"

/*变量声明区*/
idata unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键状态变量
idata unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据
idata unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据
idata unsigned char Seg_Pos;//数码管扫描位置
idata unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//
idata unsigned char Uart_Recv[10];//串口接收数据缓冲区
idata unsigned char Uart_Recv_Index;//串口接收数据索引
idata unsigned char ucRtc[3] = {23,59,55};//时钟数据
idata unsigned int Slow_Down;//减速计时器
bit Seg_Flag,Key_Flag;//数码管和按键标志位
idata unsigned int Temper_x100;//100倍温度值
idata unsigned char SegMode;//显示界面：0-温度界面 1-校准界面 2-参数界面
idata unsigned int CaliTemper;//校准后的温度
idata unsigned int CaliVal = 100;//校准值：100是0，101是1，99是-1
idata unsigned int Para = 126;//参数值：100是0，101是1，99是-1
bit Mode = 0;//0-上触发模式 1-下触发模式


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
	
	if(Key_Down == 4)
	{
		if(++SegMode == 3) SegMode = 0;
	}
	if(Key_Down == 5)
	{
		Mode ^= 1;
	}
	if(SegMode == 1)
	{
		switch(Key_Down)
		{
			case 8:
				if(--CaliVal == 255) CaliVal = 0;
			break;
			case 9:
				if(++CaliVal == 200) CaliVal = 199;
			break;
		}
	}
	if(SegMode == 2)
	{
		switch(Key_Down)
		{
			case 8:
				if(--Para == 255) Para = 0;
			break;
			case 9:
				if(++Para == 200) Para = 199;
			break;
		}
	}
}

/*信息处理函数*/
//更新数码管显示数据
void Seg_proc(void)
{
	unsigned int temp;
	unsigned char i = 5;
	if(Seg_Flag) return;
	Seg_Flag = 1;//设置标志位
	switch(SegMode)
	{
		case 0:
			temp = rd_temperature() * 100;
			if(temp > 10000)
			{
				temp = Temper_x100;
			}
			Temper_x100 = temp;
			CaliTemper = Temper_x100 + CaliVal * 100 - 10000;
			Seg_Buf[0] = 13;
			Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
			Seg_Buf[5] = CaliTemper / 1000 % 10;
			Seg_Buf[6] = CaliTemper / 100 % 10;
			Seg_Buf[7] = CaliTemper / 10 % 10;
			Seg_Point[6] = 1;
			while(Seg_Buf[i] == 0)
			{
				Seg_Buf[i] = 10;
				if(++i == 7)
				{
					break;
				}
			}
		break;
		case 1:
			Seg_Buf[0] = 15;
			Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
			if(CaliVal >= 100)
			{
				Seg_Buf[5] = 10;
				Seg_Buf[6] = (CaliVal >= 110)? (CaliVal - 100) / 10: 10;
				Seg_Buf[7] = CaliVal % 10;
			}
			else
			{
				Seg_Buf[5] = (CaliVal <= 90)? 21: 10;
				Seg_Buf[6] = (CaliVal <= 90)? (100 - CaliVal) / 10: 21;
				Seg_Buf[7] = (100 - CaliVal) % 10;
			}
			Seg_Point[6] = 0;
		break;
		case 2:
			Seg_Buf[0] = 17;
			Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
			if(Para >= 100)
			{
				Seg_Buf[5] = 10;
				Seg_Buf[6] = (Para >= 110)? (Para - 100) / 10: 10;
				Seg_Buf[7] = Para % 10;
			}
			else
			{
				Seg_Buf[5] = (Para <= 90)? 21: 10;
				Seg_Buf[6] = (Para <= 90)? (100 - Para) / 10: 21;
				Seg_Buf[7] = (100 - Para) % 10;
			}
			Seg_Point[6] = 0;
		break;
	}
	
}

/*其它显示函数*/
//LED显示函数
void LED_Proc(void)
{
	unsigned char i;
	for(i = 0;i < 3;i++)
	{
		ucLed[i] = (SegMode == i);
	}
	ucLed[3] = (Mode == 0);
	ucLed[4] = (Mode == 1);
	if(Mode == 0)
	{
		ucLed[7] = (Temper_x100 > (Para - 100) * 100);
	}
	else
	{
		ucLed[7] = (Temper_x100 < (Para - 100) * 100);
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




/* 主函数 */
// 系统初始化，设置定时器和串口
void main()
{
	System_Init();	// 系统初始化
	Timer1_Init();	// 定时器初始化
	while (1)
	{
		Key_Proc();	 // 按键处理
		Seg_Proc();	 // 数码管处理
		Led_Proc();	 // led处理
	}
}




