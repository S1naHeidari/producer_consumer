#!/bin/bash
all:
	gcc -pthread -O ./produce-consume.c -o produce-consume
debug:
	gcc -pthread ./produce-consume.c -g produce-consume
