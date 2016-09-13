#ifndef __SCREEN_H
#define __SCREEN_H

#include "stm32f10x.h"
#include <rtthread.h>

#define Seqbufferlen 1000//队列深度
extern unsigned int DataFrame_in_Flash;

typedef enum
{
	Ok		=(unsigned char)1, 
	Error	=! Ok    
}State_typedef;

/*队列结构*/
typedef struct
{
	volatile unsigned int	SeqFrontAddr;//队列的顶部，进数据
	volatile unsigned int	SeqRearAddr;//队列的底部，出数据
	volatile unsigned int	SeqReadAddr;
}FlashSeq_structdef;
/*RS485队列结构*/
typedef struct
{
	unsigned char	Data[Seqbufferlen];//队列中的数据
	unsigned int	SeqFront;//队列的顶部，进数据
	unsigned int	SeqRear;//队列的底部，出数据
}BufferSeq_structdef;

extern FlashSeq_structdef FlashScreenDataSeq;
extern unsigned int time_base;
extern unsigned char up_down_flag;
extern unsigned char pen_data_ready;
extern unsigned char net_complete_flag;
extern unsigned char net_reconnect_flag;
extern unsigned char net_reset_flag;
extern unsigned char is_idle_flag;
extern unsigned char meeting_end_flag;
extern unsigned char time_checking_flag;
extern unsigned char searching_flag;
extern unsigned char point_cnt;

extern unsigned char pen_data[256];//笔迹数据

void Screen_data_analize(unsigned char *data_buf,unsigned char num);
void ScreenRecSeqInit(void);
State_typedef ScreenRecSeqIn(unsigned char *_DataFrame, unsigned char _DataLen);
State_typedef ScreenRecSeqOut(unsigned char *_DataFrame, unsigned char _DataLen);


#endif
