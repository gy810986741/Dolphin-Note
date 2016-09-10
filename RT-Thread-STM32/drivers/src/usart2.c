#include "usart2.h"
#include "stdio.h"	 	 
#include "string.h"	
#include "Screen.h"
#include "wifi.h"
#include "cc3200.h"

//////////////////////////////////////////////////////////////////////////////////	 
////////////////////////////////////////////////////////////////////////////////// 	   
struct rt_semaphore Screen_seqSem;//创建LED闪烁控制信号量，该信号量和FreeRTOS中的信号量稍有区别
struct rt_semaphore Net_complete_seqSem;//网络设置完成信号量
//串口接收缓存区 	
u8 USART2_RX_BUF[USART2_MAX_RECV_LEN]; 				//接收缓冲,最大USART3_MAX_RECV_LEN个字节.
u8 USART2_TX_BUF[USART2_MAX_SEND_LEN]; 			//发送缓冲,最大USART3_MAX_SEND_LEN字节

//通过判断接收连续2个字符之间的时间差不大于10ms来决定是不是一次连续的数据.
//如果2个字符接收间隔超过10ms,则认为不是1次连续数据.也就是超过10ms没有接收到
//任何数据,则表示此次接收完毕.
//接收到的数据状态
//[15]:0,没有接收到数据;1,接收到了一批数据.
//[14:0]:接收到的数据长度
vu16 USART2_RX_STA=0;   	




#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	  

//初始化IO 串口1 
//bound:波特率
void uart1_init(u32 bound)
{
    //GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	//USART_ClockInitTypeDef  USART_ClockInitStructure;
	USART_ClockInitTypeDef USART_ClockInitStruct;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能UART3，GPIOB时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
 	USART_DeInit(USART1);  //复位串口
	 //USART1_TX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化
 
	//USART1_RX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化

   //USART1 NVIC 配置
//	NVIC_Configuration();
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	//配置USART1
	//中断被屏蔽了
	USART_InitStructure.USART_BaudRate = bound;       //波特率可以通过地面站配置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;  //8位数据
	USART_InitStructure.USART_StopBits = USART_StopBits_1;   //在帧结尾传输1个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;    //禁用奇偶校验
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //硬件流控制失能
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;  //发送、接收使能
	//配置USART1时钟
	USART_ClockInitStruct.USART_Clock = USART_Clock_Disable;  //时钟低电平活动
	USART_ClockInitStruct.USART_CPOL = USART_CPOL_Low;  //SLCK引脚上时钟输出的极性->低电平
	USART_ClockInitStruct.USART_CPHA = USART_CPHA_2Edge;  //时钟第二个边沿进行数据捕获
	USART_ClockInitStruct.USART_LastBit = USART_LastBit_Disable; //最后一位数据的时钟脉冲不从SCLK输出
	
	USART_Init(USART1, &USART_InitStructure);
	USART_ClockInit(USART1, &USART_ClockInitStruct);

	//使能USART1接收中断
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	
	rt_sem_init(&Screen_seqSem , "ledseqSem", 0, RT_IPC_FLAG_FIFO);//初始化信号量，初始值为1，该信号量会被第一个持有的线程清除
//使能USART1
	USART_Cmd(USART1, ENABLE); 
}

unsigned char Screen_TxBuffer[50] = {0};
unsigned char Screen_RxBuffer[256] = {0};
//unsigned int i = 0;
unsigned char count_1 = 0;
unsigned char TxCounter_1 = 0;
void Uart1_Put_Buf(unsigned char *DataToSend , unsigned char data_num)
{
	unsigned char i = 0;
	for(i = 0; i < data_num; i ++)
		Screen_TxBuffer[count_1++] = *(DataToSend + i);
	if(!(USART1->CR1 & USART_CR1_TXEIE))
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE); 
}

void USART1_IRQHandler(void)                	//串口1中断服务程序
{
	static  unsigned char err_cnt = 0;
			unsigned char ResData;//接收到的一个字节		
	static 	unsigned char Data_Len = 0,//数据长度
						  Data_Count = 0;//数据长度计数
	static 	unsigned char RxState = 0;//接收数据状态计数   用于切换状态
//	u8 Res;
	
	/* enter interrupt */
    rt_interrupt_enter();
	
	if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET)//
    {
        USART_ReceiveData(USART1);
    }		
	//发送中断
	if((USART1->SR & (1 << 7))&&(USART1->CR1 & USART_CR1_TXEIE))
	{
		USART1->DR = Screen_TxBuffer[TxCounter_1++]; //写DR清除中断标志          
		if(TxCounter_1 == count_1)
		{
			USART1->CR1 &= ~USART_CR1_TXEIE;		//关闭TXE中断
		}
	}
	//接收中断 (接收寄存器非空) 
	if(USART1->SR & (1 << 5))  
	{
		ResData = USART1->DR;
		switch (RxState)
		{
			case 0:
				
			
				if(ResData == 0x68)
				{
					RxState = 1;
					Screen_RxBuffer[0] = ResData;
				}
			break;
			case 1:
				if(ResData < 50)
				{
					RxState = 2;
					Screen_RxBuffer[1] = ResData;
					Data_Len = ResData;
					Data_Count = 0;
				}
			break;
			case 2:
				if(Data_Len > 0)
				{
					Data_Len --;
					Screen_RxBuffer[2 + Data_Count ++] = ResData;
					if(Data_Len == 0)
					{
						RxState = 0;
//						flag = 1;
//						_num = Data_Count + 2;
						Screen_data_analize(Screen_RxBuffer,Data_Count + 2);
						
					}
				}
				else
				{
					err_cnt ++;
					if(err_cnt >= 1)
					{
						err_cnt = 0;
						RxState = 0;
					}
				}
			break;
			default:
				RxState = 0;
			break;	
		}
	}
	/* leave interrupt */
    rt_interrupt_leave();
} 

#endif



unsigned char WIFI_TxBuffer[256] = {0};
unsigned char WIFI_RxBuffer[256] = {0};//原来的数组长度过小，会导致溢出
//unsigned char WIFI_RxBuffer2[39] = {0};
unsigned char count_2 = 0;
unsigned char TxCounter_2 = 0;
void Uart2_Put_Buf(unsigned char *DataToSend , unsigned char data_num)
{
	unsigned char i = 0;
	for(i = 0; i < data_num; i ++)
		WIFI_TxBuffer[count_2++] = *(DataToSend + i);
	if(!(USART2->CR1 & USART_CR1_TXEIE))
		USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
}

void USART2_IRQHandler(void)
{
	/* enter interrupt */
    rt_interrupt_enter();
	static  unsigned char 	err_cnt = 0;
			unsigned char 	ResData;//接收到的一个字节
	static 	unsigned char	Data_Len = 0,//数据长度
							Data_Count = 0;//数据长度计数
	static 	unsigned char	RxState = 0;//接收数据状态计数   用于切换状态
	static	unsigned char	ID_cnt = 0;
//	static	unsigned char	rescount = 0;

		
	
	/* enter interrupt */
	rt_interrupt_enter();
	u8 res;
	switch(socket_start)
	{
		case 0:
				if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)//接收到数据
				{	 
					res = USART_ReceiveData(USART2);		 
					if((USART2_RX_STA&(1<<15))==0)//接收完的一批数据,还没有被处理,则不再接收其他数据
					{ 
						if(USART2_RX_STA<USART2_MAX_RECV_LEN)	//还可以接收数据
						{
							TIM_SetCounter(TIM7,0);          		//计数器清空
							if(USART2_RX_STA==0) 				//使能定时器7的中断 
							{
								TIM_Cmd(TIM7,ENABLE);//使能定时器7
							}
							USART2_RX_BUF[USART2_RX_STA++]=res;	//记录接收到的值	 
						}
						else 
						{
							USART2_RX_STA|=1<<15;				//强制标记接收完成
						} 
					}
				}
				break;			
	///////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////
		case 1:
				if (USART_GetFlagStatus(USART2, USART_FLAG_ORE) != RESET)//
				{
					USART_ReceiveData(USART2);
				}		
				//发送中断
				if((USART2->SR & (1 << 7))&&(USART2->CR1 & USART_CR1_TXEIE))
				{
					USART2->DR = WIFI_TxBuffer[TxCounter_2++]; //写DR清除中断标志          
					if(TxCounter_2 == count_2)
					{
						//TxCounter_2 = 0;
						//count_2 = 0;
						USART2->CR1 &= ~USART_CR1_TXEIE;		//关闭TXE中断
					}
				}
				//接收中断 (接收寄存器非空) 
				if(USART2->SR & (1 << 5))  
				{
					
					ResData = USART2->DR;
//					WIFI_RxBuffer2[rescount++]=ResData;
//					if(rescount>38)
//						rescount = 0;
					
					switch (RxState)
					{
						case 0:
							if(ResData == 0xFA)
							{
								RxState = 1;
								WIFI_RxBuffer[0] = ResData;
							}
						break;
						case 1:
							if(ResData == 0x0A)
							{
								RxState = 2;
								WIFI_RxBuffer[1] = ResData;
							}
						break;
						case 2://FC
							RxState = 3;
							WIFI_RxBuffer[2] = ResData;
						break;
						case 3://ID
							WIFI_RxBuffer[3 + ID_cnt] = ResData;
							ID_cnt ++;
							if(ID_cnt > 1)
							{
								ID_cnt = 0;
								RxState = 4;
								break;
							}
							
						break;
						case 4:
							WIFI_RxBuffer[5 + ID_cnt] = ResData;
							ID_cnt ++;
							if(ID_cnt > 1)
							{
								ID_cnt = 0;
								Data_Len = ((unsigned int)WIFI_RxBuffer[6])<<8 | WIFI_RxBuffer[5];
								
								Data_Count = 0;
								if(Data_Len == 0)
								{
									RxState = 6;
									break;
								}
								RxState = 5;
								break;
							}
						break;
						case 5:
							if(Data_Len > 0)
							{
								Data_Len --;
								WIFI_RxBuffer[7 + Data_Count ++] = ResData;
								if(Data_Len == 0)
								{
									RxState = 6;
									break;
								}
							}
							else//这里为了防止卡在case 5 
							{
								err_cnt ++;
								if(err_cnt > 1)
								{
									err_cnt = 0;
									RxState = 0;
								}
							}
						break;
						case 6:
							WIFI_RxBuffer[7 + Data_Count ++] = ResData;
							RxState = 7;
						break;
						case 7:
							WIFI_RxBuffer[7 + Data_Count] = ResData;
							wifi_data_deal(WIFI_RxBuffer,8 + Data_Count);
							RxState = 0;
						break;
						default:
							RxState = 0;
						break;	
					}		
				}	
				break;
	}
	
	/* leave interrupt */
	rt_interrupt_leave();
}   


//初始化IO 串口2
//pclk1:PCLK1时钟频率(Mhz)
//bound:波特率	  
void usart2_init(u32 bound)
{  

	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	// GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE); //串口2时钟使能

 	USART_DeInit(USART2);  //复位串口2
		 //USART2_TX   PA2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PB10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PB10

	//USART2_RX	  PA3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PB11
	
	USART_InitStructure.USART_BaudRate = bound;//波特率一般设置为9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  
	USART_Init(USART2, &USART_InitStructure); //初始化串口	3
  

	USART_Cmd(USART2, ENABLE);                    //使能串口 
	
	//使能接收中断
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启中断   
	
	//设置中断优先级
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	
	
	TIM7_Int_Init(1000-1,7200-1);		//10ms中断
	USART2_RX_STA=0;		//清零
	TIM_Cmd(TIM7,DISABLE);			//关闭定时器7

}

//定时器7中断服务程序		    
void TIM7_IRQHandler(void)
{ 	
	if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)//是更新中断
	{	 			   
		USART2_RX_STA|=1<<15;	//标记接收完成
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update  );  //清除TIM7更新中断标志    
		TIM_Cmd(TIM7, DISABLE);  //关闭TIM7 
	}	    
}
 
//通用定时器7中断初始化
//这里时钟选择为APB1的2倍，而APB1为42M
//arr：自动重装值。
//psc：时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=定时器工作频率,单位:Mhz 
//通用定时器中断初始化
//这里始终选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数		 
void TIM7_Int_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);//TIM7时钟使能    
	
	//定时器TIM7初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE ); //使能指定的TIM7中断,允许更新中断
	
	TIM_Cmd(TIM7,ENABLE);//开启定时器7
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//子优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	
}















