/*头文件声明区*/
//包含IIC通信,Onewire,超声波传感器，键盘，数码管，串口通信等
#include "Init.h"//系统初始化
#include "LED.h"//led驱动
#include "Key.h"//矩阵键盘
#include "Seg.h"//数码管
#include "ds1302.h"
#include "iic.h"
#include "intrins.h"
#include "math.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键状态变量
idata unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据
idata unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据
unsigned char Seg_Pos;//数码管扫描位置
idata unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//
idata unsigned char ucRtc[3] = {13,3,5};//时钟数据
unsigned char Slow_Down;
bit Seg_Flag,Key_Flag,Uart_Flag;//数码管和按键标志位
idata long Freq;//频率获取值
long FreqOld;//频率获取旧值
unsigned int Time_1s;//1秒计时
unsigned char SegMode;//界面模式参数 0-频率 1-参数 2-时间 3-回显 
unsigned int PF = 2000;//超限参数
bit BianHao;//编号
int JZ;//校准值
bit ReMode;//回显模式
unsigned char ucRtcMax[3] = {0};//最大频率发生时间
float Volt;
unsigned char Time_200ms;
unsigned char Warning_Time_200ms;
bit LedModeFlag=0;
bit LedWarnFlag=0;
bit WarningFlag=0;
bit ErrorFlag=0;


/*键盘处理函数*/
//处理按键输入，检测按键上升沿和下降沿，并更新按键状态
void Key_proc(void)
{
	if(Key_Flag) return;
	Key_Flag = 1; //设置标志位，防止重复进入
	
	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上降沿
	Key_Old = Key_Val;//辅助扫描变量
	
	if(Key_Down == 4)
	{
		if(++SegMode == 4) SegMode = 0;
		if(SegMode == 3) ReMode = BianHao = 0;
	}
	if(Key_Down == 5)
	{
		if(SegMode == 1)
		{
			BianHao ^= 1;
		}
		if(SegMode == 3)
		{
			ReMode ^= 1;
		}
	}
	if(SegMode == 1)
	{
		if(BianHao == 0)
		{
			if(Key_Down == 8)
			{
				PF = (PF + 1000 > 9000)?9000:PF + 1000;	
			}
			if(Key_Down == 9)
			{
				PF = (PF - 1000 < 1000)?1000:PF - 1000;	
			}
		}
		else
		{
			if(Key_Down == 8)
			{
				JZ = (JZ + 100 > 900)?900:JZ + 100;	
			}
			if(Key_Down == 9)
			{
				JZ = (JZ - 100 < -900)?-900:JZ - 100;	
			}
		}
	}
		
}

/*信息处理函数*/
//更新数码管显示数据
void Seg_proc(void)
{
	if(Seg_Flag) return;
	Seg_Flag = 1;//设置标志位
	
	Read_Rtc(ucRtc);
	if(Freq > FreqOld)
	{
		FreqOld = Freq;
		Read_Rtc(ucRtcMax);
	}
	if(Freq < 0)
	{
		ErrorFlag = 1;
	}
  else
	{
		ErrorFlag = 0;
	}
	if(Freq > PF)
	{
		WarningFlag = 1;
	}
	else
	{
		WarningFlag = 0;
	}
	switch(SegMode)
	{
		case 0:
			if(ErrorFlag)
			{
				Seg_Buf[0] = 16;
				Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = Seg_Buf[5] = 10;
				Seg_Buf[6] = Seg_Buf[7] = 18;
			}
			else
			{
				Seg_Buf[0] = 16;
				Seg_Buf[1] = Seg_Buf[2] = 10;
				Seg_Buf[3] = (Freq / 10000 % 10 == 0)?10:Freq / 10000 % 10;
				Seg_Buf[4] = (Freq / 1000 % 10 == 0 && Seg_Buf[3] == 10)?10:Freq / 1000 % 10;
				Seg_Buf[5] = (Freq / 100 % 10 == 0 && Seg_Buf[4] == 10)?10:Freq / 100 % 10;
				Seg_Buf[6] = (Freq / 10 % 10 == 0 && Seg_Buf[5] == 10)?10:Freq / 10 % 10;
				Seg_Buf[7] = Freq % 10;
			}
		break;
		case 1:
			if(BianHao == 0)
			{
				Seg_Buf[0] = 19;
				Seg_Buf[1] = 1;
				Seg_Buf[2] = Seg_Buf[3] = 10;
				Seg_Buf[4] = PF / 1000 % 10;
				Seg_Buf[5] = PF / 100 % 10;
				Seg_Buf[6] = PF / 10 % 10;
				Seg_Buf[7] = PF % 10;
			}
			else
			{
				Seg_Buf[0] = 19;
				Seg_Buf[1] = 2;
				Seg_Buf[2] = Seg_Buf[3] = 10;
				if(JZ >= 0)
				{
					Seg_Buf[4] = 10;
					Seg_Buf[5] = (JZ == 0)?10:JZ / 100 % 10;
					Seg_Buf[6] = (Seg_Buf[5]==10)?10:0;
					Seg_Buf[7] = 0;
				}
				else
				{
					Seg_Buf[4] = 21;
					Seg_Buf[5] = (abs(JZ) == 0)?10:abs(JZ) / 100 % 10;
					Seg_Buf[6] = (Seg_Buf[5]==10)?10:0;
					Seg_Buf[7] = 0;
				}
			}
		break;
		case 2:
			Seg_Buf[0] = ucRtc[0] / 10;
			Seg_Buf[1] = ucRtc[0] % 10;
			Seg_Buf[2] = 21;
			Seg_Buf[3] = ucRtc[1] / 10;
			Seg_Buf[4] = ucRtc[1] % 10;
			Seg_Buf[5] = 21;
			Seg_Buf[6] = ucRtc[2] / 10;
			Seg_Buf[7] = ucRtc[2] % 10;
		break;
		case 3:
			if(ReMode == 0)
			{
				Seg_Buf[0] = 17;
				Seg_Buf[1] = 16;
				Seg_Buf[2] = 10;
				Seg_Buf[3] = (FreqOld / 10000 % 10 == 0)?10:FreqOld / 10000 % 10;
				Seg_Buf[4] = (FreqOld / 1000 % 10 == 0 && Seg_Buf[3] == 10)?10:FreqOld / 1000 % 10;
				Seg_Buf[5] = (FreqOld / 100 % 10 == 0 && Seg_Buf[4] == 10)?10:FreqOld / 100 % 10;
				Seg_Buf[6] = (FreqOld / 10 % 10 == 0 && Seg_Buf[5] == 10)?10:FreqOld / 10 % 10;
				Seg_Buf[7] = FreqOld % 10;
			}
			else
			{
				Seg_Buf[0] = 17;
				Seg_Buf[1] = 11;
				Seg_Buf[2] = ucRtcMax[0] / 10;
				Seg_Buf[3] = ucRtcMax[0] % 10;
				Seg_Buf[4] = ucRtcMax[1] / 10;
				Seg_Buf[5] = ucRtcMax[1] % 10;
				Seg_Buf[6] = ucRtcMax[2] / 10;
				Seg_Buf[7] = ucRtcMax[2] % 10;
			}
			
		break;
	}

}

/*其它显示函数*/
//LED显示函数
void LED_Proc(void)
{
	if(Freq < 0) Volt = 0;
	if(Freq <= 500 && Freq >= 0) Volt = 1.0;
	if(Freq >= PF) Volt = 5.0;
	if(Freq > 500 && Freq < PF)
	{
		Volt = (4.0 / (PF - 500)) * (Freq - 500) + 1;
	}
	Da_Write(Volt * 51);
	
	ucLed[0] = LedModeFlag;
	ucLed[1] = LedWarnFlag;
}	


void Timer0_Init(void)		//1微秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
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
	unsigned char i;
	if(++Slow_Down == 10)
	{
		Seg_Flag = Slow_Down = 0; //更新数码管显示标志位
	}
	if(Slow_Down % 10 == 0)
	{
		Key_Flag = 0; // 更新按键处理标志位
	}
	if(++Time_1s == 1000)
	{
		Time_1s = 0;
		Freq = TH0 << 8 | TL0;
		Freq = Freq + JZ;
		TH0 = TL0 = 0;
	}
	if(SegMode ==  0)
	{
		if(++Time_200ms == 200)
		{
			Time_200ms = 0;
			LedModeFlag ^= 1;
		}
	}
	else
	{
		Time_200ms = 0;
		LedModeFlag = 0;
	}
	if(WarningFlag)
	{
		if(++Warning_Time_200ms == 200)
		{
			Warning_Time_200ms = 0;
			LedWarnFlag ^= 1;
		}
	}
	else if(ErrorFlag)
	{
		Warning_Time_200ms = 0;
		LedWarnFlag = 1;
	}
	else
	{
		Warning_Time_200ms = 0;
		LedWarnFlag = 0;
	}
	
	
		

	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
  for(i = 0;i < 8;i++) Led_Disp(i,ucLed[i]);

}


/* 主函数 */
// 系统初始化，设置定时器和串口
void main()
{
	System_Init();	// 系统初始化
	Set_Rtc(ucRtc);
	Timer0_Init();
	Timer1_Init();	// 定时器初始化
	while (1)
	{
		Key_Proc();	 // 按键处理
		Seg_Proc();	 // 数码管处理
		Led_Proc();	 // led处理
	}
}


