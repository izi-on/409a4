#!/bin/bash

for i in {1..10}; do
	echo "At pair $i"
	echo "PAIR $i:" >>output.txt
	echo $(
		LC_ALL=C tr -dc '0-9' </dev/urandom | head -c 10000
		echo
	) >choice1.txt
	echo $(
		LC_ALL=C tr -dc '0-9' </dev/urandom | head -c 10000
		echo
	) >choice2.txt
	echo "_____________________" >>output.txt
	echo "Single-threaded:" >>output.txt
	echo "Doing single-threaded"
	total=0
	delimiter=": "
	for i in {1..7}; do
		echo "run: $i"
		printf "RUN $i: " >>output.txt
		output=$(java q1 1 $(cat choice1.txt) $(cat choice2.txt))

		time=$(echo $output | awk -F': ' '{print $2}')

		echo "time taken is ${time}"
		echo "${time}" >>output.txt
		total=$(echo "$total + $time" | bc)
	done
	average=$(echo "$total / 7" | bc -l)
	printf "Average: %0.2f\n" "$average" >>output.txt
	echo "" >>output.txt
	echo "Multi-threaded:" >>output.txt
	echo "Doing multi-threaded"
	total=0
	for i in {1..7}; do
		echo "run: $i"
		printf "RUN $i: " >>output.txt
		output=$(java q1 16 $(cat choice1.txt) $(cat choice2.txt))

		time=$(echo $output | awk -F': ' '{print $2}')

		echo "time taken is ${time}"
		echo "${time}" >>output.txt
		total=$(echo "$total + $time" | bc)
	done
	average=$(echo "$total / 7" | bc -l)
	printf "Average: %0.2f\n" "$average" >>output.txt
	echo "_____________________" >>output.txt
	echo "" >>output.txt
	echo "" >>output.txt
done
