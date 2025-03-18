/*头文件声明区*/
//包含IIC通信,Onewire,超声波传感器，键盘，数码管，串口通信等
#include "Init.h"//系统初始化
#include "LED.h"//led驱动
#include "Key.h"//矩阵键盘
#include "Seg.h"//数码管
#include "ds1302.h"//系统时钟
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
unsigned char ucRtc[3] = {23,59,50};//时钟数据
unsigned int Slow_Down;//减速计时器
bit Seg_Flag,Key_Flag,AD_Flag;//数码管和按键标志位
unsigned char SegMode;//0-时间界面 1-数据界面 2-历史查询界面
unsigned int LightVolt;//光敏电压值
unsigned int RB2Volt;//RB2电压值
unsigned int LightVoltOld;//上一次光敏电压值
unsigned int RB2VoltOld;//上一次RB2电压值
bit TrigFlag;//触发标志位
unsigned char His_ucRtc[3][3] = {0};//历史触发时间数组
unsigned char ucRtcIndexW;//写入时间指针
unsigned char ucRtcIndexR;//读取时间指针
unsigned char TrigNum;//触发次数
unsigned char Index;//历史查询界面索引值
unsigned int Time_3000ms;//3秒计时数据



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
		if(TrigFlag) return;
		if(++SegMode == 3) SegMode = 0;
		if(SegMode == 2) Index = 1;       
	}
	if(Key_Down == 5)
	{
		if(SegMode != 2) return;
		if(--ucRtcIndexR == 255) ucRtcIndexR = 2;
		if(++Index == 4) Index = 1;
	}
	if(Key_Down == 8)
	{
		if(SegMode != 2) return;
		TrigNum = 0;
	}
}

/*信息处理函数*/
//更新数码管显示数据
void Seg_proc(void)
{
	if(Seg_Flag) return;
	Seg_Flag = 1;//设置标志位
	
	Read_Rtc(ucRtc);
	LightVolt = Ad_Read(0x03) * 100 / 51;
	RB2Volt = Ad_Read(0x01) * 100 / 51;
	if(LightVoltOld >= RB2VoltOld)
	{
		if(TrigFlag == 0)
		{
			if(LightVolt < RB2Volt)
			{
				TrigFlag = 1;
				if(++TrigNum == 4) TrigNum = 3;
				His_ucRtc[ucRtcIndexW][0] = ucRtc[0];
				His_ucRtc[ucRtcIndexW][1] = ucRtc[1];
				His_ucRtc[ucRtcIndexW][2] = ucRtc[2];
				ucRtcIndexR = ucRtcIndexW;
			}
		}
	}
	LightVoltOld = LightVolt;
	RB2VoltOld = RB2Volt;
	if(TrigFlag)
	{
		Seg_Buf[0] = Seg_Buf[1] = 13;
		Seg_Point[1] = 0;
		Seg_Buf[2] = His_ucRtc[ucRtcIndexW][0] / 10 % 10;
		Seg_Buf[3] = His_ucRtc[ucRtcIndexW][0] % 10;
		Seg_Buf[4] = His_ucRtc[ucRtcIndexW][1] / 10 % 10;
		Seg_Buf[5] = His_ucRtc[ucRtcIndexW][1] % 10;
		Seg_Point[5] = 0;
		Seg_Buf[6] = His_ucRtc[ucRtcIndexW][2] / 10 % 10;
		Seg_Buf[7] = His_ucRtc[ucRtcIndexW][2] % 10;
	}
	else
	{
		switch(SegMode)
	  {
			case 0:
				Seg_Buf[0] = ucRtc[0] / 10;
				Seg_Buf[1] = ucRtc[0] % 10;
				Seg_Point[1] = 0;
				Seg_Buf[2] = 21;
				Seg_Buf[3] = ucRtc[1] / 10;
				Seg_Buf[4] = ucRtc[1] % 10;
				Seg_Buf[5] = 21;
				Seg_Point[5] = 0;
				Seg_Buf[6] = ucRtc[2] / 10;
				Seg_Buf[7] = ucRtc[2] % 10;
			break;
			case 1:
				Seg_Buf[0] = 19;
				Seg_Buf[1] = LightVolt / 100 % 10;
				Seg_Point[1] = 1;
				Seg_Buf[2] = LightVolt / 10 % 10;
				Seg_Buf[3] = LightVolt % 10;
				Seg_Buf[4] = 20;
				Seg_Buf[5] = RB2Volt / 100 % 10;
				Seg_Point[5] = 1;
				Seg_Buf[6] = RB2Volt / 10 % 10;
				Seg_Buf[7] = RB2Volt % 10;
			break;
			case 2:
				if(TrigNum >= Index)
				{
					Seg_Buf[0] = 11;
					Seg_Buf[1] = Index;
					Seg_Point[1] = 0;
					Seg_Buf[2] = His_ucRtc[ucRtcIndexR][0] / 10 % 10;
					Seg_Buf[3] = His_ucRtc[ucRtcIndexR][0] % 10;
					Seg_Buf[4] = His_ucRtc[ucRtcIndexR][1] / 10 % 10;
					Seg_Buf[5] = His_ucRtc[ucRtcIndexR][1] % 10;
					Seg_Point[5] = 0;
					Seg_Buf[6] = His_ucRtc[ucRtcIndexR][2] / 10 % 10;
					Seg_Buf[7] = His_ucRtc[ucRtcIndexR][2] % 10;
				}
				else
				{
					Seg_Buf[0] = 11;
					Seg_Buf[1] = Index;
					Seg_Point[1] = 0;
					Seg_Buf[2] = 21;
					Seg_Buf[3] = 21;
					Seg_Buf[4] = 21;
					Seg_Buf[5] = 21;
					Seg_Point[5] = 0;
					Seg_Buf[6] = 21;
					Seg_Buf[7] = 21;
				}
			break;
	  }
	}
	
	
}

/*其它显示函数*/
//LED显示函数
void LED_Proc(void)
{
	if (TrigFlag) {
        ucLed[7] = 1;
        ucLed[0] = 0;
        ucLed[1] = 0;
        ucLed[2] = 0;
    } else {
		ucLed[0] = (SegMode == 0)? 1: 0;
        ucLed[1] = (SegMode == 1)? 1: 0;
        ucLed[2] = (SegMode == 2)? 1: 0;
        ucLed[7] = 0;
    }
}	

//void AD_Read(void)
//{
//	
//}


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
	if(TrigFlag)
	{
		if(++Time_3000ms == 3000)
		{
			Time_3000ms = 0;
			TrigFlag = 0;
			if(++ucRtcIndexW == 3) ucRtcIndexW = 0;
			
		}
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
	Set_Rtc(ucRtc); // 时钟初始化
	while (1)
	{
		Key_Proc();	 // 按键处理
		Seg_Proc();	 // 数码管处理
		Led_Proc();	 // led处理
//		AD_Read();
	}
}