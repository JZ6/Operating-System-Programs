#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "traffic.h"

extern struct intersection isection;

/**
 * Populate the car lists by parsing a file where each line has
 * the following structure:
 *
 * <id> <in_direction> <out_direction>
 *
 * Each car is added to the list that corresponds with 
 * its in_direction
 * 
 * Note: this also updates 'inc' on each of the lanes
 */
void parse_schedule(char *file_name) {
	int id;
	struct car *cur_car;
	struct lane *cur_lane;
	enum direction in_dir, out_dir;
	FILE *f = fopen(file_name, "r");

	/* parse file */
	while (fscanf(f, "%d %d %d", &id, (int*)&in_dir, (int*)&out_dir) == 3) {

		/* construct car */
		cur_car = malloc(sizeof(struct car));
		cur_car->id = id;
		cur_car->in_dir = in_dir;
		cur_car->out_dir = out_dir;

		/* append new car to head of corresponding list */
		cur_lane = &isection.lanes[in_dir];
		cur_car->next = cur_lane->in_cars;
		cur_lane->in_cars = cur_car;
		cur_lane->inc++;
	}

	fclose(f);
}

/**
 *
 * Do all of the work required to prepare the intersection
 * before any cars start coming
 * 
 */

void init_intersection() {
	int i;
	for (i = 0; i < 4; i++){
		pthread_mutex_init(&isection.quad[i], NULL);

		struct lane *lane_init = &isection.lanes[i];    //Initialize lane using pointer to lane.
		pthread_mutex_init(&lane_init->lock , NULL);
		pthread_cond_init(&lane_init->producer_cv, NULL);
		pthread_cond_init(&lane_init->consumer_cv, NULL);
		lane_init->in_cars = NULL;
		lane_init->out_cars = NULL;
		lane_init->inc = 0;
		lane_init->passed = 0;
		lane_init->buffer = malloc(sizeof(struct car*) * LANE_LENGTH);
		lane_init->head = 0;
		lane_init->tail = 0;
		lane_init->capacity = LANE_LENGTH;
		lane_init->in_buf = 0;
	}
}

/**
 * TODO: Fill in this function
 *
 * Populates the corresponding lane with cars as room becomes
 * available. Ensure to notify the cross thread as new cars are
 * added to the lane.
 * 
 */
void *car_arrive(void *arg) {       //Producer
	struct lane *l = arg;
	while(l->in_cars){      //run until no cars left in in_cars
		pthread_mutex_lock(&(l->lock));
		while (l->in_buf >= l->capacity){      //Check if full buffer
			pthread_cond_wait(&(l->consumer_cv), &(l->lock));       //Wait for car_cross to consume
		}
		struct car *first_car = l->in_cars;     //First car in line
		l->buffer[l->tail] = first_car;     //Add to circular buffer
		l->tail = (l->tail + 1) % l->capacity;      //index of next car in line
		l->in_cars = l->in_cars->next;

		l->in_buf++;
		pthread_cond_signal(&(l->producer_cv));
		pthread_mutex_unlock(&(l->lock));
	}
	pthread_exit(NULL);
}

/**
 *
 * Moves cars from a single lane across the intersection. Cars
 * crossing the intersection must abide the rules of the road
 * and cross along the correct path. Ensure to notify the
 * arrival thread as room becomes available in the lane.
 *
 * Note: After crossing the intersection the car should be added
 * to the out_cars list of the lane that corresponds to the car's
 * out_dir. Do not free the cars!
 *
 * 
 * Note: For testing purposes, each car which gets to cross the 
 * intersection should print the following three numbers on a 
 * new line, separated by spaces:
 *  - the car's 'in' direction, 'out' direction, and id.
 * 
 * You may add other print statements, but in the end, please 
 * make sure to clear any prints other than the one specified above, 
 * before submitting your final code. 
 */
void *car_cross(void *arg) {        //Consumer
	struct lane *l = arg;
	while(l->inc > 0){
		pthread_mutex_lock(&(l->lock));
		while (l->in_buf <= 0){    //If no cars in buffer
			pthread_cond_wait(&(l->producer_cv), &(l->lock));   //Wait for cars to arrive
		}
		struct car *cross_car = l->buffer[l->head];     //Get first car and update next car index
		l->head = (l->head + 1) % l->capacity;

		int i;
		int *quad_locks = compute_path(cross_car->in_dir,cross_car->out_dir);       //Quadrants to be locked
		for (i = 0; i < 4; i++) {
			if (quad_locks[i] != -1){
				pthread_mutex_lock(&isection.quad[quad_locks[i]]);      //Lock traveling quadrants
			};
		}
		struct lane *out_lane;
		out_lane = &isection.lanes[cross_car->out_dir];
		cross_car->next = out_lane->out_cars;  //Entrance to out lane Quadrant locked
		out_lane->out_cars = cross_car;        //so no others can modify out_lane.out_cars
		out_lane->passed++;                    //Or out_lane.passed

		for (i = 0; i < 4; i++) {
			if (quad_locks[i] != -1){
				pthread_mutex_unlock(&isection.quad[quad_locks[i]]);    //Unlock traveled quadrants
			};
		}
		free(quad_locks);

		printf("%d %d %d\n", cross_car->in_dir, cross_car->out_dir, cross_car->id);

		l->inc--;
		l->in_buf--;
		pthread_cond_signal(&(l->consumer_cv));
		pthread_mutex_unlock(&(l->lock));
	}
	pthread_exit(NULL);
}

/**
 *
 * Given a car's in_dir and out_dir return a sorted
 * list of the quadrants the car will pass through.
 *
 */
int *compute_path(enum direction in_dir, enum direction out_dir) {
	int *quadrants = malloc(sizeof(int)*4);
	int i;

	if (in_dir == out_dir){     // U-Turn
		for (i = 0; i < 4; i++) {
			quadrants[i] = i;
		}
		return quadrants;
	}

	for (i = 0; i < 4; i++) {
		quadrants[i] = -1;
	}//invalid quadrants
	int dir_dif = in_dir - out_dir;      //Difference from in_dir to out_dir

	int first_quad = (in_dir + 1)%4;      //First quadrant is closest to in_dir
	quadrants[first_quad] = first_quad;
	//Right turn only goes through first quadrant
	if (abs(dir_dif) == 2){     //Going straight across the intersection
		quadrants[out_dir] = out_dir;
	}else if (dir_dif == -3 || dir_dif == 1){     //Left turn
		if (out_dir == 0){      //Make sure quad[-1] = -1
			quadrants[3] = 3;
		}else{
			quadrants[out_dir - 1] = out_dir - 1;
		}
		quadrants[out_dir] = out_dir;
	}
	return quadrants;   //Return quadrants that need locking in sorted order
}