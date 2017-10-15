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

static int buffer_size = 0;
static int exp_item = 0;
static int exp_item_to_receive = 0;


static void
initialize(void)
{
	for (int i = 0; i < NROF_ITEMS; i++) {
		localBuffer[i] = -1; // Initialize all condition variables
	}
}

static bool
checkBuffer(int start)
{
	for (int i = 0; i < sizeof(buffer); i++) {

		if (buffer[i] != start+i)
		{
			return false;
		}
	}
	return true;
}

/* producer thread */
static void *
producer (void * arg)
{
	printf("Producer starting...\n");
    ITEM current = 0;

    while (current != NROF_ITEMS)
    {

		pthread_mutex_lock(&buffer_lock);
		if (buffer_size >= BUFFER_SIZE)
		{
			printf("Buffer is full, waiting...\n");
//			rsleep(5000);
            pthread_cond_wait (&buffer_empty, &buffer_lock);
		}
		pthread_mutex_unlock(&buffer_lock);

        pthread_mutex_lock(&buffer_lock);
		if (exp_item == localBuffer[exp_item]) {
			printf("Item: %d has already been retrieved before, ", exp_item);
//            if (buffer_size >= 5) pthread_cond_wait(&buffer_empty, &buffer_lock);
            buffer[buffer_size] = localBuffer[exp_item];
            buffer_size++;
            exp_item++;
            printf("Putting in the buffer, buffer size: %d...\n", buffer_size);
//            rsleep(300000);
            pthread_cond_signal (&buffer_not_empty);
            pthread_mutex_unlock(&buffer_lock);
            printf("Unlocked the buffer\n");
			continue;
		}
        pthread_mutex_unlock(&buffer_lock);


        current = get_next_item();

        if (current == NROF_ITEMS) break;
        printf("Got item %d, processing...\n", current);
//		rsleep (300000);	// simulating all kind of activities...


		if (exp_item == current) {
			printf("Got the expected item: %d\n", current);
			pthread_mutex_lock(&buffer_lock);
//            if (buffer_size >= 5) pthread_cond_wait(&buffer_empty, &buffer_lock);
            buffer[buffer_size] = current;
			buffer_size++;
            exp_item++;
            printf("Putting in the buffer...\n");
//            rsleep(300000);
            pthread_cond_signal (&buffer_not_empty);
			pthread_mutex_unlock(&buffer_lock);
            printf("Unlocked the buffer\n");
        } else {
			printf("Got the wrong item: %d, storing locally...\n", current);
//            rsleep(300000);
            localBuffer[current] = current;
		}

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
static void *
consumer (void * arg)
{
    printf("Consumer started\n");
    while (true)
	{
        printf("Consumer start, expecting: %d\n", exp_item_to_receive);
		if (exp_item_to_receive == NROF_ITEMS) {
            pthread_cond_signal(&buffer_empty);
            pthread_mutex_unlock(&buffer_lock);
            break;
        }

        pthread_mutex_lock(&buffer_lock);
        if (buffer_size == 0) {
            printf("Starving\n");
            pthread_cond_wait(&buffer_not_empty, &buffer_lock);
        }

		if (buffer_size > 0) {
            printf("Buffer not empty, eating...\n");
//            pthread_mutex_unlock(&buffer_lock);
//			if (pthread_mutex_trylock(&buffer_lock) != 0) printf("Couldn't lock it\n");

//            printf("Locked the buffer of size: %d \n", buffer_size);
			for (int i = 0; i < (int) buffer_size; i++) {

				if (buffer[i] == exp_item_to_receive) {
					printf("%d\n", buffer[i]);
					exp_item_to_receive++;
				}
			}
			buffer_size = 0;
			pthread_cond_signal(&buffer_empty);
            printf("Unlocking the main mutex\n");
			pthread_mutex_unlock(&buffer_lock);

		}

//        rsleep (100);		// simulating all kind of activities...
    }
    printf("Consumer terminating...\n");
	return (NULL);
}

int main (void)
{
	printf("Main started\n");
	initialize();
	pthread_t producers[NROF_PRODUCERS];
    pthread_t consumerId;


    if (pthread_create(&consumerId, NULL, consumer, NULL) != 0) {
        printf("Error creating the consumer thread.");
    }

	for (int i = 0; i < NROF_PRODUCERS; i++) {
		pthread_create(&producers[i], NULL, producer, NULL);
		printf("Thread created\n");
//		rsleep(20000);
	}

    pthread_join(consumerId, NULL);
    printf("Joined consumer");

    for (int i = 0; i < 1; i++) {
		pthread_join(producers[i], NULL);
		printf("Thread joined");
//		rsleep(2000);
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


