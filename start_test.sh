#!/bin/sh

#These variables are user-editable

iterations=100
videoPlayTime=120 #in seconds

#These variables are NOT user-editable
i=0

while [ $i -lt $iterations ]; do

	#Delete all Chromium cache files
	sudo rm -rfv ~/.cache/chromium/Default/Media\ Cache/*
	sudo rm -rfv ~/.cache/chromium/Default/Cache/*

	#Delete all Google Chrome cache files - just to be safe
	sudo rm -rfv ~/.cache/google-chrome/Default/Media\ Cache/*
	sudo rm -rfv ~/.cache/google-chrome/Default/Cache/*
	
	#Start bandwidth throttling and start Chromium
~/Desktop/src/out/Release/chrome http://10.0.0.2/start_sita.html | perl /home/devasia/Desktop/src/process.pl &

	#Wait for video to play a while
	sleep $videoPlayTime
	#Kill chromium
	sudo killall chrome

	#Kill bandwidth throttling process
	sudo killall trickle
	sudo killall trickled

	#Increment counter 
	let i=i+1;

done
