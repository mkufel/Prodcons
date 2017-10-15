/* 
 * Operating Systems [2IN05] Practical Assignment
 * Condition Variable application
 *
 * Joris Geurts
 * j.h.j.geurts@tue.nl
 *
 */

/* ITEM:
 * a job identification (values 1..NROF_ITEMS-1)
 */
typedef int      				ITEM;
 
/* NROF_PRODUCERS:
 * number of producers
 *
 * When editing this file: ensure that NROF_PRODUCERS > 0
 */
#define NROF_PRODUCERS          10

/* NROF_ITEMS
 * number of jobs to be handled by the producers
 */
#define NROF_ITEMS              20

/* BUFFER_SIZE
 * number of items that fit in the buffer
 *
 * When editing this file: ensure that BUFFER_SIZE > 1
 */
#define BUFFER_SIZE             5

