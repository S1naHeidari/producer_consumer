#!/bin/bash
function prodcon(){
	gcc -pthread ./produce-consume.c -o ./produce-consume
	if [ $# -eq 1 ]
  	then
    		./produce-consume $1
	fi
	if [ $# -eq 2 ]
	then
		./produce-consume $1 $2
	fi
}

