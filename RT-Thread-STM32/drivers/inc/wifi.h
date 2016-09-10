#ifndef __WIFI_H
#define __WIFI_H

#define Screen_shot 0xA0
#define Meeting_adjourned 0xA1
#define Resend 0xA0
#define Observed_0 0xA0
#define Observed_1 0xA0
#define Error_code 0xA0
#define Screen_info 0xA0
#define Screen_HW 0xA0
#define Record_state 0xA0
#define Heartbeat 0xA0
#define Save_queue 0xA0
#define Clear_queue 0xA0
#define Send_queue 0xA0
#define Screen_update 0xA0
#define Normal_back 0xA0


#define Screen_shot_T 0xA0
#define Record_start 0xA0
#define Record_stop 0xA0
#define Time_set_start 0xA0
#define Time_set_stop 0xA0
#define Start_link 0xA0
#define End_link 0xA0
#define IO_control 0xA0
#define Wifi_info 0xA0
#define camera 0xA0
#define camera 0xA0
#define camera 0xA0

extern unsigned char wifi_data_to_send[256];
extern unsigned char Phone_ID[16];
extern unsigned char Board_ID[2];
extern unsigned char Phone_IP[4];
extern unsigned char Board_IP[4];
extern unsigned char Bar_code[6];
extern unsigned char wifi_data_len;
extern unsigned char is_First_connect;

void wifi_data_deal(unsigned char *data_buf,unsigned char num);
void wifi_send_pen_data(unsigned char * data_buffer, unsigned char pen_cnt);
//void wifi_send_cmd(unsigned char * data_buffer, unsigned char pen_cnt);
void wifi_ack_send(unsigned char code, unsigned short para);

#endif

