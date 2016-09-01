#include "cc3200.h"
#include "usart2.h"
#include "Screen.h"
#include "wifi.h"
#include <string.h>
#include <stdio.h>
#include "ledseq.h"

u8 socket_start = 0;
u8 Pre_Phone_IP[4] = {0};
u8 RT_Phone_IP[16] = {0};
u8 MAC_BUF[15] = {0};
const u8* Local_Port  = "9002";		// 本地端口号
const u8* PORT_Server = "9001";		// 远程端口号
const u8* IP_Server   = "192.168.100.255";		// 远程IP
const u8* PORT = "0";
const u8* IP   = "192.168.100.255";
const u8 *CC3200_WORKMODE_TBL[4]={"TCPS", "TCPC", "UDPS", "UDPC"};		//4种工作模式

//static void* timer_heartbeat;//定义空地址指针，指向四个定时器句柄地址，节省空间

void cc3200_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
		
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
	
	usart2_init(115200);
}

//////////////字节发送//////////////////////////
void USART2_SendByte(rt_uint8_t Data)
{
	//等待发送数据寄存器空
	while (!(USART2->SR & USART_FLAG_TXE));	//数据未转移到移位寄存器
	//发送数据
	USART2->DR = (Data & 0x00FF);
	//等待发送完成
	while (!(USART2->SR & USART_FLAG_TC));   
}

//////////////发送AT指令////////////////////////////
void USART2_Send_ATCmd(rt_uint8_t *data)
{    
  	rt_uint16_t i,len;
	
	len = strlen((const char*)data);
		
	for (i=0; i<len; i++)
	  USART2_SendByte(data[i]);	     
}

/**************************************************
* CC3200发送命令后,检测接收到的应答
* str:	期待的应答结果
* 返回值: 0,没有得到期待的应答结果
*     		其他,期待应答结果的位置(str的位置)
**************************************************/
u8* cc3200_check_cmd(u8 *str)
{
	
	char *strx=0;
	if(USART2_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//添加结束符
		strx=strstr((const char*)USART2_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}

/**************************************************
*函数功能：向CC3200发送命令
*cmd: 发送的命令字符串
*ack: 期待的应答结果,如果为空,则表示不需要等待应答
*waittime: 等待时间(单位:10ms)
*返回值: 0,发送成功(得到了期待的应答结果)
*        1,发送失败
**************************************************/
u8 cc3200_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART2_RX_STA=0;
	USART2_Send_ATCmd(cmd);	//发送命令
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
		{
			rt_thread_delay(10);
			if(USART2_RX_STA&0X8000)//接收到期待的应答结果
			{
				if(cc3200_check_cmd(ack))
				{
					break;//得到有效数据 
				}
					USART2_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
} 

/*******************************************************************************
* 函数名  : cc3200_Hardware_Reset
* 描述    : 硬件复位cc3200
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 复位后等待 3S 后的应答信号
*******************************************************************************/
void cc3200_Hardware_Reset(void)
{
	GPIO_ResetBits(CC3200_RST_PORT, CC3200_RST_Pin);//复位引脚拉低
	rt_thread_delay(500);
	GPIO_SetBits(CC3200_RST_PORT, CC3200_RST_Pin);//复位引脚拉高
	rt_thread_delay(5000);
}

/*******************************************************************************
* 函数名  : wifi_reset
* 描述    : 重置 cc3200 配置
* 返回值  : 无
* 说明    : 必须给足够的延时时间，不然可能无法响应
*******************************************************************************/
void wifi_reset(void)
{
	socket_start = 0;
	cc3200_Hardware_Reset();
	cc3200_send_cmd("+++", "Switch AT Command Mode!", 100);
	rt_thread_delay(3000);
	sprintf((char*)USART2_TX_BUF, "%s", "AT+RESET");
	cc3200_send_cmd(USART2_TX_BUF, "Reset", 150);		// "Reset All Config!"
	rt_thread_delay(5000);
	rt_thread_delay(5000);			// 足够的延时，等待响应
	cc3200_Hardware_Reset();		// 重启有效，不可屏蔽
}

/*******************************************************************************
* 函数名  : cc3200_start
* 描述    : cc3200 启动
* 返回值  : 无
* 说明    : 获取 IP, 建立 socket 都需要在连入 wifi 的前提下，否则无法通信
*******************************************************************************/
void cc3200_start(void)
{
	while(cc3200_smartcfg(Link_wait_time,Cfg_over_time))	// 等待和超时时间由宏定义给出
	{
		rt_thread_delay(1);		// 连入 WiFi 时, 跳出等待
	}
	cc3200_get_staip(Board_IP);		// 连入 WiFi 后, 获取白板 IP
}

/*******************************************************************************
* 函数名  : cc3200_smartcfg
* 描述    : 一键配置cc3200
* 返回值  : 0：成功，1：失败
* 说明    : wait_time：连接等待时间; over_time: 配置连接超时时间，单位 1S
*******************************************************************************/
u8 cc3200_smartcfg(u16 wait_time, u16 over_time)
{
	u8 time=0;
	socket_start = 0;
	cc3200_Hardware_Reset();
	cc3200_send_cmd("+++", "Switch AT Command Mode!", 50);
	if(cc3200_send_cmd("AT+ROLE=?", "WiFi Role = STA.", 50) == 0)// 充分延时，使数据接收完
	{	
		while(wait_time--)
		{
			rt_thread_delay(1000);
			if(GPIO_ReadInputDataBit(CC3200_Link_PORT, CC3200_Link_Pin) == 0)
				break;
			
			if(wait_time == 0)
			{
				sprintf((char*)USART2_TX_BUF, "%s", "AT+STA=ff");					// 每次 smartconfig 时，清空列表
				cc3200_send_cmd(USART2_TX_BUF, "Del All STA Profile", 100);
				cc3200_send_cmd("AT+SL=smartcfg", "Enter Smartconfig Mode...", 100);
			}
		}
	}
	else
	{
		rt_thread_delay(5000);	// 充分延时, 否则可能配置不成功
		cc3200_send_cmd("AT+ROLE=STA", "Change WiFi Role = STA.", 100);		// 必须要保证配置成功
		rt_thread_delay(1000);
		cc3200_send_cmd("AT+RST", "Device Restart...", 300);
		rt_thread_delay(3000);
		cc3200_send_cmd("+++", "Switch AT Command Mode!", 50);
		rt_thread_delay(2000);
		sprintf((char*)USART2_TX_BUF, "%s", "AT+STA=ff");					// 每次 smartconfig 时，清空列表
		cc3200_send_cmd(USART2_TX_BUF, "Del All STA Profile", 100);
		cc3200_send_cmd("AT+SL=smartcfg", "Enter Smartconfig Mode...", 100);
	}
//	ledseq_run(LED_REC, seq_alive);
	
	if(GPIO_ReadInputDataBit(CC3200_Smart_PORT, CC3200_Smart_Pin) == 0)
	{
		while(GPIO_ReadInputDataBit(CC3200_Link_PORT, CC3200_Link_Pin))			// 等待连接
		{
			rt_thread_delay(1000);
			time++;
			if(time >= over_time)
				return 1;
		}
	}
//	ledseq_stop(LED_REC, seq_alive);
//	ledseq_run(LED_REC, seq_power_on);
	return 0;
}  

/*******************************************************************************
* 函数名  : ASCII_to_binary
* 描述    : ASCII 转换成 二进制
* 返回值  : int型转换结果
* 说明    : 
*******************************************************************************/
u8 ASCII_to_binary(u8* data, u8 len)
{
	u8 i, data_t=0;
	for(i=0;i<len;i++)
	{
		data_t *= 10;
		data_t += data[i] - '0';
	}
	return data_t;
}

/*******************************************************************************
* 函数名  : cc3200_get_staip
* 描述    : 获取IP 地址
* 返回值  : 无
* 说明    : ipbuf: IP 缓冲区
*******************************************************************************/
void cc3200_get_staip(u8* Local_IP)
{
	u8 *p, *p1, *p2, *p3, *p4;
	u8 Local_IP0[3] = {0};
	u8 Local_IP1[3] = {0};
	u8 Local_IP2[3] = {0};
	u8 Local_IP3[3] = {0};
	if(cc3200_send_cmd("AT+IP=?", "DHCP", 50) == 0)
	{
		p  = cc3200_check_cmd(",");
		p1 = (u8*)strstr((const char*)(p+1), ".");
		*p1= 0;
		sprintf((char*)Local_IP0, "%s", p+1);
		Local_IP[0] = ASCII_to_binary(Local_IP0, p1-p-1);

		p2 = (u8*)strstr((const char*)(p1+1), ".");
		*p2= 0;
		sprintf((char*)Local_IP1, "%s", p1+1);
		Local_IP[1] = ASCII_to_binary(Local_IP1, p2-p1-1);

		p3 = (u8*)strstr((const char*)(p2+1), ".");
		*p3= 0;
		sprintf((char*)Local_IP2, "%s", p2+1);
		Local_IP[2] = ASCII_to_binary(Local_IP2, p3-p2-1);

		p4 = (u8*)strstr((const char*)(p3+1), ",");
		*p4= 0;
		sprintf((char*)Local_IP3, "%s", p3+1);
		Local_IP[3] = ASCII_to_binary(Local_IP3, p4-p3-1);
	}
}

/*******************************************************************************
* 函数名  : cc3200_soclet1
* 描述    : 前一次建立socket, 获取服务器 IP
* 返回值  : 无
* 说明    : 初始模式：UDPC,9002,0.0.0.0,9001
*******************************************************************************/
void cc3200_set_udpPort(void)
{
	u8 Radio_IP[16] = {0};
	socket_start = 0;
	cc3200_send_cmd("+++", "Switch AT Command Mode!", 50);
	sprintf((char*)Radio_IP, "%d.%d.%d.%d", Board_IP[0],Board_IP[1],Board_IP[2],255);
	sprintf((char*)USART2_TX_BUF, "AT+SOCK=%s,%s,%s,%s", CC3200_WORKMODE_TBL[3], Local_Port, Radio_IP, PORT_Server);
	cc3200_send_cmd(USART2_TX_BUF, "Set Socket CFG:", 100);
	socket_start = 1;
//	rt_thread_delay(1500);
	cc3200_send_cmd("AT+RST", "Device Restart...", 300);
	rt_thread_delay(3000);
	while(GPIO_ReadInputDataBit(CC3200_Socket_PORT, CC3200_Socket_Pin) == 1)
	{
		rt_thread_delay(1);
	}
	ledseq_stop(LED_NET, seq_alive);
	ledseq_run(LED_NET, seq_alive);
	
}

/*******************************************************************************
* 函数名  : cc3200_soclet2
* 描述    : 后一次建立socket, 建立通信
* 返回值  : 无
* 说明    : 重启后生效，等待连入网络，大约 8s 左右
*******************************************************************************/
void cc3200_set_udpIP(void)
{
	u8 i;
	for(i=0;i<4;i++)
		Pre_Phone_IP[i] = Phone_IP[i];

	sprintf((char*)RT_Phone_IP, "%d.%d.%d.%d", Phone_IP[0],Phone_IP[1],Phone_IP[2],Phone_IP[3]);

	socket_start = 0;
	cc3200_send_cmd("+++", "Switch AT Command Mode!", 50);
	sprintf((char*)USART2_TX_BUF, "AT+SOCK=%s,%s,%s,%s", CC3200_WORKMODE_TBL[3], Local_Port, RT_Phone_IP, PORT_Server);
	cc3200_send_cmd(USART2_TX_BUF, "Set Socket CFG:", 100);	
	socket_start = 1;
	cc3200_send_cmd("AT+RST", "Device Restart...", 300);
	rt_thread_delay(3000);
	while(GPIO_ReadInputDataBit(CC3200_Link_PORT, CC3200_Link_Pin) == 1)
	{
		rt_thread_delay(1);
	}

//	ledseq_stop(LED_NET, seq_alive);
	ledseq_stop(LED_NET, seq_alwayson);
	ledseq_run(LED_NET, seq_alwayson);
	net_complete_flag = 1;
}

/*******************************************************************************
* 函数名  : cc3200_socket
* 描述    : 建立 socket
* 返回值  : 无
* 说明    : 重启后生效，等待连入网络，大约 8s 左右
*******************************************************************************/
void cc3200_socket(void)
{
//	u8 wait_time = 0;
	u8 i;
	sprintf((char*)USART2_TX_BUF, "AT+SOCK=%s,%s,%s,%s", CC3200_WORKMODE_TBL[3], Local_Port, IP, PORT_Server);
	cc3200_send_cmd(USART2_TX_BUF, "Set Socket CFG:", 100);
	socket_start = 1;
	cc3200_send_cmd("AT+RST", "Device Restart...", 300);
	rt_thread_delay(3000);
	while(GPIO_ReadInputDataBit(CC3200_Socket_PORT, CC3200_Socket_Pin) == 1)
	{
		rt_thread_delay(1);
	}
	ledseq_stop(LED_NET, seq_alive);
	ledseq_run(LED_NET, seq_alive);
	
	while(Phone_IP[0] == 0)	//这个地方是用来等待第一次连接的过程中接收到IP数据，同时也解决了第一次连接的时候，
							//如果白板ID不对，那么在协议解析的时候，不会给Phone_IP数组赋值，
							//也就是不会相应错误的白板ID连接请求
	{
		rt_thread_delay(1);
	}

	for(i=0;i<4;i++)
		Pre_Phone_IP[i] = Phone_IP[i];

	sprintf((char*)RT_Phone_IP, "%d.%d.%d.%d", Phone_IP[0],Phone_IP[1],Phone_IP[2],Phone_IP[3]);

	socket_start = 0;
	cc3200_send_cmd("+++", "Switch AT Command Mode!", 50);
	sprintf((char*)USART2_TX_BUF, "AT+SOCK=%s,%s,%s,%s", CC3200_WORKMODE_TBL[3], Local_Port, RT_Phone_IP, PORT_Server);
	cc3200_send_cmd(USART2_TX_BUF, "Set Socket CFG:", 100);	
	socket_start = 1;
	cc3200_send_cmd("AT+RST", "Device Restart...", 300);
	rt_thread_delay(3000);
	while(GPIO_ReadInputDataBit(CC3200_Link_PORT, CC3200_Link_Pin) == 1)
	{
		rt_thread_delay(1);
	}

//	ledseq_stop(LED_NET, seq_alive);
	ledseq_stop(LED_NET, seq_power_on);
	ledseq_run(LED_NET, seq_power_on);
	net_complete_flag = 1;
}
void cc3200_reconnect(void)
{
	unsigned char i = 0, j = 0;
	unsigned char is_same_IP = 1;
	while(1)
	{
		if(net_reconnect_flag)
		{
			net_reconnect_flag = 0;
			for(i = 0; i < 4; i++)//判断手机ID是否正确
			{
				if(Pre_Phone_IP[i] != Phone_IP[i])
				{
					for(j = 0; j < 4; j++)
						Pre_Phone_IP[j] = Phone_IP[j];
					is_same_IP = 0;
					break;
				}
			}
			if(is_same_IP)
			{
				net_complete_flag = 1;
				break;
			}
			else
			{
				sprintf((char*)RT_Phone_IP, "%d.%d.%d.%d", Phone_IP[0],Phone_IP[1],Phone_IP[2],Phone_IP[3]);
				socket_start = 0;
				cc3200_send_cmd("+++", "Switch AT Command Mode!", 50);
				sprintf((char*)USART2_TX_BUF, "AT+SOCK=%s,%s,%s,%s", CC3200_WORKMODE_TBL[3], Local_Port, RT_Phone_IP, PORT_Server);
				cc3200_send_cmd(USART2_TX_BUF, "Set Socket CFG:", 100);	
				socket_start = 1;
				cc3200_send_cmd("AT+RST", "Device Restart...", 300);
				rt_thread_delay(3000);
				while(GPIO_ReadInputDataBit(CC3200_Link_PORT, CC3200_Link_Pin) == 1)
				{
					rt_thread_delay(1);
				}
				net_complete_flag = 1;
				break;
			}
		}
		rt_thread_delay(1);
	}
}

/*******************************************************************************
* 函数名  : socket_stop
* 描述    : 断开手机与白板连接
* 返回值  : 无
* 说明    : 设置为出厂模式 TCPS,9002,0.0.0.0,0
*******************************************************************************/
void socket_stop(void)
{
	socket_start = 0;
	cc3200_send_cmd("+++", "Switch AT Command Mode!", 50);
	rt_thread_delay(100);
	sprintf((char*)USART2_TX_BUF, "AT+SOCK=%s,%s,%s,%s", CC3200_WORKMODE_TBL[0], Local_Port, IP, PORT);		// TCPS,9002,0.0.0.0,0
	cc3200_send_cmd(USART2_TX_BUF, "Set Socket CFG:", 100);	
	socket_start = 1;
	rt_thread_delay(100);
	cc3200_send_cmd("AT+RST", "Device Restart...", 300);
	rt_thread_delay(3000);
}
void MAC_to_binary(unsigned char* data, unsigned char* mac_buf)	
{
	unsigned char i, j, data_t=0;
	for(j = 0; j < 6; j++)
	{
		for(i = 0; i < 2; i++)
		{
			if(data[i + j * 2] > 64 && data[i + j * 2] < 71)
			{
				data_t *= 16;
				data_t += data[i + j * 2] - '7';	// 大写字母转换成16进制
			}
			else if(data[i + j * 2] > 47 && data[i + j * 2] < 58)
			{
				data_t *= 16;
				data_t += data[i + j * 2] - '0'; 
			}
		}
		mac_buf[j] = data_t; 
		data_t = 0;
	}
}
/*******************************************************************************
* 函数名  : cc3200_get_MAC
* 描述    : 获取 MAC
* 返回值  : 无
* 说明    : 参数一 字符型，参数二 十六进制
*******************************************************************************/
unsigned char cc3200_get_MAC(unsigned char *macbuf, unsigned char* bar_buf)		
{
	unsigned char *p, *p1;
	socket_start = 0;
	cc3200_Hardware_Reset();
	cc3200_send_cmd("+++", "Switch AT Command Mode!", 100);
	rt_thread_delay(1500);
	if(cc3200_send_cmd("AT+MAC", "Chip", 100) == 0)
	{
		p = cc3200_check_cmd("0x");
		p1 = (unsigned char*)strstr((const char*)(p + 2), "\r");
		*p1 = 0;		// 添加结束符
		sprintf((char*)macbuf, "%s", p + 2);	
	}
	if((char*)macbuf == 0)
		return 1;
	else 
	{
		MAC_to_binary(macbuf, bar_buf);
		return 0;
	}
}

