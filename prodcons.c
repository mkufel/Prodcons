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

static ITEM get_next_item (void);	// already implemented (see below)

static pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER; // mutex for the buffer

static pthread_cond_t buffer_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;

static ITEM buffer[BUFFER_SIZE];
static ITEM side_buffer[NROF_ITEMS];	// buffer to store items retrieved out of order

static int buffer_size = 0;
static int exp_item = 0;	// used by producers to keep track of the next expected item
static int exp_item_to_consume = 0;	// used by the consumer to keep track of the next item that should be consumed


/*
 * initialize()
 *
 * Initializes values of all items in the side buffer to -1 to make sure the initial value of an elt of the array
 * does not correspond to any expected item.
 */
static void
initialize()
{
	for (int i = 0; i < NROF_ITEMS; i++) {
		side_buffer[i] = -1;
	}
}


/*
 * buffer_check()
 *
 * @pre buffer_lock has been locked by the caller.
 * Checks if there is remaining space in the buffer.
 */
static void
buffer_check()
{
    if (buffer_size >= BUFFER_SIZE)
    {
        if (pthread_cond_wait (&buffer_empty, &buffer_lock) != 0) {
			perror ("Producer waiting for buffer_empty failed\n");
		}
    }
}


/* producer thread */
static void *
producer (void * arg)
{
    ITEM current = 0;

    while (current != NROF_ITEMS)
    {
		if (pthread_mutex_lock(&buffer_lock) != 0) {
			perror ("Producer cannot lock the buffer_lock\n");
		}

		buffer_check();

		if (exp_item == side_buffer[exp_item]) {	// Check if the expected item has already been retrieved before
			buffer[buffer_size] = side_buffer[exp_item];

			buffer_size++;
            exp_item++;

            if (pthread_cond_signal (&buffer_not_empty) != 0) {
				perror ("Producer cannot send the buffer_not_empty signal after storing an old item\n");
			}

            if (pthread_mutex_unlock(&buffer_lock) != 0) {
				perror ("Producer cannot unlock the buffer_lock after storing an old item\n");
			}

			continue;	//skip the rest of the loop
		}

		if (pthread_mutex_unlock(&buffer_lock) != 0) {
			perror ("Producer cannot unlock the buffer_lock\n");
		}

        current = get_next_item();

        if (current == NROF_ITEMS) break;	// if all items retrieved, break


		if (exp_item == current) {	//if got the exp_item, store in the buffer

			if (pthread_mutex_lock(&buffer_lock) != 0) {
				perror ("Producer cannot lock the buffer_lock before storing a new item\n");
			}

			buffer_check();
			buffer[buffer_size] = current;

			buffer_size++;
            exp_item++;

			if (pthread_cond_signal (&buffer_not_empty) != 0) {
				perror ("Producer cannot send the buffer_not_empty signal after storing a new item\n");
			}

			if (pthread_mutex_unlock(&buffer_lock) != 0) {
				perror ("Producer cannot unlock the buffer_lock after storing a new item\n");
			}

        } else {	// if got an item different than exp_item, store in the side buffer
            side_buffer[current] = current;
		}
	}

	return (NULL);
}


/* consumer thread */
static void *
consumer (void * arg)
{
    while (true)
	{
		if (exp_item_to_consume == NROF_ITEMS) {	// if already consumed all expected items, break
            break;
        }

        if (pthread_mutex_lock(&buffer_lock) != 0) {
			perror ("Consumer cannot lock the buffer_lock\n");
		}

        if (buffer_size == 0) {		//if buffer empty, wait for signal buffer_not_empty

			if(pthread_cond_wait(&buffer_not_empty, &buffer_lock) != 0) {
				perror ("Consumer waiting for buffer_not_empty failed\n");
			}
		}

		if (buffer_size > 0) {	// if buffer not empty
			for (int i = 0; i < buffer_size; i++) {

				if (buffer[i] == exp_item_to_consume) {	//consume the next item if equal to exp_item_to_consume
					printf("%d\n", buffer[i]);
					exp_item_to_consume++;
				}
			}

			buffer_size = 0;	// all item have been consumed, set the size to 0
		}

		//brodcast the signal, because all the producers are waiting to start operating on the buffer again
        if (pthread_cond_broadcast(&buffer_empty) != 0) {
			perror ("Consumer cannot send the buffer_empty signal\n");
		}

		if(pthread_mutex_unlock(&buffer_lock) != 0) {
			perror ("Consumer cannot unlock the buffer_lock\n");
		}
    }

	return (NULL);
}


int main (void)
{
	pthread_t producers[NROF_PRODUCERS];
    pthread_t consumerId;

	initialize();

	if (pthread_create(&consumerId, NULL, consumer, NULL) != 0) {	// create the consumer
        printf("Error creating the consumer thread.");
    }

	for (int i = 0; i < NROF_PRODUCERS; i++) {	// create producers

		if (pthread_create(&producers[i], NULL, producer, NULL) != 0) {
			printf("Error creating a producer thread.");
		}
	}

    if (pthread_join(consumerId, NULL) != 0) { // wait for the consumer to terminate
		printf("Error joining the consumer thread.");
	}

    for (int i = 0; i < NROF_PRODUCERS; i++) {	// wait for producers to terminate

		if (pthread_join(producers[i], NULL) != 0) {
			printf("Error joining a producer thread.");
		}
	}

    return (0);
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


