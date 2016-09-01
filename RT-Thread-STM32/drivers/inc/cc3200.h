#ifndef __CC3200_H
#define __CC3200_H


#include <rtthread.h>
#include "stm32f10x.h"

#define Link_wait_time		30
#define Cfg_over_time		120

#define CC3200_RST_PORT GPIOC
#define CC3200_RST_Pin	GPIO_Pin_13

#define CC3200_Link_PORT	GPIOC
#define CC3200_Link_Pin		GPIO_Pin_3

#define CC3200_Socket_PORT	GPIOC
#define CC3200_Socket_Pin	GPIO_Pin_2

#define CC3200_Smart_PORT	GPIOC
#define CC3200_Smart_Pin	GPIO_Pin_1

extern u16 point_datas[12];
extern u8 MAC_BUF[15];
extern u8 socket_start;

void cc3200_init(void);
void cc3200_start(void);
u8 cc3200_smartcfg(u16 wait_time, u16 over_time);
unsigned char cc3200_get_MAC(u8 *macbuf, u8* MAC_BUF);
void cc3200_get_staip(u8* ipbuf);
void cc3200_socket(void);
void cc3200_reconnect(void);
u8 cc3200_send_stringdata(u8 *data,u8 *ack,u16 waittime);
void USART2_SendData(rt_uint16_t* data, u16 len);
void Respond_data(void);
void wifi_reset(void);

void cc3200_set_udpPort(void);
void cc3200_set_udpIP(void);

#endif
