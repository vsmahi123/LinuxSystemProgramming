/*
 *what happens when the buffer is half full, and a massive burst of new data arrives that exceeds the remaining space?

The hardware and the C language don't automatically solve this for you. As the systems architect, you have to explicitly code one of three strict overflow policies based on what your system is trying to achieve.

Here is exactly what happens to those overflowed elements under the three standard architectures:
Policy 1: Block the Producer (The "Reliable" Policy)
---------------------------------------------------------
If you are transferring critical data that absolutely cannot be lost (like a file transfer, a database write, or your MPSC math calculations from earlier), you implement Backpressure.

    What happens to the overflow data: It stays waiting in the Producer's hands.

    How it works: The Producer writes data until the Write Pointer literally bumps into the Read Pointer. At that exact moment, the buffer is declared "Full." The Producer is forced to go to sleep (using a condition variable) and waits.

    The Result: The overflow elements are perfectly safe, but the Producer is stalled until the Consumer wakes up and eats some data to free up space.

Policy 2: Drop the Newest Data (The "Network" Policy)
---------------------------------------------------------------
If you are designing a high-speed network router or an Ethernet driver, holding up the Producer is usually impossible. The internet doesn't stop sending packets just because your buffer is full.

    What happens to the overflow data: It is instantly deleted/ignored.

    How it works: The Producer checks the available space. It writes what it can until the Write Pointer hits the Read Pointer. Any data left over in the new request is simply dropped on the floor.

    The Result: The Consumer gets a perfectly continuous stream of the oldest, earliest data, but the newest data is permanently lost. (This is exactly what causes "Packet Loss" in video games and Zoom calls).

Policy 3: Overwrite the Oldest Data (The "True Ring" Policy)
------------------------------------------------------------------
If you are designing a system like a kernel crash logger (dmesg), an airplane black box, or a live video feed, old data is worthless. You only care about the absolute newest data.

    What happens to the overflow data: It gets written into the buffer, mercilessly crushing the old, unread data.

    How it works: The Write Pointer never stops and never checks for space. When the burst arrives, the Write Pointer laps the Read Pointer and just keeps writing over the old elements.

    The Catch (The Ghost Pointer): To prevent the Consumer from reading corrupted, half-overwritten garbage, the Producer forcefully pushes the Read Pointer forward as it overwrites data.

    The Result: The newest overflow elements are saved perfectly. The oldest elements are destroyed before the Consumer ever got to see them.
 *
 *
 */



#include<stdio.h>
#include<stdint.h>
#include<pthread.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/time.h>
#include<time.h>
#include<string.h>
#include<unistd.h>
#include<string.h>

typedef enum ringStat
{
	RING_EMPTY=0,
	RING_FULL
}ringstat_t;

typedef enum ringpolicy
{
	BLOCK_PRODUCER = 0,
	DISCARD_OVERFLOWD,
	DISCARD_OLD
}ringpolicy_t;


#define RING_MAX_ELEM 10
#define MAXWAIT 5
#define NR_PROD 5
#define NR_CONS 1
typedef struct ringBuff
{
	int ring[RING_MAX_ELEM];
	int writerIndex;
	int readerIndex;
	int total;
	pthread_mutex_t wmtx;
	pthread_cond_t prodCond;

}ringbuff_t;

ringstat_t isRingEmpty(ringbuff_t *ring);
ringstat_t isRingFull(ringbuff_t *ring);
void discardOldIfRingFull(int *arr,int elem);
void discardRemainingIfRingFull(int prod,int *arr,int elem);
void blockProducerIfRingFull(int *arr,int elem);
int getNewElem(int *arr);


ringbuff_t ring = { .ring = {0,},
		    .writerIndex = 0,
		    .readerIndex = 0,
		    .total = 0,
		    .wmtx = PTHREAD_MUTEX_INITIALIZER,
		    .prodCond = PTHREAD_COND_INITIALIZER};

void discardOldIfRingFull(int *arr,int elem)
{
	printf("called %s \n",__func__);

}

// mtx is for total - considering 1 p 1 c
void discardRemainingIfRingFull(int prod,int *arr,int elem)
{
	int i=0;
	pthread_mutex_lock(&ring.wmtx);
	while(RING_FULL != isRingFull(&ring) && elem>i)
	{
		ring.ring[ring.writerIndex] = arr[i++];
		ring.total++; //mtx is for total 
		printf("prod [%d]writing to ring[%d] = %d total=%d\n",prod,ring.writerIndex,ring.ring[ring.writerIndex],ring.total);
		ring.writerIndex = (ring.writerIndex+1)%RING_MAX_ELEM;
	}
	pthread_mutex_unlock(&ring.wmtx);
	pthread_cond_broadcast(&ring.prodCond);
	return;
}
void blockProducerIfRingFull(int *arr,int elem)
{
	
	printf("called %s \n",__func__);
}

ringstat_t isRingFull(ringbuff_t *ring)
{
	return ring->total == RING_MAX_ELEM?RING_FULL:RING_EMPTY;
}

ringstat_t isRingEmpty(ringbuff_t *ring)
{
	return ring->total == 0?RING_EMPTY:RING_FULL; // always checking with RING_EMPTY 
}

void *elemProducer(void *arg)
{
	int arr[RING_MAX_ELEM] = {0,};
	int elemToPush = 0;
	int prod = (intptr_t)arg;
	ringpolicy_t policy = DISCARD_OVERFLOWD;
	printf("prod[%d]: producer %d alive!!\n",prod,prod);
	int maxit = 10;
	while(maxit)
	{
		elemToPush = getNewElem(arr);
		printf("prod[%d]: new elem to push %d\n",prod,elemToPush);
		switch(policy)
		{
			case	BLOCK_PRODUCER:
					blockProducerIfRingFull(arr,elemToPush);
					break;

			case DISCARD_OVERFLOWD:
					discardRemainingIfRingFull(prod,arr,elemToPush);
					break;

			case DISCARD_OLD:
					discardOldIfRingFull(arr,elemToPush);
					break;

			default:
					discardRemainingIfRingFull(prod,arr,elemToPush);
					break;


		}
		maxit--;
		//sleep(1);
	}
	return NULL;
}

void *elemConsumer(void *arg)
{
	time_t initial,now,diff;
	int locarr[RING_MAX_ELEM] = {0};
	int locItr=0; 
	int ret=0;
	struct timespec maxwait;
	//pthread_attr_t attr;
	//pthread_attr_getschedpolicy(pthread_self(),)
	printf("cons[%ld]: consumer %ld alive!!\n",(intptr_t)arg,(intptr_t)arg);
	//time(&initial);
	while(1)
	{
		pthread_mutex_lock(&ring.wmtx);
		clock_gettime(CLOCK_REALTIME,&maxwait);
		maxwait.tv_sec += MAXWAIT;
		while(RING_EMPTY == isRingEmpty(&ring))
		{
			ret = pthread_cond_timedwait(&ring.prodCond,&ring.wmtx,&maxwait);
			if(ret == ETIMEDOUT)
			{
				printf("cons[%ld]: roducers not produced for %d secs , Exiting!!!\n",(intptr_t)arg,MAXWAIT);
				return NULL;
			}
		}
		while(RING_EMPTY!= isRingEmpty(&ring))
		{
			
			//time(&initial);
			printf("cons[%ld]: adding ring[%d] at loc[%d]\n",(intptr_t)arg,ring.readerIndex,locItr);
			locarr[locItr++]=ring.ring[ring.readerIndex];
			ring.readerIndex = (ring.readerIndex+1)%RING_MAX_ELEM;
			--ring.total;//mtx for total
		}
		pthread_mutex_unlock(&ring.wmtx);
		//time(&now);
		printf("cons[%ld]: reading ring elems =",(intptr_t)arg);
		for(int i=0;i<locItr;i++)
		{
			printf("%d ",locarr[i]);
		}
		locItr=0;
		printf("\n\n");

		/*diff = difftime(now,initial);
		if(diff>5) //5sec
		{
		printf("time init = %ld   time now =%ld\n",initial,now);
			break;
		}*/
	}

	return NULL;
}




int getNewElem(int *arr)
{
	int max=0;
	max = rand()%RING_MAX_ELEM;
	max = max>=0?max:RING_MAX_ELEM;

	for(int i=0;i<max;i++)
	{
		arr[i] = rand()%(i+1);
	}

	return max;
}

void main()
{
	pthread_t eProducer[5],eConsumer;
	pthread_attr_t attr;
 	pthread_attr_init(&attr);
	struct sched_param param; 
	memset(&param, 0, sizeof(param));
	pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
	param.sched_priority = 60;
	pthread_attr_setschedparam(&attr,&param);

	//mutex initialization
	pthread_mutexattr_t mtxattr;
	pthread_mutexattr_init(&mtxattr);
	
	pthread_mutexattr_setprotocol(&mtxattr,PTHREAD_PRIO_INHERIT);
	pthread_mutex_init(&ring.wmtx,&mtxattr);



	for(int i=0;i<NR_PROD;i++)
	{	
		pthread_create(&eProducer[i],NULL,elemProducer,(void*)(intptr_t)i+1);
	}

	int ret = pthread_create(&eConsumer,&attr,elemConsumer,(void*)(intptr_t)1);
	if (ret != 0) {
        // If it fails, print the exact kernel error and EXIT immediately!
        fprintf(stderr, "\nFATAL: Failed to create SCHED_FIFO Consumer: %s\n", strerror(ret));
        fprintf(stderr, "Are you running on WSL or Docker?\n\n");
        exit(EXIT_FAILURE); 
    }
	for(int i=0;i<NR_PROD;i++)
	{	
		pthread_join(eProducer[i],NULL);
	}
	pthread_join(eConsumer,NULL);

		
}
