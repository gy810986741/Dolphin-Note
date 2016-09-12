#include "usart2.h"
#include "Screen.h"
#include <string.h>
#include "wifi.h"
#include "ledseq.h"
#include "spi_flash.h"
#include "prome_stat.h"
#define SW_VERSION	"1.0.0"
#define HW_VERSION	"1.0.0"

#define EnableIRP()   __enable_irq()
#define DisableIRP()  __disable_irq()
#define Pendata_frame_len	12
//ScreenSeq_structdef ScreenDataSeq = {0};
FlashSeq_structdef FlashScreenDataSeq = {0};
unsigned int DataFrame_in_Flash = 0;
unsigned int time_base = 0;
unsigned char up_down_flag = 0;
unsigned char pen_data_ready = 0;
unsigned char net_complete_flag = 0;
unsigned char net_reconnect_flag = 0;
unsigned char net_reset_flag = 0;
unsigned char is_idle_flag = 0;
unsigned char meeting_end_flag = 0;
unsigned char time_checking_flag = 0;
unsigned char searching_flag = 0;
unsigned char pen_data[256] = {0};//笔迹数据
unsigned char point_cnt = 0;//触摸点计数
void Screen_data_analize(unsigned char *data_buf,unsigned char num)//触屏协议处理
{
	vs16 rc_value_temp;
//	unsigned char sum = 0;
	unsigned char i = 0;
//	unsigned char point_cnt = 0;//触摸点计数

//	p = &point_data[0];
//	for(i=0;i<(num-1);i++)
//		sum += *(data_buf+i);
//	if(!(sum==*(data_buf+num-1)))		
//	{	
//		return;		//判断sum
//	}
	
	if(!(*(data_buf) == 0x68))	
	{	
		return;
	}		//判断帧头
/////////////////////////////////////////////////////////////////////////////////////
	switch(*(data_buf + 2))
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			point_cnt = (*(data_buf + 1) - 2) / 10;
			for(i = 0; i < point_cnt; i++)
			{
				pen_data[i*Pendata_frame_len + 0] =  *(data_buf + 5 + i*10);
				pen_data[i*Pendata_frame_len + 1] =  *(data_buf + 6 + i*10);
				pen_data[i*Pendata_frame_len + 2] =  *(data_buf + 7 + i*10);
				pen_data[i*Pendata_frame_len + 3] =  *(data_buf + 8 + i*10);
				pen_data[i*Pendata_frame_len + 4] =  *(data_buf + 9 + i*10);
				pen_data[i*Pendata_frame_len + 5] =  *(data_buf +10 + i*10);
				pen_data[i*Pendata_frame_len + 6] =  *(data_buf +11 + i*10);
				pen_data[i*Pendata_frame_len + 7] =  *(data_buf +12 + i*10);
				if(*(data_buf + 3 + i*10 + 0) == 0x04)//笔状态  抬起 0x04
				{
					pen_data[i*Pendata_frame_len + 8] = (*(data_buf + 4 + i*10 ) & 0x7f) | 0x80;
				}
				else if(*(data_buf + 3 + i*10 + 0) == 0x07)//笔状态  书写中 0x07
				{
					pen_data[i*Pendata_frame_len + 8] = (*(data_buf + 4 + i*10 ) & 0x7f);
					up_down_flag = 1;
				}
				pen_data[i*Pendata_frame_len + 9] =	time_base & 0x0000ff ;//时间戳
				pen_data[i*Pendata_frame_len +10] =(time_base & 0x00ff00) >> 8;
				pen_data[i*Pendata_frame_len +11] =(time_base & 0xff0000) >> 16;
			}
			ScreenRecSeqIn(pen_data, point_cnt);//入队列
			wifi_send_pen_data(pen_data, point_cnt);
			pen_data_ready = 1;
			
			break;
		case 0x60:
			up_down_flag = 0;
//			for(i = 0; i < 200; i++)
//			{
//				pen_data[i] = 0;
//			}
			break;
	}
}
/*初始化数据存储队列*/
void ScreenRecSeqInit()
{
	FlashScreenDataSeq.SeqFrontAddr = 0;		//为队列添加缓存地址
	FlashScreenDataSeq.SeqRearAddr = 0;			//将计数器清零
	FlashScreenDataSeq.SeqReadAddr = 0;
}
/*向接收队列放入一帧数据*/

//数据入队列
//入口参数：unsigned char *_DataFrame 待写入的数组
//			unsigned char _DataLen	  待写入的笔迹数据点数计数
//返回    ：state ：Error 队列满  ok 写入成功
//unsigned char breakpoint = 0;
void delay_ms(u16 time)
{    
	u16 i=0;  
	while(time--)
	{
		i=12000;  //????
		while(i--) ;    
	}
}
State_typedef ScreenRecSeqIn(unsigned char *_DataFrame, unsigned char _DataLen)
{
//	DisableIRP();
	if(((FlashScreenDataSeq.SeqFrontAddr + _DataLen * Pendata_frame_len) & 0x200000) &&
		/*(FlashScreenDataSeq.SeqFrontAddr >= FlashScreenDataSeq.SeqRearAddr) || *///头在尾前
//		((FlashScreenDataSeq.SeqFrontAddr < FlashScreenDataSeq.SeqRearAddr)&&//头还未追上尾
		(((FlashScreenDataSeq.SeqFrontAddr + _DataLen * Pendata_frame_len) & 0x1FFFFF) > FlashScreenDataSeq.SeqRearAddr))//下一帧数据如果写入后头指针是否会超过尾
	{
//		//缓存满
//		ledseq_run(BEEP, seq_armed);
//		BEEP_STAT_Flag = 3;//缓存满 蜂鸣器响三声
//		ERR_Status = 0xE1;//在心跳包中加入缓存满标志
//		EnableIRP();
		//TUDO::判断缓冲区情况
    	return Error;
	}
	else
	{
//		DisableIRP();
//		delay_ms(20);
		SPI_FLASH_BufferWrite(_DataFrame, FlashScreenDataSeq.SeqFrontAddr, _DataLen * Pendata_frame_len);//写入一帧数据
//		EnableIRP();
		FlashScreenDataSeq.SeqFrontAddr = ((FlashScreenDataSeq.SeqFrontAddr + _DataLen * Pendata_frame_len) & 0x1FFFFF);
		
		/*计算在缓冲区中的数据总数*/
		if(FlashScreenDataSeq.SeqFrontAddr < FlashScreenDataSeq.SeqRearAddr)//
			DataFrame_in_Flash = FlashScreenDataSeq.SeqFrontAddr + 0x200000 - FlashScreenDataSeq.SeqRearAddr;
		else
			DataFrame_in_Flash = FlashScreenDataSeq.SeqFrontAddr - FlashScreenDataSeq.SeqRearAddr;
//		if((DataFrame_in_Flash > 1258290)&&(DataFrame_in_Flash < 1677720))//缓存满80% 蜂鸣器响两声
//		{
//			ledseq_run(BEEP, seq_flash_60_waring);
//			BEEP_STAT_Flag = 2;
//		}

//		else if(DataFrame_in_Flash > 1677720)//缓存满60% 蜂鸣器响一声
//		{
//			ledseq_run(BEEP, seq_flash_80_waring);
//			BEEP_STAT_Flag = 1;
//		}
	}

//	EnableIRP();
	return Ok;
} 									 
/*接收队列出队列，如果队列为空的话则返回Error*/
/*
 * 数据出队列，该函数会判断缓冲区中的数据总数，如果数据量>=一帧，则全部读取，否则返回队列空
 *
 *
 */
State_typedef ScreenRecSeqOut(unsigned char *_DataFrame, unsigned char _DataLen)
{
	DisableIRP();
	if(FlashScreenDataSeq.SeqReadAddr == FlashScreenDataSeq.SeqFrontAddr)  //缓存空
	{
		EnableIRP();
		return Error;
	}
	else
	{
		SPI_FLASH_BufferRead(_DataFrame, FlashScreenDataSeq.SeqReadAddr, _DataLen);
		if((FlashScreenDataSeq.SeqReadAddr / 4096) != ((FlashScreenDataSeq.SeqReadAddr + _DataLen) / 4096))
			SPI_FLASH_SectorErase(FlashScreenDataSeq.SeqReadAddr);
//		EnableIRP();
		FlashScreenDataSeq.SeqReadAddr = ((FlashScreenDataSeq.SeqReadAddr + _DataLen) & 0x1FFFFF);
		EnableIRP();
		return Ok;
	}
}
///*初始化数据存储队列*/
//void ScreenRecSeqInit()
//{
////	ScreenDataSeq.Datalen[] = 0;
//	ScreenDataSeq.addr = 0;		//为队列添加缓存地址
//	ScreenDataSeq.SeqFront = 0;			//将计数器清零
//	ScreenDataSeq.SeqRear = 0;
//}
///*向接收队列放入一帧数据*/
//State_typedef ScreenRecSeqIn(unsigned char *_DataFrame, unsigned char _DataLen)
//{
//	DisableIRP();
//	if(((ScreenDataSeq.SeqFront + 1) % Seqbufferlen) == ScreenDataSeq.SeqRear)  //缓存满 TUDO::是否存满，有两种情况，第一种是队列满，第二种是存储空间满
//	{
//		EnableIRP();
//		//TUDO::判断缓冲区情况
//		return Error;
//	}
//	/*TUDO::该处加入flash存储*/
//	
//	ScreenDataSeq.addr += _DataLen * 12;
//	ScreenDataSeq.DataHead[ScreenDataSeq.SeqFront].DataLen = _DataLen * 12;
//	ScreenDataSeq.DataHead[ScreenDataSeq.SeqFront].DataAddr = ScreenDataSeq.addr;
//	
////	SPI_FLASH_PageWrite(_DataFrame, ScreenDataSeq.DataHead[ScreenDataSeq.SeqFront].DataAddr, (_DataLen + 1) * 12);
//	SPI_FLASH_BufferWrite(_DataFrame, ScreenDataSeq.DataHead[ScreenDataSeq.SeqFront].DataAddr, _DataLen * 12);
//	
//  	ScreenDataSeq.SeqFront = (ScreenDataSeq.SeqFront + 1) % Seqbufferlen;  //指向循环0~（Seqbufferlen - 1）
//	if(ScreenDataSeq.SeqFront == 0)
//	{
//		ScreenDataSeq.addr = 0;
//	}
//	EnableIRP();
//  	return Ok; 
//} 									 
///*接收队列出队列，如果队列为空的话则返回Error*/
//State_typedef ScreenRecSeqOut(unsigned char *_DataFrame, unsigned char *_DataLen)
//{
//	DisableIRP();
//  	if(ScreenDataSeq.SeqFront == ScreenDataSeq.SeqRear)  //缓存空
//  	{
//		EnableIRP();
//    	return Error;
//  	}
//	else
//  	{
//		//TUDO::该处加入flash读取
//		SPI_FLASH_BufferRead(_DataFrame, ScreenDataSeq.DataHead[ScreenDataSeq.SeqRear].DataAddr , ScreenDataSeq.DataHead[ScreenDataSeq.SeqRear].DataLen);
//		*_DataLen = ScreenDataSeq.DataHead[ScreenDataSeq.SeqRear].DataLen;
//		if((ScreenDataSeq.DataHead[ScreenDataSeq.SeqRear].DataAddr / 4096) > 0)
//		{
//			SPI_FLASH_SectorErase(4096 * (ScreenDataSeq.DataHead[ScreenDataSeq.SeqRear].DataAddr / 4096 - 1));
//		}
//    	ScreenDataSeq.SeqRear = (ScreenDataSeq.SeqRear + 1) % Seqbufferlen; 
//		//是否会受到中断影响？
//		EnableIRP();
//    	return Ok;
//		
//  	} 
//}
