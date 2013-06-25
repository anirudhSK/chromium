/*
 * util.cc
 *
 *  Created on: Jun 18, 2013
 *      Author: devasia
 */

#include "util.h"

using namespace std;

void Util::init(){
	numFramesRandomSeek=100;
	loading=false;
	seek=false;

	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
}

void Util::log(string message){
	if(strcmp("Loading", message.c_str())==0){
		previousMessage="Loading";
		clock_gettime(CLOCK_MONOTONIC_RAW, &frame);
		double time=timespecDiff(&frame, &start);
		//TODO: Write to file instead of 'cout'
		cout<<"#"<<message<<" at "<<time<<"\n";
	}

	else if(){

	}

}

double Util::returnFramesToRandomSeek(){
	return numFramesRandomSeek;
}

bool Util::randomSeek(){
	return seek;
}

//This is a private function of the class
double Util::timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p){
	int64_t nano = ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
	double d = static_cast<double>(nano);
	d=d/1000000; //Convert to milliseconds
	return d;
}
