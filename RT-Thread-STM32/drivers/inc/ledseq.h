/*
 * ledseq.h - LED sequence handler
 */

#ifndef __LEDSEQ_H__
#define __LEDSEQ_H__

#include <stdbool.h>
#include <led.h>

typedef void * xTimerHandle;

#define LEDSEQ_CHARGE_CYCLE_TIME  100
//Led sequence action
#define LEDSEQ_WAITMS(X) (X)
#define LEDSEQ_STOP      -1
#define LEDSEQ_LOOP      -2

typedef struct 
{
  bool value;
  int action;
} ledseq_t;

//Public API
void ledseq_init(void);
//bool ledseqTest(void);

void ledseq_run(led_t led, ledseq_t * sequence);
void ledseq_stop(led_t led, ledseq_t * sequence);
void ledseqSetTimes(ledseq_t *sequence, rt_int32_t onTime, rt_int32_t offTime);

//Existing led sequences
extern ledseq_t seq_armed[];
extern ledseq_t seq_alwayson[];
extern ledseq_t seq_error[];
extern ledseq_t seq_alive[];
extern ledseq_t seq_lowbat[];
extern ledseq_t seq_flash_3_times[];
extern ledseq_t seq_power_on[];
extern ledseq_t seq_flash_80_waring[];
extern ledseq_t seq_flash_60_waring[];
extern ledseq_t seq_testPassed[];

#endif

