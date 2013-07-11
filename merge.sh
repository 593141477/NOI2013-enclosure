#!/bin/bash

>output.cpp
for i in src/*.h 
do
	cat "$i" >>output.cpp
	echo "" >>output.cpp
done
for i in src/*.cpp 
do
	cat "$i" >>output.cpp
	echo "" >>output.cpp
done