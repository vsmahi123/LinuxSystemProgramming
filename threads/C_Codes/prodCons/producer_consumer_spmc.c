#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<stdint.h>
#include<sys/time.h>
#include<errno.h>

int data;
pthread_cond_t produced = PTHREAD_COND_INITIALIZER;
pthread_cond_t consumed = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void *producerThread(void *arg);
void *consumerThread(void *arg);


void *producerThread(void *arg)
{
	int producerNo = (intptr_t)arg;
	for(int i=0;i<1000;i++)
	{
		pthread_mutex_lock(&mtx);
		while(data > 0)
		{	
			pthread_cond_wait(&consumed,&mtx);
		}
		data+=1000;
		printf("producer[%d] produced %d data %d\n",producerNo,i,data);
		
		pthread_mutex_unlock(&mtx);
		
		pthread_cond_broadcast(&produced);
		//pthread_cond_signal(&produced);
	//	sleep(1);
	}
	
	return NULL;
}

void *consumerThread(void *arg)
{

	int consumerNo=(intptr_t)arg;
	struct timespec maxwait={5,5};

	int ret=0;
	while(1)
	{
		pthread_mutex_lock(&mtx);
		
		clock_gettime(CLOCK_REALTIME, &maxwait);
		int currtime = maxwait.tv_sec;
		maxwait.tv_sec+=2;
		while(data == 0)
		{	
			ret = pthread_cond_timedwait(&produced,&mtx,&maxwait);
			//ret = pthread_cond_wait(&produced,&mtx);
			if (ret == ETIMEDOUT)
			{
				printf("\t\t\t!!!!! Consumer %d exiting as producer not produced for %d sec\n",consumerNo,(int)maxwait.tv_sec - currtime);
				pthread_mutex_unlock(&mtx);
				return NULL;
			}
		}
		printf("consumer[%d] consumed data %d\n",consumerNo,data);
		data--;


		pthread_mutex_unlock(&mtx);
		pthread_cond_signal(&consumed);
	}

	printf("\t\t\t!!!!! Consumer %d consumed %d and exit\n",consumerNo,100);
	return NULL;
}
	


void main(void)
{
	pthread_t producer;
	pthread_t consumer[5];

	pthread_create(&producer,NULL,producerThread,(void *)(intptr_t)1);
	pthread_create(&consumer[0],NULL,consumerThread,(void *)(intptr_t)1);
	pthread_create(&consumer[1],NULL,consumerThread,(void *)(intptr_t)2);
	pthread_create(&consumer[2],NULL,consumerThread,(void *)(intptr_t)3);
	consumerThread((intptr_t)0);
	pthread_join(producer,NULL);
	pthread_join(consumer[0],NULL);
	pthread_join(consumer[1],NULL);
	pthread_join(consumer[2],NULL);

}
