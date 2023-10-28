#!/bin/bash

# Define the serial port
SERIAL_PORT="/dev/ttyACM0"

# Set baud rate (make sure it matches the Arduino sketch)
BAUD_RATE="9600"

# Open the serial port and use file descriptor 3
exec 3<>"$SERIAL_PORT"

# Configure serial port settings
stty -F "$SERIAL_PORT" cs8 -cstopb -parenb $BAUD_RATE

# Variable to store the previous calculated level
prev_level=0

# Variable to store the last used level
last_used_level=0

# Read and process data from the Arduino
while true; do
    # Read one line from the serial port
    input_line=$(head -n 1 <&3)

    # Extract numerical value from the input line
    value=$(echo "$input_line" | grep -oE '[0-9]+([.][0-9]+)?')

    # Check if the value is non-empty and numeric
    if [[ -n "$value" && "$value" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
        # Calculate level (value divided by 8) and round to nearest integer
        level=$(printf "%.0f" $(echo "scale=2; $value / 8" | bc))

        # Ensure the level is within the valid range (0 to 100)
        if [ "$level" -gt 100 ]; then
            level=100
        fi

        # Compare with the previous level, execute ddcutil if the difference is more than 4
        if [ $((level - prev_level)) -gt 4 ] || [ $((prev_level - level)) -gt 4 ]; then
            # Run ddcutil command with the calculated level
            ddcutil --display 1 setvcp 10 "$level"

            # Update the last used level with the current level
            last_used_level=$level

            # Update the previous level with the current level
            prev_level=$level

            # Print the received data, the calculated level, and the last used level
            echo "Received data: $input_line, Calculated Level: $level, Last Used Level: $last_used_level"
        else
            # Print a message indicating that the level did not change significantly
            echo "Level did not change significantly. Skipping ddcutil command. Last Used Level: $last_used_level"
        fi
    else
        # Print an error message if the value is not numeric
        echo "Error: Invalid data received - $input_line"
    fi

    # Introduce a delay of 1 second (slows down the script)
    sleep 1
done

# Close the serial port (This will never be reached in this script due to the infinite loop)
exec 3>&-
