#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int *used_time;
int cur_time;

int lru_evict() {
	int least_time = used_time[0];
	int victim = 0;
	int l;
	for (l = 0; l < memsize; l++) {
		if (used_time[l] < least_time) {
			least_time = used_time[l];
			victim = l;
		}
	}
	return victim;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	used_time[p->frame >> PAGE_SHIFT] = cur_time;
	cur_time++;
	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	used_time = (int *) malloc(sizeof(int) * memsize);
}
