/* comm.h */

#ifndef ANDROID_COMM_H
#define ANDROID_COMM_H

/* TEMP */
#include <semaphore.h>
#include <pthread.h>


#define RECEIVEBUFFSZ 255
#define SENDBUFFSZ 1023

/* TEMP */
extern sem_t s_ReceiveWaitingForConsumptionSema;
extern int s_ReceiveWaitingForConsumption;
extern pthread_mutex_t s_ReceiveMutex;
extern int s_ReceiveWaitingForData;
extern sem_t s_ReceiveWaitingForDataSema;

void android_msgq_push_byte(unsigned char c);
void android_msgq_init();
int android_msgq_is_empty();
unsigned char android_msgq_pop_byte();
void android_msgq_push_byte(unsigned char c);
void android_msgq_wake_waiting();
void android_msgq_begin_message(void);
void android_msgq_end_message(void);

#endif	/* ANDROID_COMM_H */

/* End of file comm.h */
