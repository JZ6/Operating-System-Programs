#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include <string.h>


extern int memsize;

extern int debug;

extern struct frame *coremap;

int *order;
extern char *tracefile;
int lines;
addr_t *vads;
int cur_line;


/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	int evictee = 0;
	int max_occ = 0;
	int cur_ind;
	int occ;

	for (cur_ind = 0; cur_ind < memsize; cur_ind++) {
		int found_nxt_ref = 0;
		for (occ = cur_line; occ < lines; occ++) {
			if (order[cur_ind] == vads[occ]) {   //if the virtual address gets referenced again later
				found_nxt_ref = 1;
				int nxt_ref = occ - cur_line;
				if (nxt_ref > max_occ) {    //Find the virtual address that gets referenced again last.
					max_occ = nxt_ref;
					evictee = cur_ind;
				}
				break;
			}
		}
		if (!found_nxt_ref) {  //If virtual address doesn't get referenced again, evict it.
			return cur_ind;
		}
	}
	return evictee;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	order[p->frame >> PAGE_SHIFT] = vads[cur_line];
	cur_line++;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	FILE *tracef;
	if ((tracef = fopen(tracefile, "r")) == NULL) {
		perror("Error opening file");
		exit(1);
	}
	fseek(tracef, 0, SEEK_END);
	lines = ftell(tracef);
	rewind(tracef);

	order = (int *) malloc(sizeof(int) * memsize);
	vads = malloc(lines * sizeof(addr_t));
	char tb[256];
	char access;
	addr_t va;
	int i = 0;
	while (fgets(tb, 256, tracef) != NULL) {
		sscanf(tb, "%c %lx", &access, &va);
		vads[i] = va;
		i++;
	}
	fclose(tracef);
	return;
}
