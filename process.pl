#!/usr/bin/perl -w

open(DECODED, '>/home/devasia/Desktop/decoded.txt');
$|=1;
open(BUFFERED, '>/home/devasia/Desktop/buffered.txt');
$|=1;

while($line=<STDIN>){
 @sp=split(" ", $line);
 
if($sp[0] eq "#Decoded"){
  #print $sp[3], " ", $sp[1], "\n";
  print DECODED $sp[3], " ", $sp[1], "\n";
  DECODED->autoflush(1);
 }

elsif($sp[0] eq "#Buffered"){
  #print $sp[3], " ", $sp[1], "\n";
  print BUFFERED $sp[3], " ", $sp[1], "\n";
  BUFFERED->autoflush(1);
 }

else{
  print $line;
 }
}

close(DECODED);
close(BUFFERED);
