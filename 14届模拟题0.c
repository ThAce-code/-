/*头文件声明区*/
//包含IIC通信,Onewire,超声波传感器，键盘，数码管，串口通信等
#include "Init.h"//系统初始化
#include "LED.h"//led驱动
#include "Key.h"//矩阵键盘
#include "Seg.h"//数码管
#include "iic.h"//IIC通信
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
idata unsigned char Distance;
idata unsigned char temp;
bit SegMode = 0;
idata unsigned char ParaDisp = 30;
idata unsigned char ParaCtrl = 30;
bit SendFlag = 0;
bit WarningFlag;
bit LedFlag;
idata unsigned char Time_200ms;



/*键盘处理函数*/
//处理按键输入，检测按键上升沿和下降沿，并更新按键状态
void Key_proc(void)
{
	unsigned char i;
	if(Key_Flag) return;
	Key_Flag = 1; //设置标志位，防止重复进入
	
	Key_Val = Key_Read(); //读取按键
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//检测上升沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//检测下降沿
	Key_Old = Key_Val;//更新按键状态
	
	if(Key_Down == 4)
	{
		SegMode ^= 1;
		if(SegMode == 1) ParaDisp = ParaCtrl;
		if(SegMode == 0) ParaCtrl = ParaDisp;
	}
	if(SegMode == 1)
	{
		if(Key_Down == 12)
		{
			ParaDisp += 10;
			if(ParaDisp > 245) ParaDisp = 255;
		}
		if(Key_Down == 16)
		{
			ParaDisp -= 10;
			if(ParaDisp < 10) ParaDisp = 0;
		}
	}	
		if(SegMode == 0)
		{
			if(Key_Down == 8)
			{
				ParaCtrl = Distance;
				for(i = 1;i < 8;i++)
				{
					ucLed[i] = 0;
				}
			}
		}
	if(Key_Down == 9)
	{
		SendFlag = 1;
		if(SendFlag)
		{
			printf("Distance:%dcm",(unsigned int)Distance);
			SendFlag = 0;
		}
	}
		
}


/*信息处理函数*/
//更新数码管显示数据
void Seg_proc(void)
{
	unsigned char i = 5;
	if(Seg_Flag) return;
	Seg_Flag = 1;//设置标志位
	
	Distance = Ut_Wave_Data();
	if(Distance > ParaCtrl)
	{
		WarningFlag = 1;
	}
	else
	{
		WarningFlag = 0;
	}
	if(SegMode == 0)
	{
		Seg_Buf[0] = 20;
		Seg_Buf[1] = 1;
		Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
		Seg_Buf[5] = Distance / 100 % 10;
		Seg_Buf[6] = Distance / 10 % 10;
		Seg_Buf[7] = Distance % 10;
		while(Seg_Buf[i] == 0)
		{
			Seg_Buf[i] = 10;
			if(++i == 7)
			{
				break;
			}
		}
	}
	else
	{
		Seg_Buf[0] = 20;
		Seg_Buf[1] = 2;
		Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
		Seg_Buf[5] = ParaDisp / 100 % 10;
		Seg_Buf[6] = ParaDisp / 10 % 10;
		Seg_Buf[7] = ParaDisp % 10;
		while(Seg_Buf[i] == 0)
		{
			Seg_Buf[i] = 10;
			if(++i == 7)
			{
				break;
			}
		}
	}
		
	
}

/*其它显示函数*/
//LED显示函数
void LED_Proc(void)
{
	ucLed[0] = (SegMode == 0);
	ucLed[1] = (SegMode == 1);
	ucLed[2] = LedFlag;
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
	if(WarningFlag)
	{
		if(++Time_200ms == 200)
		{
			Time_200ms = 0;
			LedFlag ^= 1;
		}
	}
	else
	{
		Time_200ms = 0;
		LedFlag = 0;
	}
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
}

void Uart_Isr(void) interrupt 4
{
	
}



/* 主函数 */
// 系统初始化，设置定时器和串口
void main()
{
	System_Init();	// 系统初始化
	Uart1_Init();
	Timer1_Init();	// 定时器初始化
	while (1)
	{
		Key_Proc();	 // 按键处理
		Seg_Proc();	 // 数码管处理
		Led_Proc();	 // led处理
	}
}




