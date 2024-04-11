#!/bin/bash

n=300000
output_file="run_averages.txt"

# Clear the output file or create it if it doesn't exist
: >"$output_file"

human_output="THIS FILE IS AUTO-GENERATED. EACH VALUE IS AN AVERAGE COMPUTED OVER 10 ITERATIONS"
echo "$human_output" >>"$output_file"

# Loop over i from 0 to 4
for i in {0..4}; do
	total_time=0

	# Perform 10 runs for each i
	for run in {1..10}; do
		output=$(./q2 $i $n)                 # Run the program
		time=$(echo "$output" | sed -n '3p') # Extract the time taken

		# Sum up the matches and time
		total_time=$(echo "$total_time + $time" | bc)
	done

	# Calculate averages
	avg_time=$(echo "scale=5; $total_time / 10" | bc)

	# Write the averages to the output file
	echo "i=$i: Average Time = $avg_time sec" >>"$output_file"
done

echo "Averages computed and stored in $output_file"
