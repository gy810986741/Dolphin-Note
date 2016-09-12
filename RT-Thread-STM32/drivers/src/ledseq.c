
/* LED闪烁功能是由数组中的action实现的，每一个action包含了对应的LED的on/off(value)和该状态所持续的时间，
 * 当该时间到达后才能继续执行下一个动作。
 * LED的闪烁序列存储在一个list中，每一个sequence中的状态和时间是按照优先级排序(index)，每一个sequence
 * 只能对一个LED有效。
 * 该功能使用了四个软件定时器，对应四个LED。四个定时器公用一个超时函数，在超时函数内通过对触发超时函数
 * 的句柄地址进行判断来确定该定时器是对哪一个LED进行操作。在此过程中需要使用信号量来确保在超时函数仍在处理
 * 的时候其他定时器触发了该超时函数的情况下不会导致定时器紊乱。
 */
#include <stdbool.h>
#include "board.h"
#include "ledseq.h"
//#include "led.h"

static void* timer_led[LED_NUM];//定义空地址指针，指向四个定时器句柄地址，节省空间

/* Led sequence priority */
static ledseq_t * sequences[] = //LED闪烁序列定义
{
	seq_testPassed,	//测试通过
	seq_lowbat,		//电量低
	seq_power_on,	//充电完成
	seq_flash_80_waring,	//充电中
	seq_flash_60_waring,	//bootloader指示
	seq_armed,		//警告
	seq_alwayson,//校准中
	seq_error,	//校准完成
	seq_alive,		//正常运行
	seq_flash_3_times,		//连接成功
};

/* Led sequences */
ledseq_t seq_lowbat[] = 
{
	{ true, LEDSEQ_WAITMS(1000)},
	{    0, LEDSEQ_LOOP},
};

ledseq_t seq_armed[] = 
{
	{ true, LEDSEQ_WAITMS(500)},
	{false, LEDSEQ_WAITMS(500)},
	{ true, LEDSEQ_WAITMS(500)},
	{false, LEDSEQ_WAITMS(500)},
	{ true, LEDSEQ_WAITMS(500)},
	{false, LEDSEQ_WAITMS(500)},
	{    0, LEDSEQ_STOP},
};

ledseq_t seq_alwayson[] = 
{
	{ true, LEDSEQ_WAITMS(1000)},
	{    0, LEDSEQ_LOOP},
};

ledseq_t seq_error[] = 
{
	{ true, LEDSEQ_WAITMS(500)},
	{false, LEDSEQ_WAITMS(500)},
	{    0, LEDSEQ_LOOP},
};

ledseq_t seq_alive[] = 
{
	{ true, LEDSEQ_WAITMS(50)},
	{false, LEDSEQ_WAITMS(950)},
	{    0, LEDSEQ_LOOP},
};

ledseq_t seq_flash_3_times[] = 
{
	{false, LEDSEQ_WAITMS(500)},
	{ true, LEDSEQ_WAITMS(500)},
	{false, LEDSEQ_WAITMS(500)},
	{ true, LEDSEQ_WAITMS(500)},
	{false, LEDSEQ_WAITMS(500)},
	{ true, LEDSEQ_WAITMS(500)},
	{    1, LEDSEQ_STOP},//最后一个状态在STOP的情况下会保持value的值
};


ledseq_t seq_power_on[] = 
{
	{ true, LEDSEQ_WAITMS(1000)},
	{    1, LEDSEQ_STOP},
};

ledseq_t seq_flash_80_waring[] = 
{
	{ true, LEDSEQ_WAITMS(500)},
	{false, LEDSEQ_WAITMS(500)},
	{ true, LEDSEQ_WAITMS(500)},
	{false, LEDSEQ_WAITMS(500)},
	{    0, LEDSEQ_STOP},
};

ledseq_t seq_flash_60_waring[] = 
{
	{ true, LEDSEQ_WAITMS(500)},
	{false, LEDSEQ_WAITMS(500)},
	{    0, LEDSEQ_STOP},
};

ledseq_t seq_testPassed[] = 
{
	{ true, LEDSEQ_WAITMS(50)},
	{false, LEDSEQ_WAITMS(50)},
	{ true, LEDSEQ_WAITMS(50)},
	{false, LEDSEQ_WAITMS(50)},
	{ true, LEDSEQ_WAITMS(50)},
	{false, LEDSEQ_WAITMS(50)},
	{ true, LEDSEQ_WAITMS(50)},
	{false, LEDSEQ_WAITMS(50)},
	{ true, LEDSEQ_WAITMS(50)},
	{false, LEDSEQ_WAITMS(50)},
	{ true, LEDSEQ_WAITMS(50)},
	{false, LEDSEQ_WAITMS(50)},
	{ true, LEDSEQ_WAITMS(50)},
	{false, LEDSEQ_WAITMS(50)},
	{false, LEDSEQ_STOP},
};

/* LED序列处理机制 */
#define SEQ_NUM (sizeof(sequences)/sizeof(sequences[0]))

static void runLedseq(void * parameter);
static void updateActive(led_t led);
static int getPrio(ledseq_t *seq);

//State of every sequence for every led: LEDSEQ_STOP if stopped or the current step
static int state[LED_NUM][SEQ_NUM];
//Active sequence for each led
static int activeSeq[LED_NUM];
struct rt_semaphore ledseqSem;//创建LED闪烁控制信号量，该信号量和FreeRTOS中的信号量稍有区别

static bool isInit = false;

void ledseq_init(void)
{
	int i = 0,j;

	if(isInit)
		return;
	/* initilize led */
	rt_hw_led_init();

	//初始化led序列
	for(i = 0; i < LED_NUM; i++)
	{
		activeSeq[i] = LEDSEQ_STOP;
		for(j = 0; j < SEQ_NUM; j ++)
			state[i][j] = LEDSEQ_STOP;
	}
	/* creat four soft timers for four LEDs .each timer use the same timeout function */
	for(i = 0; i < LED_NUM; i ++)
		timer_led[i] = rt_timer_create("ledseqTimer", 			/*定时器标识  "ledseqTimer"*/
										runLedseq,				/*超时函数回调函数*/
										(void *)&timer_led[i],	/*超时函数入口参数*///这里将每一个定时器句柄的地址取出送给超时函数
										1000, 					/*定时长度*/
										RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);/*定时方式为周期性定时  软件定时*/
							
	rt_sem_init(&ledseqSem , "ledseqSem", 1, RT_IPC_FLAG_FIFO);//初始化信号量，初始值为1，该信号量会被第一个持有的线程清除

	isInit = true;
}

/* start up the flash function of one led */
void ledseq_run(led_t led, ledseq_t *sequence)
{
	int prio = getPrio(sequence);

	if(prio < 0) return;

	rt_sem_take(&ledseqSem, RT_WAITING_FOREVER);
	state[led][prio] = 0;  //Reset the seq. to its first step
	updateActive(led);
	rt_sem_release(&ledseqSem);

	//Run the first step if the new seq is the active sequence
	if(activeSeq[led] == prio)
		runLedseq(&timer_led[led]);
}
/* stop the flash function of current led */
void ledseq_stop(led_t led, ledseq_t *sequence)
{
	int prio = getPrio(sequence);

	if(prio < 0) return;
	/* take the semaphore */
	rt_sem_take(&ledseqSem, RT_WAITING_FOREVER);
	/* */
	state[led][prio] = LEDSEQ_STOP;  //Stop the seq.
	updateActive(led);
	/* release the semaphore */
	rt_sem_release(&ledseqSem);
	//Run the next active sequence (if any...)
	runLedseq(&timer_led[led]);
}

/* Center of the led sequence machine. This function is executed by the FreeRTOS
 * timer and runs the sequences.this function is the time-out-handle
 */
static void runLedseq(void *parameter)
{
	ledseq_t *step;
	led_t led;
	/* find out which timer trigger the time-out function by 
	   comparing the parameter with the address of the handler of timer*/
	if(parameter == &timer_led[0])
		led = LED_RED;
	
	else if(parameter == &timer_led[1])
		led = LED_GREEN;
	
	else if(parameter == &timer_led[2])
		led = LED_NET;
	
	else if(parameter == &timer_led[3])
		led = LED_SNP;
	
	else if(parameter == &timer_led[4])
		led = LED_REC;
	
	else if(parameter == &timer_led[5])
		led = BEEP;

	rt_timer_t *xTimer = (rt_timer_t *)parameter;//note:解决了传递定时器句柄的问题
	
	bool leave=false;
	
	while(!leave) 
	{
		int prio = activeSeq[led];

		if (prio == LEDSEQ_STOP)
			return;

		step = &sequences[prio][state[led][prio]];
		
		state[led][prio]++;
		/* take the semaphore */
		rt_sem_take(&ledseqSem, RT_WAITING_FOREVER);
		switch(step->action)
		{
			case LEDSEQ_LOOP:
				state[led][prio] = 0;
			break;
			case LEDSEQ_STOP:
				state[led][prio] = LEDSEQ_STOP;
				updateActive(led);
				ledSet(led, step->value);
			break;
			default:  //The step is a LED action and a time
				ledSet(led, step->value);
				if (step->action == 0)	break;
				/* change the period of current timer for active LED */
				rt_timer_control( *xTimer , RT_TIMER_CTRL_SET_TIME, (void*)&step->action);/**/
				/* start timer */
				rt_timer_start(*xTimer);
				leave=true;
			break;
		}
		/* release the semaphore */
		rt_sem_release(&ledseqSem);
	}
}

/* Utility functions */
static int getPrio(ledseq_t *seq)
{
	int prio;
	/* Find the priority of the sequence */
	for(prio = 0; prio < SEQ_NUM; prio ++)
		if(sequences[prio] == seq) 
			return prio;

	return -1; //Invalid sequence
}

static void updateActive(led_t led)
{
	int prio;

	activeSeq[led]=LEDSEQ_STOP;
	ledSet(led, false);

	for(prio = 0; prio < SEQ_NUM; prio ++)
	{
		if (state[led][prio] != LEDSEQ_STOP)
		{
			activeSeq[led] = prio;
			break;
		}
	}
}


