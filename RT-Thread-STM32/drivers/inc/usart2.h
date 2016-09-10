#ifndef __USART2_H
#define __USART2_H	 

#include <rtthread.h>
#include "stm32f10x.h"  
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//串口3驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2014/3/29
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	   
#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define EN_USART1_RX 1

extern struct rt_semaphore Screen_seqSem;//创建LED闪烁控制信号量，该信号量和FreeRTOS中的信号量稍有区别
extern struct rt_semaphore Net_complete_seqSem;
extern u8  USART_RX_BUF[USART_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART_RX_STA;         		//接收状态标记
extern unsigned char Screen_TxBuffer[50];
extern unsigned char Screen_RxBuffer[256];
//如果想串口中断接收，请不要注释以下宏定义
void uart1_init(u32 bound);
void uart2_init(u32 bound);
void Uart1_Put_Buf(unsigned char *DataToSend , unsigned char data_num);
void Uart2_Put_Buf(unsigned char *DataToSend , unsigned char data_num);

#define USART2_MAX_RECV_LEN		200					//最大接收缓存字节数
#define USART2_MAX_SEND_LEN		200					//最大发送缓存字节数
#define USART2_RX_EN 			1					//0,不接收;1,接收.

extern u8  USART2_RX_BUF[USART2_MAX_RECV_LEN]; 		//接收缓冲,最大USART3_MAX_RECV_LEN字节
extern u8  USART2_TX_BUF[USART2_MAX_SEND_LEN]; 		//发送缓冲,最大USART3_MAX_SEND_LEN字节
extern vu16 USART2_RX_STA;   						//接收数据状态

void usart2_init(u32 bound);				//串口2初始化 

void TIM7_Int_Init(u16 arr,u16 psc);

#endif

