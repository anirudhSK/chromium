/*
 * util.cc
 *
 *  Created on: Jun 18, 2013
 *      Author: devasia
 */

#include <string.h>
#include <iostream>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include "util.h"

using namespace std;

//TODO: Make these variables private
struct timespec start, frame;
double numFramesRandomSeek;
bool seek;

string previousMessage;
double previousMessageTime;

double frame_count;

void Util::init(){
	//Init private data members
	numFramesRandomSeek=100;
	seek=false;
	frame_count=0;
	previousMessage="";

	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
}

void Util::log(string message){

	if(strcmp("Loading", message.c_str())==0){

		clock_gettime(CLOCK_MONOTONIC_RAW, &frame);
		double time=timespecDiff(&frame, &start);

		previousMessage="Loading";
		previousMessageTime=time;
	}

	else if((strcmp("Loading", previousMessage.c_str())==0) && (strcmp("FrameReady", message.c_str())==0)){

		clock_gettime(CLOCK_MONOTONIC_RAW, &frame);
		double time=timespecDiff(&frame, &start);

		cout<<"#Loading Time: "<<time-previousMessageTime<<" ms\n";

		previousMessage="FrameReady";
		previousMessageTime=time;
		frame_count++;
	}

	else if((strcmp("FrameReady", previousMessage.c_str())==0) && (strcmp("FrameReady", message.c_str()))==0){
		clock_gettime(CLOCK_MONOTONIC_RAW, &frame);
		double time=timespecDiff(&frame, &start);

		double stall=time-previousMessageTime;

		if(stall>60){
			cout<<"#Stall of "<<stall<<" ms detected between Frame "<<frame_count<<" and "<<frame_count+1<<"\n";
		}

		previousMessage="FrameReady";
		previousMessageTime=time;
		frame_count++;
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
