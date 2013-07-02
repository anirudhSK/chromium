#!/bin/sh

#These variables are user-editable

iterations=100
videoPlayTime=20 #in seconds

#These variables are NOT user-editable
i=0

while [ $i -lt $iterations ]; do

	#Delete all Chromium cache files
	sudo rm -rfv ~/.cache/chromium/Default/Media\ Cache/*
	sudo rm -rfv ~/.cache/chromium/Default/Cache/*

	#Delete all Google Chrome cache files - just to be safe
	sudo rm -rfv ~/.cache/google-chrome/Default/Media\ Cache/*
	sudo rm -rfv ~/.cache/google-chrome/Default/Cache/*
	
	#Start bandwidth throttling
	sudo ipfw pipe 2 config bw 8Mbit/s

	#Start Chromium
	~/Desktop/src/out/Release/chrome

	#Wait for video to play a while
        sleep $videoPlayTime
	
	#Change bandwidth throttling
	sudo ipfw pipe 2 config bw 1Mbit/s
	
	#Kill chromium
	sudo killall chrome

	#Increment counter 
	let i=i+1;

done
