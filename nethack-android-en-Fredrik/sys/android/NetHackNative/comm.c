/* comm.c */

#include "comm.h"

/* TODO: Think more about the +1 here. Is that really correct? */

static unsigned char s_MsgReceiveBuff[RECEIVEBUFFSZ + 1];
static volatile int s_MsgReceiveCnt;
/*static*/ sem_t s_ReceiveWaitingForConsumptionSema;
/*static*/ int s_ReceiveWaitingForConsumption;
/*static*/ pthread_mutex_t s_ReceiveMutex;
/*static*/ int s_ReceiveWaitingForData;
/*static*/ sem_t s_ReceiveWaitingForDataSema;


void android_msgq_init()
{
	s_MsgReceiveCnt = 0;
}


int android_msgq_is_empty()
{
	return s_MsgReceiveCnt == 0;
}


unsigned char android_msgq_pop_byte()
{
	unsigned char ret = s_MsgReceiveBuff[0];
	int i;

	/* Hmm, no good! */
	/* TODO: Switch to ring buffer!!! */
	for(i = 0; i < s_MsgReceiveCnt - 1; i++)
	{
		s_MsgReceiveBuff[i] = s_MsgReceiveBuff[i + 1];
	}
	s_MsgReceiveCnt--;

	return ret;
}


void android_msgq_push_byte(unsigned char c)
{
	while(s_MsgReceiveCnt >= RECEIVEBUFFSZ)
	{
		s_ReceiveWaitingForConsumption = 1;

		pthread_mutex_unlock(&s_ReceiveMutex);
		sem_wait(&s_ReceiveWaitingForConsumptionSema);

		pthread_mutex_lock(&s_ReceiveMutex);
	}

	s_MsgReceiveBuff[s_MsgReceiveCnt++] = c;
}


void android_msgq_wake_waiting()
{
	if(s_ReceiveWaitingForConsumption)
	{
		s_ReceiveWaitingForConsumption = 0;
		sem_post(&s_ReceiveWaitingForConsumptionSema);
	}
}


void android_msgq_begin_message(void)
{
	pthread_mutex_lock(&s_ReceiveMutex);
}


void android_msgq_end_message(void)
{
	if(s_ReceiveWaitingForData)
	{
		s_ReceiveWaitingForData = 0;
		sem_post(&s_ReceiveWaitingForDataSema);
	}

	pthread_mutex_unlock(&s_ReceiveMutex);
}

/* End of file comm.c */
