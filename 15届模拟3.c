/*头文件声明区*/
//包含IIC通信,Onewire,超声波传感器，键盘，数码管，串口通信等
#include "main.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键状态变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//数码管小数点数据
unsigned char Seg_Pos;//数码管扫描位置
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据
unsigned char Uart_Recv[10];//串口接收数据缓冲区
unsigned char Uart_Recv_Index;//串口接收数据索引
unsigned char ucRtc[3] = {23,59,55};//时钟数据
unsigned int Slow_Down;//减速计时器
bit Seg_Flag,Key_Flag;//数码管和按键标志位
unsigned char SegMode;//0-测距界面 1-参数界面 2-记录界面
unsigned char Mode;//参数调整模式 0-按键模式 1-旋钮模式
unsigned char Distance;//距离
unsigned char ParamentDisp[2] = {10,60};//参数显示
unsigned char ParamentCtrl[2] = {10,60};//参数控制
unsigned char Para_mode;
unsigned char warning;//报警次数
bit Warning_flag;
unsigned char Vol_level;
unsigned char time_100ms;
bit LedDispFlag;


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
		case 4:
			SegMode = (++SegMode) % 3;
			if(SegMode == 1)
			{
				ParamentDisp[0] = ParamentCtrl[0];
				ParamentDisp[1] = ParamentCtrl[1];
			}	
			if(SegMode == 2)
			{
				ParamentCtrl[0] = ParamentDisp[0];
				ParamentCtrl[1] = ParamentDisp[1];
			}	
		break;
		case 5:
			if(SegMode == 1)
			{
				Mode ^= 1; 
			}	
			if(SegMode == 2)
			{
				warning = 0;
			}	
		break;
	}	
	if (SegMode == 1)
    { 
        if (Mode == 0)
        {
            if (Key_Down == 9)
                ParamentDisp[1] = (ParamentDisp[1] + 10 > 90) ? 50: ParamentDisp[1] + 10;
            if (Key_Down == 8)
                ParamentDisp[0] = (ParamentDisp[0] + 10 > 40) ? 0: ParamentDisp[0] + 10;
        }
        else
        {
            if (Key_Down == 9)
                Para_mode = 1;
            if (Key_Down == 8)
                Para_mode = 2;
        }
    }
}

/*信息处理函数*/
//更新数码管显示数据
void Seg_proc(void)
{
	unsigned char i = 5;
	unsigned char j;
	if(Seg_Flag) return;
	Seg_Flag = 1;//设置标志位
	
	Distance = Ut_Wave_Data();
	// 超出上下限
	if ((Distance < ParamentDisp[0] || Distance > ParamentDisp[1]))
	{
			// 没有报警的时候
			if (Warning_flag == 0)
			{
					warning++;
					Warning_flag = 1;
			}
	}
	// 没有超出上下限，拉低报警flag，准备下一次报警
	else
	{
			Warning_flag = 0;
	}
	if (Mode == 1)
	{
			Vol_level = Ad_Read(0x03) / 51;
			Vol_level = (Vol_level >= 4) ? 4 : Vol_level; 
			if (Para_mode == 1)
					ParamentDisp[1] = Vol_level * 10 + 50;
			else if (Para_mode == 2)
					ParamentDisp[0] = Vol_level * 10;
	}
	switch(SegMode)
	{
		case 0:
			Seg_Buf[0] = 11;
			Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
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
		break;
		case 1:
			Seg_Buf[0] = 19;
			Seg_Buf[1] = Mode + 1;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = ParamentDisp[0] / 10 % 10;
			Seg_Buf[4] = ParamentDisp[0] % 10;
			Seg_Buf[5] = 21;
			Seg_Buf[6] = ParamentDisp[1] / 10 % 10;
			Seg_Buf[7] = ParamentDisp[1] % 10;
		break;
		case 2:
			Seg_Buf[0] = 15;
			for(j = 1;j < 7;j++)
			{
				Seg_Buf[j] = 10;
			}
			if(warning <= 9)
			{
				Seg_Buf[7] = warning;
			}	
			else
			{
				Seg_Buf[7] = 21;
			}	
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
	ucLed[7] = (Warning_flag == 0)? 1: LedDispFlag;
}	



void Timer0_Init(void)		//1毫秒@11.0592MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TL0 = 0x18;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;
	EA = 1;
}



/*定时器1中断服务函数*/
//定时器1的中断服务函数，用于更新系统时钟，处理按键和数码管显示
void Timer0_Isr(void) interrupt 1
{
	if (++Slow_Down == 10)
	{
		Seg_Flag = Slow_Down = 0; //更新数码管显示标志位
	}
	if (Slow_Down % 10 == 0)
	{
		Key_Flag = 0; // 更新按键处理标志位
	}
	if(Warning_flag)
	{
		if(++time_100ms == 100)
		{
			time_100ms = 0;
			LedDispFlag ^= 1;
		}	
	}	
	else
	{
		time_100ms = 0;
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
	Timer0_Init();	// 定时器初始化
	Set_Rtc(ucRtc); // 时钟初始化
	while (1)
	{
		Key_Proc();	 // 按键处理
		Seg_Proc();	 // 数码管处理
		Led_Proc();	 // led处理
	}
}




