#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

char *knowledge;
int brain_size;
int grade;

void Usage(char prog_name[]) {
	fprintf(stderr, "usage:  %s <Number of Midterms> \n",
	        prog_name);
	exit(0);
}  /* Usage */

void relaxing(int time) {       //Dont sleep too much
	sleep(time);
}

void cramming(int time) {
	static const char alphanum[] =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"abcdefghijklmnopqrstuvwxyz";
	int i;
	for (i = 0; i < time; i++) {
		int remember = brain_size - rand() % 100;
		knowledge[remember] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	knowledge[time] = 0;
}

void testing(int time) {
	int answer = 3;     //It's always C
	int i;
	for (i = 0; i < time; i++) {
		int guess = rand() % 5;
		if(knowledge[i%brain_size] == 'c' || answer == guess){
			grade++;
		}
	}
}

int main(int argc, char **argv) {
	/* Markers used to bound trace regions of interest */
	volatile char MARKER_START, MARKER_END;
	/* Record marker addresses */
	FILE *marker_fp = fopen("simpleloop.marker", "w");
	if (marker_fp == NULL) {
		perror("Couldn't open marker file:");
		exit(1);
	}
	fprintf(marker_fp, "%p %p", &MARKER_START, &MARKER_END);
	fclose(marker_fp);
	MARKER_START = 33;

	if (argc != 2) Usage(argv[0]);
	int midterms = strtol(argv[1], NULL, 10);
	brain_size = midterms * 100;
	knowledge = calloc(brain_size, sizeof(char));

	relaxing(1);
	int m;
	for (m = 0; m < midterms - 1; m++) {
		cramming(midterms * 10);
		testing(midterms * 100);
		cramming(midterms * 10);
	}
	testing(midterms * 1000);
	relaxing(1);

	MARKER_END = 34;
	return 0;
}
