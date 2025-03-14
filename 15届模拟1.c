#include "STC15F2K60S2.H"
#include "Led.h"
#include "Key.h"
#include "Seg.h"
#include "ds1302.h"
#include "iic.h"
#include "intrins.h"
#include "Init.h"
#include "stdio.h"
#include "string.h"
/* LED显示 */
unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};

/* 数码管显示 */
unsigned char Seg_Slow_Down;// 数码管减速
unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10}; // 数码管显示的值
unsigned char Seg_Pos;// 数码管指示
unsigned char Seg_Point[8] = {0, 0, 0, 0, 0, 0, 0, 0};// 某位是否显示小数点

/* 时间方面 */
unsigned char ucRtc[3] = {0x23, 0x09, 0x59}; // 初始化时间
unsigned char InputTime[3];// 输入数据的时间
/* 键盘方面 */
unsigned char Key_Slow_Down;

/* 显示 */
unsigned char SegMode; // 0 时间 1 输入 2 记录

/* 数据 */
unsigned char InputData[4]; // 输入的四位数据
unsigned char InputDataIndex;  // 输入数据的索引
unsigned int Data;
unsigned int DataOld;
unsigned char EEPROMData[4];
unsigned char EEPROMDataOld[4];
bit DataUpFlag; // 数据上升标志
/* 键盘处理函数 */
void Key_Proc()
{
    static unsigned char Key_Val, Key_Down, Key_Up, Key_Old;
    unsigned char i;
    if (Key_Slow_Down)
        return;
    Key_Slow_Down = 1;

    Key_Val = Key_Read();
    Key_Down = Key_Val & (Key_Old ^ Key_Val);
    Key_Up = ~Key_Val & (Key_Old ^ Key_Val);
    Key_Old = Key_Val;
    if (Key_Down == 4)
    {
        
        if (SegMode == 1)
        {
            Data = InputData[0] * 1000 + InputData[1] * 100 + InputData[2] * 10 + InputData[3];
            EEPROMData[0] = InputTime[0] / 16 * 10 + InputTime[0] % 16;
            EEPROMData[1] = InputTime[1] / 16 * 10 + InputTime[1] % 16;
            EEPROMData[2] = Data >> 8;
            EEPROMData[3] = Data & 0x00ff;
            EEPROM_Write(EEPROMData, 0, 4);
            DataUpFlag = (Data > DataOld);
            DataOld = Data;
        }
        SegMode = (++SegMode) % 3;
        for (i = 0; i < 4; i++)
            InputData[i] = 10; // 直接对应灭，每次进行界面切换的时候就清空一下数组
        InputDataIndex = 0;// 输入数据索引重置
    }
    if (SegMode == 1)
    {
        // 清空数据
        if (Key_Down == 5)
        {
            for (i = 0; i < 4; i++)
                InputData[i] = 10; // 直接对应灭，每次进行界面切换的时候就清空一下数组
            InputDataIndex = 0;// 输入数据索引重置
        }
        // 输入第一位数据
        if (InputDataIndex == 1)
            Read_Rtc(InputTime);
        // 当输入的数据不足四位的时候
        if (InputDataIndex < 4)
        {
            switch (Key_Down)
            {
            case 6:
                InputData[InputDataIndex] = 0;
                InputDataIndex++;
                break;
            case 8:
                InputData[InputDataIndex] = 7;
                InputDataIndex++;
                break;
            case 12:
                InputData[InputDataIndex] = 8;
                InputDataIndex++;
                break;
            case 16:
                InputData[InputDataIndex] = 9;
                InputDataIndex++;
                break;
            case 9:
                InputData[InputDataIndex] = 4;
                InputDataIndex++;
                break;
            case 13:
                InputData[InputDataIndex] = 5;
                InputDataIndex++;
                break;
            case 17:
                InputData[InputDataIndex] = 6;
                InputDataIndex++;
                break;
            case 10:
                InputData[InputDataIndex] = 1;
                InputDataIndex++;
                break;
            case 14:
                InputData[InputDataIndex] = 2;
                InputDataIndex++;
                break;
            case 18:
                InputData[InputDataIndex] = 3;
                InputDataIndex++;
                break;
            }
        }
    }
}
/* 数码管处理函数 */
void Seg_Proc()
{
    unsigned char i;
    if (Seg_Slow_Down)
        return;
    Seg_Slow_Down = 1;
    switch (SegMode)
    {
    case 0:
        /* 时间 */
        Read_Rtc(ucRtc);
        Seg_Buf[0] = ucRtc[0] / 16;
        Seg_Buf[1] = ucRtc[0] % 16;
        Seg_Buf[2] = 21; //-
        Seg_Buf[3] = ucRtc[1] / 16;
        Seg_Buf[4] = ucRtc[1] % 16;
        Seg_Buf[5] = 21; //-
        Seg_Buf[6] = ucRtc[2] / 16;
        Seg_Buf[7] = ucRtc[2] % 16;
        break;
    case 1:
        /* 输入 */
        Seg_Buf[0] = 13; // C
        Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = 10;
        // 没有输入的时候直接是空的
        if (InputDataIndex == 0)
        {
            Seg_Buf[4] = Seg_Buf[5] = Seg_Buf[6] = Seg_Buf[7] = 10;
        }
        else
        {
            for (i = 0; i < InputDataIndex; i++)
            {
                Seg_Buf[7 - i] = InputData[InputDataIndex - i - 1];
            }
        }
        break;
    case 2:
        /* 记录 */
        Seg_Buf[0] = 15; // E
        Seg_Buf[1] = Seg_Buf[2] = 10;
        Seg_Buf[3] = InputTime[0] / 16;
        Seg_Buf[4] = InputTime[0] % 16;
        Seg_Buf[5] = 21; //-
        Seg_Buf[6] = InputTime[1] / 16;
        Seg_Buf[7] = InputTime[1] % 16;
        break;
    }
}

/* LED处理函数 */
void Led_Proc()
{
	unsigned char i;
	for(i = 0;i < 3;i++)
	{
		ucLed[i] = SegMode == i? 1 : 0;
	}
  ucLed[3] = DataUpFlag;
}

/* 定时器0中断初始化 */
void Timer0_Init(void) // 1毫秒@12.000MHz
{
    AUXR &= 0x7F; // 定时器时钟12T模式
    TMOD &= 0xF0; // 设置定时器模式
    TL0 = 0x18;   // 设置定时初始值
    TH0 = 0xFC;   // 设置定时初始值
    TF0 = 0;      // 清除TF0标志
    TR0 = 1;      // 定时器0开始计时
    ET0 = 1;
    EA = 1;
}

/* 定时器0中断函数 */
void Timer0_ISR(void) interrupt 1
{
    if (++Key_Slow_Down == 10)
        Key_Slow_Down = 0;
    if (++Seg_Slow_Down == 200)
        Seg_Slow_Down = 0;
    if (++Seg_Pos == 8)
        Seg_Pos = 0;
    Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], Seg_Point[Seg_Pos]);
    Led_Disp(Seg_Pos, ucLed[Seg_Pos]);
}

void main()
{
    System_Init();
    Timer0_Init();
    Set_Rtc(ucRtc);
    EEPROM_Read(EEPROMDataOld, 0, 4);
    DataOld = EEPROMDataOld[2] << 8 | EEPROMDataOld[3];
    while (1)
    {
        Key_Proc();
        Seg_Proc();
        Led_Proc();
    }
}