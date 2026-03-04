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
		/*while(data > 0)
		{	
			pthread_cond_wait(&consumed,&mtx);
		}*/
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
	int dataToConsume=0;
	int ret=0;
	while(1)
	{
		pthread_mutex_lock(&mtx);
		
		clock_gettime(CLOCK_REALTIME, &maxwait);
		int currtime_sec = maxwait.tv_sec;
		int currtime_nsec= maxwait.tv_nsec;
		maxwait.tv_sec+=60;
		while(data == 0)
		{	
			ret = pthread_cond_timedwait(&produced,&mtx,&maxwait);
			//ret = pthread_cond_wait(&produced,&mtx);
			if (ret == ETIMEDOUT)
			{
				printf("\t\t\t!!!!! Consumer %d exiting as producer not produced for %ld sec\n",consumerNo,maxwait.tv_sec - currtime_sec);
				pthread_mutex_unlock(&mtx);
				return NULL;
			}
		}
		printf("\t\t\t\tconsumer[%d] consuming data %d\n",consumerNo,data);
		if(data) 
		{
			dataToConsume =data;
			data =0;
		}


		pthread_mutex_unlock(&mtx);
		while(dataToConsume)
		dataToConsume--;
		struct timespec consumerTime;
		clock_gettime(CLOCK_REALTIME, &consumerTime);
		printf("\t\t\t!!!!! Consumer %d consumed in %ld sec %ld nsec and exit\n",consumerNo,consumerTime.tv_sec - currtime_sec,consumerTime.tv_nsec - currtime_nsec);
		//pthread_cond_signal(&consumed);
	}

	return NULL;
}
	


void main(void)
{
	pthread_t producer[5];
	pthread_t consumer[5];

	pthread_create(&producer[0],NULL,producerThread,(void *)(intptr_t)1);
	pthread_create(&producer[1],NULL,producerThread,(void *)(intptr_t)2);
	pthread_create(&producer[2],NULL,producerThread,(void *)(intptr_t)3);
	pthread_create(&producer[3],NULL,producerThread,(void *)(intptr_t)4);
	pthread_create(&producer[4],NULL,producerThread,(void *)(intptr_t)5);
	pthread_create(&consumer[0],NULL,consumerThread,(void *)(intptr_t)1);

	pthread_join(producer[0],NULL);
	pthread_join(producer[1],NULL);
	pthread_join(producer[2],NULL);
	pthread_join(producer[3],NULL);
	pthread_join(producer[4],NULL);
	pthread_join(consumer[0],NULL);

}
