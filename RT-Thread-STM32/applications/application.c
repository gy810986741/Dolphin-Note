/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 * 2013-07-12     aozima       update for auto initial.
 */

/**
 * @addtogroup STM32
 */
/*@{*/

#include <board.h>
#include <rtthread.h>

#ifdef  RT_USING_COMPONENTS_INIT
#include <components.h>
#endif  /* RT_USING_COMPONENTS_INIT */

#ifdef RT_USING_DFS
/* dfs filesystem:ELM filesystem init */
#include <dfs_elm.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#endif

#ifdef RT_USING_RTGUI
#include <rtgui/rtgui.h>
#include <rtgui/rtgui_server.h>
#include <rtgui/rtgui_system.h>
#include <rtgui/driver.h>
#include <rtgui/calibration.h>
#endif

#include "led.h"
#include "ledseq.h"
#include "cc3200.h"
#include "Screen.h"
#include "wifi.h"
#include "usart2.h"
#include "spi_flash.h"
#include "key.h"
#include "prome_stat.h"

#define KEY_net_long_push 0x01
#define KEY_net_short_push 0x02
#define KEY_rec_long_push 0x04
#define KEY_rec_short_push 0x08
#define KEY_snp_long_push 0x10
#define KEY_snp_short_push 0x20
//#define key_net_long_push 0x1

unsigned char F_key_state = 0x00;
unsigned char F_key_state_pre = 0x00;
unsigned int F_key_longpush = 0;
//unsigned char KEY_STATE_ = 0x00;

ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t led_stack[ 512 ];
static struct rt_thread led_thread;
static void led_thread_entry(void* parameter)
{
    rt_hw_led_init();
	ledseq_init();
	ledseq_run(LED_GREEN, seq_power_on);
//	ledseq_run(LED_SNP, seq_flash_3_times);
//	rt_sem_take(&Net_complete_seqSem, RT_WAITING_FOREVER);
	heart_beat_lose_cnt = 0;
    while (1)
    {
		if(net_complete_flag)
		{
			heart_beat_lose_cnt ++;
			if(heart_beat_lose_cnt >= 4)
			{
				heart_beat_lose_cnt = 0;
				net_complete_flag = 0;
			}
		}
        rt_thread_delay( 1000 ); /* sleep 1 second and switch to other thread */
    }
}

ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t cc3200_stack[ 2048 ];
static struct rt_thread cc3200_thread;
static void cc3200_thread_entry(void* parameter)
{
	static unsigned char Main_Fuction_state = 0;
	rt_sem_init(&Net_complete_seqSem , "netseqSem", 0, RT_IPC_FLAG_FIFO);//初始化信号量，初始值为1，该信号量会被第一个持有的线程清除
	cc3200_init();
	cc3200_get_MAC(MAC_BUF, Bar_code);
//	SPI_FLASH_Init();
//	SPI_FLASH_ChipErase();
//	ScreenRecSeqInit();
	while(1)
	{
		switch(Main_Fuction_state)
		{
			case 0://等待接入wifi
				cc3200_start();
				Main_Fuction_state = 1;
				break;
			case 1://等待手机连接
				cc3200_set_udpPort();
				Main_Fuction_state = 2;
				break;
			case 2://等待手机连接
				if((F_key_state & KEY_net_long_push) == KEY_net_long_push)//NET键长按三秒，发起断开连接请求
				{
					F_key_state = 0;//F_key_state & (KEY_net_long_push ^ 0x00);
					ledseq_stop(LED_NET, seq_alive);
					wifi_reset();
					Main_Fuction_state = 0;
				}	
				if(searching_flag == 1)
				{
					searching_flag = 0;
					Uart2_Put_Buf(wifi_data_to_send , 17);
				}				
				if(!(Phone_IP[0] == 0))
					Main_Fuction_state = 3;
				break;
			case 3://等待手机连接
				cc3200_set_udpIP();
				Main_Fuction_state = 4;
				break;
			case 4://等待手机连接
				
				if(net_complete_flag == 1)
				{
					Uart2_Put_Buf(wifi_data_to_send , 17);
					Main_Fuction_state = 5;
					//ledseq_run(LED_RED, seq_error);
					ledseq_run(LED_SNP, seq_power_on);
					ledseq_run(LED_REC, seq_power_on);
					ledseq_run(BEEP, seq_testPassed);
					rt_sem_release(&Net_complete_seqSem);
				}
				break;
			case 5:
				if(net_complete_flag == 1)
				{
					if(pen_data_ready)
					{
						pen_data_ready = 0;
						if(up_down_flag)
						{
							Uart2_Put_Buf(wifi_data_to_send , 9 + (wifi_data_len * 12));
						}
					}
					if(is_idle_flag)
					{
						is_idle_flag = 0;
						heart_beat_lose_cnt = 0;
						Uart2_Put_Buf(wifi_data_to_send , 11);
					}
					if((F_key_state & KEY_net_long_push) == KEY_net_long_push)//NET键长按三秒，发起断开连接请求
					{
						F_key_state = 0;//F_key_state & (KEY_net_long_push ^ 0x00);
						wifi_ack_send(0xA1, 0x0000);
						Uart2_Put_Buf(wifi_data_to_send , 11);
					}
					if((F_key_state & KEY_snp_short_push) == KEY_snp_short_push)//SNP键短按，发起截屏请求
					{
						F_key_state = 0;//F_key_state & (KEY_snp_short_push ^ 0x00);
						ledseq_run(LED_SNP, seq_flash_3_times);
						wifi_ack_send(0x70, 0x0000);
						Uart2_Put_Buf(wifi_data_to_send , 11);
					}
					if(meeting_end_flag)//接收到会议结束指令，收到手机确认信息后断开网络
					{
						meeting_end_flag = 0;
						net_complete_flag = 0;
						is_First_connect = 1;
						Phone_IP[0] = 0;
						Phone_IP[1] = 0;
						Phone_IP[2] = 0;
						Phone_IP[3] = 0;
						ledSet(LED_REC, false);
						ledSet(LED_SNP, false);
//						ledseq_run(LED_REC, seq_power_on);
//						ledseq_stop(LED_SNP, seq_flash_3_times);
						ledseq_stop(LED_NET, seq_alwayson);
						ledseq_run(LED_NET, seq_alive);
						Main_Fuction_state = 1;
						break;
					}
					if(time_checking_flag)
					{
						time_checking_flag = 0;
						wifi_ack_send(0x74, 0x0000);
						Uart2_Put_Buf(wifi_data_to_send , 13);
					}
				}
				else if(net_complete_flag == 0)
				{//断网后，进入连接响应程序，此时需要判断请求连接的白板ID-->手机ID-->判断IP是否改变-->是否需要重新配置UDPC
					ledSet(LED_REC, false);
					ledSet(LED_SNP, false);
					ledseq_stop(LED_NET, seq_alwayson);
					ledseq_run(LED_NET, seq_alive);
					cc3200_reconnect();
					ledseq_run(LED_SNP, seq_power_on);
					ledseq_run(LED_REC, seq_power_on);
					ledseq_stop(LED_NET, seq_alive);
					ledseq_run(LED_NET, seq_alwayson);
					Uart2_Put_Buf(wifi_data_to_send , 17);
				}
				else if(net_complete_flag == 2)
				{
					
				}
				break;
			default :
				break;
		}
		rt_thread_delay(1);
	}
//	while(cc3200_get_staip(Board_IP))
//	{
//		rt_thread_delay(1);
//	}	
//	while(1)
//	{
//		if(net_reset_flag == 1)
//		{
//			net_reset_flag = 0;
//			wifi_reset();
//			while(cc3200_get_staip(Board_IP))
//			{
//				rt_thread_delay(1);
//			}	
//		}
//		cc3200_socket();

//		while(1)
//		{
//			//第一次网络配置完成后跳出该循环
//			if(net_complete_flag == 1)
//			{
//				Uart2_Put_Buf(wifi_data_to_send , 17);
//				break;
//			}
//			rt_thread_delay(1);
//		}
//		ledseq_run(LED_RED, seq_error);
//		ledseq_run(LED_SNP, seq_power_on);
//		ledseq_run(LED_REC, seq_power_on);
//		ledseq_run(BEEP, seq_testPassed);
//		rt_sem_release(&Net_complete_seqSem);
//		while(1)
//		{		
//			if(net_complete_flag == 1)
//			{
//				if(pen_data_ready)
//				{
//					pen_data_ready = 0;
//					if(up_down_flag)
//					{
//						Uart2_Put_Buf(wifi_data_to_send , 9 + (wifi_data_len * 12));
//					}
//				}
//				if(is_idle_flag)
//				{
//					is_idle_flag = 0;
//					heart_beat_lose_cnt = 0;
//					Uart2_Put_Buf(wifi_data_to_send , 11);
//				}
//				if((F_key_state & KEY_net_long_push) == KEY_net_long_push)//NET键长按三秒，发起断开连接请求
//				{
//					F_key_state = F_key_state & (KEY_net_long_push ^ 0x00);
//					wifi_ack_send(0xA1, 0x0000);
//					Uart2_Put_Buf(wifi_data_to_send , 11);
//				}
//				if((F_key_state & KEY_snp_short_push) == KEY_snp_short_push)//SNP键短按，发起截屏请求
//				{
//					F_key_state = F_key_state & (KEY_snp_short_push ^ 0x00);
//					ledseq_run(LED_SNP, seq_flash_3_times);
//					wifi_ack_send(0x70, 0x0000);
//					Uart2_Put_Buf(wifi_data_to_send , 11);
//				}
//				if(meeting_end_flag)//接收到会议结束指令，收到手机确认信息后断开网络
//				{
//					meeting_end_flag = 0;
//					net_complete_flag = 0;
//					Phone_IP[0] = 0;
//					Phone_IP[1] = 0;
//					Phone_IP[2] = 0;
//					Phone_IP[3] = 0;
//					ledseq_stop(LED_NET, seq_power_on);
//					ledseq_run(LED_NET, seq_alive);
//					break;
//				}
//			}
//			else if(net_complete_flag == 0)
//			{//断网后，进入连接响应程序，此时需要判断请求连接的白板ID-->手机ID-->判断IP是否改变-->是否需要重新配置UDPC
//				
//				ledseq_stop(LED_NET, seq_power_on);
//				ledseq_run(LED_NET, seq_alive);
//				cc3200_reconnect();
//				ledseq_stop(LED_NET, seq_alive);
//				ledseq_run(LED_NET, seq_power_on);
//				Uart2_Put_Buf(wifi_data_to_send , 17);
//			}
//			else if(net_complete_flag == 2)
//			{
//				
//			}
//			rt_thread_delay(1);
//		}
//	}
}

ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t Screen_stack[ 1024 ];
static struct rt_thread Screen_thread;
static void Screen_thread_entry(void* parameter)
{
//	rt_sem_take(&Net_complete_seqSem, RT_WAITING_FOREVER);
	uart1_init(115200);
//	SPI_FLASH_Init();
//	SPI_FLASH_ChipErase();
//	ScreenRecSeqInit();
	while(1)
	{
//		time_base ++;
		rt_thread_delay(10);
	}
}
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t Key_stack[ 512 ];
static struct rt_thread Key_thread;

static void Key_thread_entry(void* parameter)
{
//	unsigned char F_key_state = 0x00;
//	unsigned int key_time = 0;
	static unsigned char key_lock_flag = 1;
	key_init();

//	bsp_InitLed();
//	bsp_InitKey();
//    
	while(1)
	{
//		F_key_state = key_scan();
		switch(key_scan())
		{
			case 0x12:
				if(key_lock_flag == 0)
				{
					F_key_state = F_key_state | KEY_rec_short_push;
					key_lock_flag = 1;
				}
				break;
			case 0x22:
				if(key_lock_flag == 0)
				{
					F_key_state = F_key_state | KEY_snp_short_push;
					key_lock_flag = 1;
				}
				break;
			case 0x32:
				if(key_lock_flag == 0)
				{
					F_key_state = F_key_state | KEY_net_short_push;
					key_lock_flag = 1;
				}
				break;
			case 0x33://NET长按
				if(key_lock_flag == 0)
				{
					F_key_state = F_key_state | KEY_net_long_push;
					key_lock_flag = 1;
				}
				break;
			case 0x23:
				if(key_lock_flag == 0)
				{
					F_key_state = F_key_state | KEY_snp_long_push;
					key_lock_flag = 1;
				}
				break;
			case 0x13:
				if(key_lock_flag == 0)
				{
					F_key_state = F_key_state | KEY_rec_long_push;
					key_lock_flag = 1;
				}
				break;
			default:
				key_lock_flag = 0;
				//F_key_longpush = 0;
				break;
		}
//		/* ???? */
//		bsp_KeyScan();
//        /* LED?? */
//        bsp_LedScan();
        
//        rt_thread_delay(10);
	}
}

//ALIGN(RT_ALIGN_SIZE)
//static rt_uint8_t Screen_stack[ 512 ];
//static struct rt_thread Screen_thread;
//static void Screen_thread_entry(void* parameter)
//{
//	uart1_init(115200);
//	while(1)
//	{
//		time_base ++;
//			rt_thread_delay(10);
//	}
//}

void rt_init_thread_entry(void* parameter)
{
#ifdef RT_USING_COMPONENTS_INIT
    /* initialization RT-Thread Components */
    rt_components_init();
#endif

#ifdef  RT_USING_FINSH
    finsh_set_device(RT_CONSOLE_DEVICE_NAME);
#endif  /* RT_USING_FINSH */

}

int rt_application_init(void) 
{
    rt_thread_t init_thread;

    rt_err_t result;

    /* init led thread */
    result = rt_thread_init(&led_thread, "led", led_thread_entry, RT_NULL, (rt_uint8_t*)&led_stack[0], sizeof(led_stack), 20, 5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&led_thread);
    }
		
    	/* init key thread */
    result = rt_thread_init(&Key_thread, "Key", Key_thread_entry, RT_NULL, (rt_uint8_t*)&Key_stack[0], sizeof(Key_stack), 17, 5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&Key_thread);
    }
		/* init cc3200 thread */
    result = rt_thread_init(&cc3200_thread, "cc3200", cc3200_thread_entry, RT_NULL, (rt_uint8_t*)&cc3200_stack[0], sizeof(cc3200_stack), 19, 5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&cc3200_thread);
    }
		
		/* init screen thread */
    result = rt_thread_init(&Screen_thread, "Screen", Screen_thread_entry, RT_NULL, (rt_uint8_t*)&Screen_stack[0], sizeof(Screen_stack), 18, 5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&Screen_thread);
    }

#if (RT_THREAD_PRIORITY_MAX == 32)
    init_thread = rt_thread_create("init", rt_init_thread_entry, RT_NULL, 2048, 8, 20);
#else
    init_thread = rt_thread_create("init", rt_init_thread_entry, RT_NULL, 2048, 80, 20);
#endif

    if (init_thread != RT_NULL)
        rt_thread_startup(init_thread);

    return 0;
}

/*@}*/
