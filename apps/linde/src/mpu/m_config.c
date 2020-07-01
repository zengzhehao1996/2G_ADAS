#include <stdio.h>
#include <stdlib.h>
#include "m_config.h"
// Parameter profile 

int static_delayed = 0; // # The delay of the stationary state
int move_delayed = 0;	// # The delay of the motion state

int fork_state = 100;  //The condition of a vehicle.fork_state = 0  # The condition of a vehicle.
// int times_state = 0;  //The condition of a vehicle.fork_state = 0  # The condition of a vehicle.

bool queue_state = false;
int vibra_state = 1; // The state of vibration of a vehicle.


void set_static_delayed(int delayed){
	static_delayed = delayed;
}

int get_static_delayed(){
	return static_delayed;
}

void set_move_delayed(int delayed){
	move_delayed = delayed;
}

int get_move_delayed(){
	return move_delayed;
}

void set_fork_state(int fork_s){
	fork_state = fork_s;
}

int get_fork_state(){
	return fork_state;
}

void set_queue_state(){
	queue_state = true;
}

bool get_queue_state(){
	return queue_state;
}

void set_vibra_state(int aa){
	vibra_state = aa;
}

int get_vibra_state(){
	return vibra_state;
}