/*ͷ�ļ�������*/
//����IICͨ��,Onewire,�����������������̣�����ܣ�����ͨ�ŵ�
#include "Init.h"//ϵͳ��ʼ��
#include "LED.h"//led����
#include "Key.h"//�������
#include "Seg.h"//�����
#include "ds1302.h"//ϵͳʱ��
#include "iic.h"//IICͨ��
#include "intrins.h"
#include "STDIO.h"
#include "STRING.h"

/*����������*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//����״̬����
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//�������ʾ����
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};//�����С��������
unsigned char Seg_Pos;//�����ɨ��λ��
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//
unsigned char Uart_Recv[10];//���ڽ������ݻ�����
unsigned char Uart_Recv_Index;//���ڽ�����������
unsigned char ucRtc[3] = {23,59,6};//ʱ������
unsigned int Slow_Down;//���ټ�ʱ��
bit Seg_Flag,Key_Flag,Uart_Flag;//����ܺͰ�����־λ
unsigned char SegMode;
unsigned int LightVolt;
unsigned int RB2Volt ;



/*���̴�����*/
//���������룬��ⰴ�������غ��½��أ������°���״̬
void Key_proc(void)
{
	if(Key_Flag) return;
	Key_Flag = 1; //���ñ�־λ����ֹ�ظ�����
	
	Key_Val = Key_Read(); //��ȡ����
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//���������
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//����½���
	Key_Old = Key_Val;//���°���״̬
	
	if(Key_Down == 4)
	{
		SegMode ^= 1;
	}
}

/*��Ϣ������*/
//�����������ʾ����
void Seg_proc(void)
{
	if(Seg_Flag) return;
	Seg_Flag = 1;//���ñ�־λ
	
	Read_Rtc(ucRtc);
	LightVolt = Ad_Read(0x43) * 100 / 51;
	RB2Volt = Ad_Read(0x41) * 100 / 51;
	switch(SegMode)
	{
		case 0:
			Seg_Buf[0] = ucRtc[0] / 10;
			Seg_Buf[1] = ucRtc[0] % 10;
			Seg_Buf[2] = 21;
			Seg_Buf[3] = ucRtc[1] / 10;
			Seg_Buf[4] = ucRtc[1] % 10;
			Seg_Buf[5] = 21;
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
	}
	
}

/*������ʾ����*/
//LED��ʾ����
void LED_Proc(void)
{
	
}	





void Timer1_Init(void)		//1����@12.000MHz
{
	AUXR &= 0xBF;			//��ʱ��ʱ��12Tģʽ
	TMOD &= 0x0F;			//���ö�ʱ��ģʽ
	TL1 = 0x18;				//���ö�ʱ��ʼֵ
	TH1 = 0xFC;				//���ö�ʱ��ʼֵ
	TF1 = 0;				//���TF1��־
	TR1 = 1;				//��ʱ��1��ʼ��ʱ
	ET1 = 1;
	EA = 1;
}


/*��ʱ��1�жϷ�����*/
//��ʱ��1���жϷ����������ڸ���ϵͳʱ�ӣ����������������ʾ
void Timer1_Isr(void) interrupt 3
{
	if (++Slow_Down == 10)
	{
		Seg_Flag = Slow_Down = 0; //�����������ʾ��־λ
	}
	if (Slow_Down % 10 == 0)
	{
		Key_Flag = 0; // ���°��������־λ
	}
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//�������ʾר��
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
}




/* ������ */
// ϵͳ��ʼ�������ö�ʱ���ʹ���
void main()
{
	System_Init();	// ϵͳ��ʼ��
	Timer1_Init();	// ��ʱ����ʼ��
	Set_Rtc(ucRtc); // ʱ�ӳ�ʼ��
	while (1)
	{
		Key_Proc();	 // ��������
		Seg_Proc();	 // ����ܴ���
		Led_Proc();	 // led����
	}
}
