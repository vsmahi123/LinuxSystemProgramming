#include<stdio.h>
#include<unistd.h>
#include<pthread.h>

int data;
pthread_cond_t produced = PTHREAD_COND_INITIALIZER;
pthread_cond_t consumed = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void *producerThread(void *arg);
void *consumerThread(void *arg);


void *producerThread(void *arg)
{
	char *str = (char *)arg;
	for(int i=0;i<10000;i++)
	{
		pthread_mutex_lock(&mtx);
		while(data > 0)
		{	
			pthread_cond_wait(&consumed,&mtx);
		}
		data++;
		printf("produced %d data %d\n",i,data);

		pthread_mutex_unlock(&mtx);
		pthread_cond_signal(&produced);
		sleep(1);
	}
	
	return NULL;
}
void *consumerThread(void *arg)
{	for(int i =0;i<10000;i++)
	{
		pthread_mutex_lock(&mtx);
		
		while(data == 0)
		{	
			pthread_cond_wait(&produced,&mtx);
		}
		data--;
		printf("consumed %d data %d\n",i,data);


		pthread_mutex_unlock(&mtx);
		pthread_cond_signal(&consumed);
	}
	return NULL;
}
	


void main(void)
{
	pthread_t producer;

	pthread_create(&producer,NULL,producerThread,NULL);
	consumerThread(NULL);
	pthread_join(producer,NULL);

}
