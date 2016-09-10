//#include "sys.h"
//#include "delay.h"
#include "usart.h"
#include "usart2.h"
#include "wifi.h"
#include "Screen.h"
#include <string.h>
#include <stdlib.h>
#define BYTE0(dwTemp)       (*(char *)(&dwTemp))
#define BYTE1(dwTemp)       (*((char *)(&dwTemp) + 1))
#define BYTE2(dwTemp)       (*((char *)(&dwTemp) + 2))
#define BYTE3(dwTemp)       (*((char *)(&dwTemp) + 3))
	
unsigned char wifi_data_to_send[256] = {0};
unsigned char Phone_ID[16] = {0};
unsigned char BroadcastPhone_ID[16] = {0};
unsigned char Board_ID[2] = {0};
unsigned char Phone_IP[4] = {0};
unsigned char Board_IP[4] = {0};
//unsigned char Bar_code[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
unsigned char Bar_code[6] = {0};
unsigned char wifi_data_len = 0;
unsigned char is_First_connect = 1;
//void insert(void *a,void *ins,int index,size_t n,size_t width)
//{
//	char *tmp = malloc(width * (n - index - 1));
//	memcpy(tmp, (char *)a + (index * width), width * (n - index - 1));
//	memcpy((char *)a + index * width, ins, width);
//	memcpy((char *)a + width * (index + 1), tmp, width * (n - index - 1));
//	free(tmp);
//}
void wifi_init()
{
	
}
void wifi_send_pen_data(unsigned char * data_buffer, unsigned char pen_cnt)
{
	unsigned char ii = 0;
	unsigned int num = 0;
	unsigned char sum = 0;
//	unsigned char *p = (wifi_data_to_send + 7);
//	p = wifi_data_to_send + 7;
	wifi_data_len = pen_cnt;
	num = (pen_cnt) * 12;
	wifi_data_to_send[0] = 0xFA;
	wifi_data_to_send[1] = 0x0A;
	wifi_data_to_send[2] = 0xDD;
	wifi_data_to_send[3] = Phone_ID[14];
	wifi_data_to_send[4] = Phone_ID[15];
	wifi_data_to_send[5] = BYTE0(num);
	wifi_data_to_send[6] = BYTE1(num);
//	*wifi_data_to_send += 7;
	
	for(ii = 0; ii < pen_cnt; ii ++)
	{
		memcpy(&(wifi_data_to_send [7 + ii*12]), data_buffer + ii*12, 12);
	}
	for(ii = 0; ii < (num + 7); ii ++)
	{
		sum += wifi_data_to_send[ii];
	}
	wifi_data_to_send[7 + num] = sum;
	wifi_data_to_send[8 + num] = 0xED;
//	rt_sem_take(&Screen_seqSem, RT_WAITING_FOREVER);
	rt_sem_release(&Screen_seqSem);	
	
}
void task_wifi()
{
//	if(screen_data_ready)
//	{
//		screen_data_ready = 0;
////		wifi_send_data(unsigned char * data_buffer, unsigned char pen_cnt);
//	}
}
//------------------------------------
void wifi_ack_send(unsigned char code, unsigned short para) //·¢ËÍÓ¦´ðÐÅÏ¢Ö¸Áî0x77,0x76,0xB1,0xB2 (code),para Îª0x77µÄ²ÎÊý
{
	unsigned char i,sum;
//	unsigned char wifi_send_len = 0;
	wifi_data_to_send[0] = 0xFA;
	wifi_data_to_send[1] = 0x0A;
	wifi_data_to_send[2] = code;
	wifi_data_to_send[3] = Phone_ID[14];
	wifi_data_to_send[4] = Phone_ID[15];
	
	switch(code)
	{
		case(0x77):
			//ÆÕÍ¨×´Ì¬Ó¦´ð
			wifi_data_to_send[5] = 0x02;
			wifi_data_to_send[6] = 0x00;
			wifi_data_to_send[7]=(para>>8);
			wifi_data_to_send[8]=(para&0xFF);
			sum = 0;
			for(i = 0; i < 8; i ++)
			{
				sum += wifi_data_to_send[i];
			}	
			wifi_data_to_send[9] = sum;
			wifi_data_to_send[10] = 0xED;	
			wifi_data_len=11;	//·¢ËÍ×Ö·û´®³¤¶È	
			break;
		
		case(0x76):
			wifi_data_to_send[5] = 0x08;
			wifi_data_to_send[6] = 0x00;
	
			wifi_data_to_send[7] = Board_IP[0];
			wifi_data_to_send[8] = Board_IP[1];
			wifi_data_to_send[9] = Board_IP[2];
			wifi_data_to_send[10] = Board_IP[3];

			wifi_data_to_send[11] = Board_ID[0];
			wifi_data_to_send[12] = Board_ID[1];
			wifi_data_to_send[13] = Phone_ID[14];
			wifi_data_to_send[14] = Phone_ID[15];
			sum = 0;
			for(i = 0; i < 15; i ++)
			{
				sum += wifi_data_to_send[i];
			}	
			wifi_data_to_send[15] = sum;
			wifi_data_to_send[16] = 0xED;		
			wifi_data_len=17;	//·¢ËÍ×Ö·û´®³¤¶È
			break;
		case(0x78)://ÓÃÓÚÉè±¸ÊÇ·ñÁªÍøµÄ¼ì²â
			wifi_data_to_send[3] = BroadcastPhone_ID[14];
			wifi_data_to_send[4] = BroadcastPhone_ID[15];
			wifi_data_to_send[5] = 0x08;
			wifi_data_to_send[6] = 0x00;
	
			wifi_data_to_send[7] = Board_IP[0];
			wifi_data_to_send[8] = Board_IP[1];
			wifi_data_to_send[9] = Board_IP[2];
			wifi_data_to_send[10] = Board_IP[3];

			wifi_data_to_send[11] = Board_ID[0];
			wifi_data_to_send[12] = Board_ID[1];
			wifi_data_to_send[13] = BroadcastPhone_ID[14];
			wifi_data_to_send[14] = BroadcastPhone_ID[15];
			sum = 0;
			for(i = 0; i < 15; i ++)
			{
				sum += wifi_data_to_send[i];
			}	
			wifi_data_to_send[15] = sum;
			wifi_data_to_send[16] = 0xED;		
			wifi_data_len=17;	//·¢ËÍ×Ö·û´®³¤¶È
			break;
		case 0xA1:
			//ÆÕÍ¨×´Ì¬Ó¦´ð
			wifi_data_to_send[5] = 0x02;
			wifi_data_to_send[6] = 0x00;
			wifi_data_to_send[7]=(para>>8);
			wifi_data_to_send[8]=(para&0xFF);
			sum = 0;
			for(i = 0; i < 8; i ++)
			{
				sum += wifi_data_to_send[i];
			}	
			wifi_data_to_send[9] = sum;
			wifi_data_to_send[10] = 0xED;	
			wifi_data_len=11;	//·¢ËÍ×Ö·û´®³¤¶È	
			//¹Ì¼þ°æ±¾ºÅ
			break;
		case 0x70:
			//ÆÕÍ¨×´Ì¬Ó¦´ð
			wifi_data_to_send[5] = 0x02;
			wifi_data_to_send[6] = 0x00;
			wifi_data_to_send[7]=(para>>8);
			wifi_data_to_send[8]=(para&0xFF);
			sum = 0;
			for(i = 0; i < 8; i ++)
			{
				sum += wifi_data_to_send[i];
			}	
			wifi_data_to_send[9] = sum;
			wifi_data_to_send[10] = 0xED;	
			wifi_data_len=11;	//·¢ËÍ×Ö·û´®³¤¶È	
			//¹Ì¼þ°æ±¾ºÅ
			break;
		case 0xB1:
		
			//¹Ì¼þ°æ±¾ºÅ
			break;
		
		case 0xB2:
		
			//´¥ÆÁÐÅÏ¢
			break;
		case 0x74:
			wifi_data_to_send[5] = 0x04;
			wifi_data_to_send[6] = 0x00;

			wifi_data_to_send[7] = 0;
			wifi_data_to_send[8] = 0;
			wifi_data_to_send[9] = 0;
			wifi_data_to_send[10] = 0;

			sum = 0;
			for(i = 0; i < 11; i ++)
			{
				sum += wifi_data_to_send[i];
			}	
			wifi_data_to_send[11] = sum;
			wifi_data_to_send[12] = 0xED;		
			wifi_data_len = 13;	//·¢ËÍ×Ö·û´®³¤¶È			
			//´¥ÆÁÐÅÏ¢
			break;
	}
//	is_wifisent_flag=2;//´ËÐÅÏ¢ÎªÓ¦´ðÐÅÏ¢,²»ÐèÒªÊÖ»úÓ¦´ð
//	Uart2_Put_Buf(wifi_data_to_send , wifi_send_len);
}
void wifi_data_deal(unsigned char *data_buf, unsigned char num)
{
	
//	vs16 rc_value_temp;
	unsigned char sum = 0;
	unsigned char i = 0;
	unsigned char Bar_code_err_flag = 0;
	unsigned char Phone_ID_err_flag = 0;
	
//	unsigned char point_cnt = 0;//´¥Ãþµã¼ÆÊý

	if(!(*(data_buf)==0xFA && *(data_buf + 1)==0x0A))	
	{	
		return;
	}		//ÅÐ¶ÏÖ¡Í·
/////////////////////////////////////////////////////////////////////////////////////
	switch(*(data_buf + 2))
	{				//µÚÒ»´ÎÁ¬½ÓµÄÊ±ºò£¬ÐèÒªÅÐ¶Ï¶þÎ¬ÂëÊÇ·ñÓë´æ´¢µÄ¶þÎ¬ÂëÏàÍ¬£¬Èç¹ûÏàÍ¬ÔòÁ¬½Ó£»
					//ºóÐøµÄ¶ÏÏßÖØÁ¬£¬ÐèÒªÅÐ¶ÏÊÖ»úIDÊÇ·ñÓëµÚÒ»´ÎÁ¬½ÓÊ±´æ´¢µÄÏàÍ¬£¬Èç¹ûÏàÍ¬µÄ»°£¬¼ÌÐøÅÐ¶ÏÊÖ»úIPÊÇ·ñ¸Ä±ä£¬
		case 0x75:	//Èç¹û¸Ä±äÔòÐèÒªÖØÐÂÅäÖÃUDPCµÄIP²ÎÊý
			for(i = 0; i < 6; i++)//ÈÎºÎÇé¿öÏÂÊÕµ½Á¬½ÓÇëÇó¶¼ÐèÒªÅÐ¶Ï¶þÎ¬ÂëÊÇ·ñÕýÈ·
			{
				if(*(data_buf + 27 + i) != Bar_code[i])
				{
					Bar_code_err_flag = 1;
					break;
				}
			}
			if(Bar_code_err_flag == 1)//¶þÎ¬Âë´íÎóÔò²»ÏìÓ¦Á¬½ÓÇëÇó
				net_complete_flag = 0;
			else//¶þÎ¬ÂëÕýÈ·£¬Ê×ÏÈ£¬Èç¹ûÊÇµÚÒ»´ÎÁ¬½Ó£¬´æ´¢½ÓÊÕµ½µÄÊÖ»úIP£¬½øÐÐÍøÂçÉèÖÃ¡£
			{	//Èç¹û²»ÊÇµÚÒ»´ÎÁ¬½Ó£¬ÔòÐèÒª±È¶ÔÊÖ»úIDÊÇ·ñÕýÈ·
				//ÊÖ»úIP
				if(is_First_connect)
				{
					is_First_connect = 0;
					//if()
					Phone_IP[0]	= *(data_buf + 7);
					Phone_IP[1]	= *(data_buf + 8);
					Phone_IP[2]	= *(data_buf + 9);
					Phone_IP[3]	= *(data_buf + 10);
					//ÊÖ»úID
					for(i = 0; i < 16; i ++)
					{
						Phone_ID[i] = *(data_buf + 11 + i);
						//Phone_ID[1] = *(data_buf + 26);
					}
					//°×°åID
					Board_ID[0] = *(data_buf + 31);
					Board_ID[1] = *(data_buf + 32);
					wifi_ack_send(0x76, 0);
				}
				else
				{
					for(i = 0; i < 16; i++)//ÅÐ¶ÏÊÖ»úIDÊÇ·ñÕýÈ·
					{
						if(*(data_buf + 11 + i) != Phone_ID[i])
						{
							Phone_ID_err_flag = 1;
							break;
						}
					}	
					if(Phone_ID_err_flag == 1)//ÊÖ»úID´íÎóÔò²»ÏìÓ¦
					{	
					}
					else
					{
						Phone_IP[0]	= *(data_buf + 7);
						Phone_IP[1]	= *(data_buf + 8);
						Phone_IP[2]	= *(data_buf + 9);
						Phone_IP[3]	= *(data_buf + 10);
						net_reconnect_flag = 1;
						wifi_ack_send(0x76, 0);
					}
				}
			}
			break;
		case 0x78://ÓÃÓÚ¼ì²â°×°åÊÇ·ñ½ÓÈë¾ÖÓòÍø
			for(i = 0; i < 6; i++)//ÈÎºÎÇé¿öÏÂÊÕµ½Á¬½ÓÇëÇó¶¼ÐèÒªÅÐ¶Ï¶þÎ¬ÂëÊÇ·ñÕýÈ·
			{
				if(*(data_buf + 27 + i) != Bar_code[i])
				{
					Bar_code_err_flag = 1;
					break;
				}
			}
			if(Bar_code_err_flag == 1)//¶þÎ¬Âë´íÎóÔò²»ÏìÓ¦Á¬½ÓÇëÇó
				searching_flag = 0;
			else
			{
//				//if()
//				Phone_IP[0]	= *(data_buf + 7);
//				Phone_IP[1]	= *(data_buf + 8);
//				Phone_IP[2]	= *(data_buf + 9);
//				Phone_IP[3]	= *(data_buf + 10);
				//ÊÖ»úID
				for(i = 0; i < 16; i ++)
				{
					BroadcastPhone_ID[i] = *(data_buf + 11 + i);
					//Phone_ID[1] = *(data_buf + 26);
				}
				//°×°åID
//				Board_ID[0] = *(data_buf + 35);
//				Board_ID[1] = *(data_buf + 36);
				wifi_ack_send(0x78, 0);
				searching_flag = 1;
			}
//			wifi_ack_send(0x77, 0);
//			is_idle_flag = 1;
			break;
		case 0xA9://ÐÄÌø°ü
			wifi_ack_send(0x77, 0);
			is_idle_flag = 1;
//			FlashScreenDataSeq.SeqReadAddr = FlashScreenDataSeq.SeqFrontAddr;
//			FlashScreenDataSeq.SeqRearAddr = FlashScreenDataSeq.SeqReadAddr;			//½«¼ÆÊýÆ÷ÇåÁã
			break;
		case 0xA1:{//½Óµ½ÊÖ»ú·¢ËÍµÄ»áÒé½áÊøÖ¸Áî
			meeting_end_flag = 1;
			is_idle_flag = 0;
			//wifi_ack_send(0x77,0);
			//*Ôö¼ÓÖ¸Áî
			//(1)Á¬½Ó×´Ì¬¶Ï¿ª,Í¬Ê±ÖØÆôwifiÄ£¿é
			//(2)¶ÓÁÐÇåÁã,WIFI»º³åÇåÁã,ÊÖ»úIPºÍIDÇåÁã,°×°åIPÖØ¶Á
			//(3)ERR_Status±äÁ¿ÇåÁã,»áÒé×´Ì¬×¼±¸
			break;
			}
		case 0xA2:{//ÒªÇóÉÏ´ÎÖ¸ÁîÖØ·¢
//			Uart2_Put_Buf(wifi_data_to_send , wifi_sent_len);
			break;
			}
		case 0xA5:{//Ä¿Ç°Ð§¹ûµÈÍ¬ÓÚA9
//		  if(ERR_Status==0)
//				wifi_ack_send(0x77,0);
//			else
//				wifi_ack_send(0x77,0x7800|ERR_Status);		
			break;		
			}
		case 0xA6:{//°×°å´¥ÆÁÐÅÏ¢
			wifi_ack_send(0xB2,0);
			break;
			}
		case 0xa7:{//Ó²¼þ°æ±¾ºÅÐÅÏ¢
			wifi_ack_send(0xB1,0);
			break;
			}
		case 0xA8:{//²éÑ¯µ±Ç°Â¼ÖÆ×´Ì¬
//		switch(REC_STAT_Flag){
//			case 0:
//				temp_sh=0x7910;
//				break;
//			case 1:
//				temp_sh=0x7911;
//				break;
//			case 2:
//				temp_sh=0x7913;
//			break;
//		}
		
//		wifi_ack_send(0x77,temp_sh);
			break;
			}
		case 0xAC:{	//°ÑÊý¾ÝÊ¹ÖÕ±£´æÔÚ¶ÓÁÐÖÐ,²»¹ÜÓÐÃ»ÓÐ·¢ËÍ
					//*Ôö¼Ó¶ÓÁÐÎ²Ö¸ÕëµÄ×´Ì¬¿ØÖÆ±äÁ¿
					//FLA_DPMV_Enable=0;
			wifi_ack_send(0x77,0);
			break;
			}
		case 0xAB:{	//¶Ó¸üÇåÁãÃüÁî
			wifi_ack_send(0x77,0);
					//*Ôö¼Óµ÷ÓÃ¶ÓÁÐÇåÁã×Óº¯Êý
					//*Èç¹ûÓÐ´íÎó,wifi_ack_send(0x77,0x78FE)
			break;
			}
		case 0xAD:{	//¶ÓÁÐÖÐµÄÊý¾Ý·¢ËÍÖ¸Áî
			wifi_ack_send(0x77,0);
//			FLA_DPMV_Enable=1;
					//*Ôö¼Ó ¶ÓÁÐµÄµ±Ç°Ö¸ÕëÖ¸ÏòÎ²Ö¸ÕëËùÔÚÎ»ÖÃ
			break;
			}
		case 0xAA:{	//¹Ì¼þÉý¼¶Ä£Ê½
			wifi_ack_send(0x77,0);
					//*Ôö¼Ó¹Ì¼þÉýÊ±ÐèÒªµÄÖ¸Áî
			break;
			}
//		case 0x77:{//ÊÕµ½ÊÖ»úÖ¸ÁîÕý³£Ó¦´ðÐÅºÅ
////		is_wifisent_flag=1;//ÉÏÒ»ÌõÖ¸Áî·¢ËÍÍê³É
////*Ôö¼Ó:Èç¹ûFLA_DPMV_Enable==1;¶ÓÁÐÎ²Ö¸Õë=µ±Ç°Ö¸Õë
//			break;	
//			}
		case 0x73:{	//ÊÕµ½¶ÔÊ±ÇëÇó
					//*Ôö¼Ó¼ÆÊ±Æ÷ÇåÁãÖ¸Áî,¿ªÆô¼ÆÊ±Æ÷Ö¸Áî
			time_base = 0;
			//wifi_ack_send(0x74,0);
			time_checking_flag = 1;
			break;
			}
		case 0x79:{//ÊÕµ½Ö¸Ê¾ÐÅÏ¢¿ØÖÆ
			//*Ôö¼Óµ÷ÓÃÖ¸Ê¾¿ØÖÆº¯ÊLED_Bee_Coný,²ÎÊýÎª*(data_buf+7),¸Ãº¯ÊýµÄ¶¨Òå²Î¼û´«ÊäÐ­Òé:0x79µÄ²ÎÊý		
			wifi_ack_send(0x77,0);
			break;		
			}
		case 0x7A:{//ÊÕµ½ÉèÖÃÍøÂçÕÊºÅºÍÃÜÂëµÄÖ¸Áî
			wifi_ack_send(0x77,0);
//*Ôö¼ÓÖ¸Áî:
//(1)¹¹½¨Ò»¸ö×Ö·û´®:AT+STA=[ssid],[sectype],[key]		
//(2)WIFIÄ£¿é½øÈëATÄ£Ê½		
//(3)°Ñ×Ö·û´®ËÍÈë,µÈ´ý½ÓÊÕÕý³£ÐÅÏ¢
//(4)ÖØÆôwifiÄ£¿é
			break;
			}
		case 0x5A:{//ÊÕµ½ÉèÖÃ°×°å¶þÎ¬ÂëÖ¸Áî
			wifi_ack_send(0x77,0);
//		for(i = 0; i < 10; i++)
//		{
//			Bar_code[i]=*(data_buf+7+i);
//		}
//*Ôö¼ÓÖ¸Áî:
//µ÷ÓÃ°ÑBar_code·´Ð´ÈëFLASHÖ¸Áî
//Èç¹ûÓÐ´íÎó,µ÷ÓÃ	wifi_ack_send(0x77,0x78FE);	
			break;
			}	
		case 0x5B:{//ÉèÖÃ´¥ÆÁ±êÖ¾ºÅ unsigned char TC_INF[4]
			wifi_ack_send(0x77,0);
//		for(i=0;i<4;i++){
//			TC_INF[i]=*(data_buf+7+i);
//		}
//*Ôö¼ÓÖ¸Áî:
//µ÷ÓÃ°ÑTC_INF·´Ð´ÈëFLASHÖ¸Áî
//Èç¹ûÓÐ´íÎó,µ÷ÓÃ	wifi_ack_send(0x77,0x78FE);	
			break;
			}
		case 0x5c:{//ÉèÖÃÎó²îãÐÖµ
			wifi_ack_send(0x77,0);
//		Ex=*(data_buf+7);
//		Ey=*(data_buf+8);
//*Ôö¼ÓÖ¸Áî:
//µ÷ÓÃ°ÑEX EY ·´Ð´ÈëFLASHÖ¸Áî
//Èç¹ûÓÐ´íÎó,µ÷ÓÃ	wifi_ack_send(0x77,0x78FE);	
			break;
			}
		case 0x5D:{//ÉèÖÃµãÊý¾ÝÉÏ´«¸øÊÖ»úµÄÊ±¼ä¼ä¸ô
			wifi_ack_send(0x77,0);
//		Dev_T=*(data_buf+8);
//		Dev_T<<=8;
//		Dev_T+=*(data_buf+7);
//*Ôö¼ÓÖ¸Áî:
//µ÷ÓÃ°ÑDev_T·´Ð´ÈëFLASHÖ¸Áî
//Èç¹ûÓÐ´íÎó,µ÷ÓÃ	wifi_ack_send(0x77,0x78FE);	
//ÖØÐÂÉèÖÃÑÓÊ±Ö¸Áî		
			break;
			}			
	}
}
