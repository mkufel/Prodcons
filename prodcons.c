/* 
 * Operating Systems [2INCO] Practical Assignment
 * Condition Variables Application
 *
 * Ahmed Ahres (0978238)
 * Maciej Kufel (0944597)
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * "Extra" steps can lead to higher marks because we want students to take the initiative. 
 * Extra steps can be, for example, in the form of measurements added to your code, a formal 
 * analysis of deadlock freeness etc.
 */
 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "prodcons.h"

static ITEM buffer[BUFFER_SIZE];
static ITEM localBuffer[NROF_ITEMS];

static void rsleep (int t);			// already implemented (see below)
static ITEM get_next_item (void);	// already implemented (see below)

//static pthread_cond_t buffer_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;


static pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER; // for main buffer
static pthread_mutex_t  	localMutex 		   = PTHREAD_MUTEX_INITIALIZER; // for local buffer

static pthread_cond_t buffer_empty = PTHREAD_COND_INITIALIZER;

static int bufferSize = 0;
static int expectedItem = 0;


static void
initialize(void)
{
	for (int i = 0; i < NROF_ITEMS; i++) {
		localBuffer[i] = -1; // Initialize all condition variables
	}
}

//static bool
//checkBuffer(int start)
//{
//	for (int i = 0; i < sizeof(buffer); i++) {
//
//		if (buffer[i] != start+i)
//		{
//			return false;
//		}
//	}
//	return true;
//}

/* producer thread */
static void *
producer (void * arg)
{
	printf("Thread starting...\n");

	int current = 0;
	printf("Number of items: %d\n", NROF_ITEMS);
	printf("Number of currents: %d\n", current);
	bool condition = (current == NROF_ITEMS);
	printf("%d\n", condition);

//	while (current == NROF_ITEMS)
//	{
//		printf("Inside");
////		bool condition = (current = NROF_ITEMS);
////		printf("Current: %d and NR_OF_ITEMS: %d are equal: %d", current, NROF_ITEMS, condition);
//		current++;
//	}

    while (current != NROF_ITEMS)
    {

//		pthread_mutex_lock(&buffer_lock);
//		if (bufferSize == BUFFER_SIZE)
//		{
//			printf("Buffer is full, waiting...\n");
//			pthread_cond_wait (&buffer_empty, &buffer_lock);
//		}
//		pthread_mutex_unlock(&buffer_lock);


		if (expectedItem == localBuffer[expectedItem]) {
			printf("Item: %d has already been retrieved before\n", expectedItem);
			expectedItem++;
			continue;
		}
		current = get_next_item();
		printf("Got item %d, processing...\n", current);
		ITEM retrieved = NULL;
		rsleep (3000);	// simulating all kind of activities...

		pthread_mutex_lock (&localMutex);
		if (localBuffer[expectedItem] == expectedItem) {
			retrieved = localBuffer[expectedItem];
		}
		pthread_mutex_unlock(&localMutex);

		if (retrieved != NULL) {
			printf("Retrieved item: %d from the local buffer\n", retrieved);
			pthread_mutex_lock(&buffer_lock);
			buffer[bufferSize] = retrieved;
			bufferSize++;
			expectedItem++;
			pthread_cond_signal(&buffer_not_empty);
			pthread_mutex_unlock(&buffer_lock);
		} else if (expectedItem == current) {
			printf("Got the expected item: %d\n", current);
			pthread_mutex_lock(&buffer_lock);
			buffer[bufferSize] = current;
			bufferSize++;
			expectedItem++;
			pthread_cond_signal (&buffer_not_empty);
			pthread_mutex_unlock(&buffer_lock);
		} else {
			printf("Got the wrong item: %d, storing locally\n", current);
			localBuffer[current] = current;
		}
		retrieved = NULL;
	}
	printf("Left the loop\n");
	// TODO:
	// * put the item into buffer[]
	//
	// follow this pseudocode (according to the ConditionSynchronization lecture):
	//      mutex-lock;
	//      while not condition-for-this-producer
	//          wait-cv;
	//      critical-section;
	//      possible-cv-signals;
	//      mutex-unlock;
	//
	// (see condition_test() in condition_basics.c how to use


	return (NULL);
}

///* consumer thread */
//static void *
//consumer (void * arg)
//{
//
//    while (true)
//	{
//		if (expectedItem == NROF_ITEMS) break;
//
//		if (bufferSize > 0) {
//			pthread_mutex_lock(&mainMutex);
//
//			for (int i = 0; i < bufferSize; i++) {
//
//				if (buffer[i] == expectedItem) {
//					printf("%d\n", buffer[i]);
//					expectedItem++;
//				}
//			}
//			bufferSize = 0;
//			pthread_cond_signal(&buffer_empty);
//			pthread_mutex_unlock(&mainMutex);
//
//		} else {
//			pthread_cond_wait(&buffer_not_empty, &mainMutex);
//		}
//
//        rsleep (100);		// simulating all kind of activities...
//    }
//	return (NULL);
//}

int main (void)
{
	printf("Main started\n");
	initialize();
	pthread_t producerThread[NROF_PRODUCERS];

	for (int i = 0; i < 1; i++) {
		pthread_create(&producerThread[i], NULL, producer, NULL);
		printf("Thread created\n");
		rsleep(20000);
	}
	for (int i = 0; i < 1; i++) {
		pthread_join(producerThread[i], NULL);
		printf("Thread joined");
		rsleep(2000);
	}
//
//	pthread_t consumerThread;
//	pthread_create(&consumerThread, NULL, consumer, NULL);
//	pthread_join(&consumerThread, NULL);

    return (0);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void 
rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time(NULL));
        first_call = false;
    }
    usleep (random () % t);
}


/* 
 * get_next_item()
 *
 * description:
 *		thread-safe function to get a next job to be executed
 *		subsequent calls of get_next_item() yields the values 0..NROF_ITEMS-1 
 *		in arbitrary order 
 *		return value NROF_ITEMS indicates that all jobs have already been given
 * 
 * parameters:
 *		none
 *
 * return value:
 *		0..NROF_ITEMS-1: job number to be executed
 *		NROF_ITEMS:		 ready
 */
static ITEM
get_next_item(void)
{
    static pthread_mutex_t	job_mutex	= PTHREAD_MUTEX_INITIALIZER;
	static bool 			jobs[NROF_ITEMS+1] = { false };	// keep track of issued jobs
	static int              counter = 0;    // seq.nr. of job to be handled
    ITEM 					found;          // item to be returned
	
	/* avoid deadlock: when all producers are busy but none has the next expected item for the consumer 
	 * so requirement for get_next_item: when giving the (i+n)'th item, make sure that item (i) is going to be handled (with n=nrof-producers)
	 */
	pthread_mutex_lock (&job_mutex);

    counter++;
	if (counter > NROF_ITEMS)
	{
	    // we're ready
	    found = NROF_ITEMS;
	}
	else
	{
	    if (counter < NROF_PRODUCERS)
	    {
	        // for the first n-1 items: any job can be given
	        // e.g. "random() % NROF_ITEMS", but here we bias the lower items
	        found = (random() % (2*NROF_PRODUCERS)) % NROF_ITEMS;
	    }
	    else
	    {
	        // deadlock-avoidance: item 'counter - NROF_PRODUCERS' must be given now
	        found = counter - NROF_PRODUCERS;
	        if (jobs[found] == true)
	        {
	            // already handled, find a random one, with a bias for lower items
	            found = (counter + (random() % NROF_PRODUCERS)) % NROF_ITEMS;
	        }    
	    }
	    
	    // check if 'found' is really an unhandled item; 
	    // if not: find another one
	    if (jobs[found] == true)
	    {
	        // already handled, do linear search for the oldest
	        found = 0;
	        while (jobs[found] == true)
            {
                found++;
            }
	    }
	}
    jobs[found] = true;
			
	pthread_mutex_unlock (&job_mutex);
	return (found);
}


