#!/bin/bash

osd_program=/home/firefly/osd/osd_service/demo_osd
osd_conf_file=/home/firefly/osd/osd_service/osd.conf

gpio_number=56
gpio_threshold_value=0
time_out=10

gpio_path=/sys/class/gpio
gpio_direction=in
gpio_detection_path=${gpio_path}/gpio${gpio_number}/value

osd_program_pid=0

target_clear_time=02:30:00

CLEAR_BACKGROUND_PROGRAM () {
    for (( i=0; i <= 20; i++ ))
    do
        sleep 0.3
        if ps -p $osd_program_pid > /dev/null; then
            kill -n 15 $osd_program_pid
        else
            break
        fi
    done
}

CLEANUP () {
    CLEAR_BACKGROUND_PROGRAM
    exit 0
}

CLEAR_CACHA () {
    echo "-------------------------------------------------------"
    echo "System cache cleared at $(date)"
    sync
    echo 3 > /proc/sys/vm/drop_caches
    echo "-------------------------------------------------------"
}

INIT_GPIO () {
    local number=$1 direction=$2

    if [ ! -d ${gpio_path}/gpio$number ]; then
        echo $number > ${gpio_path}/export
    fi

    echo $direction > ${gpio_path}/gpio${number}/direction
}


INIT_GPIO $gpio_number $gpio_direction

taskset -c 4-7 $osd_program $osd_conf_file &
osd_program_pid=$!

trap 'CLEANUP' SIGTERM

while true; do
    gpio_value=$(cat $gpio_detection_path)
    if [ $gpio_value -eq $gpio_threshold_value ]; then
        start_time=$(date +%s)
        end_time=$start_time
        duration=0

        while [ $(cat $gpio_detection_path) -eq $gpio_threshold_value ]; do
            sleep 0.2
            end_time=$(date +%s)
            ((duration = end_time - start_time))
            if [ $duration -gt $time_out ]; then
                CLEAR_BACKGROUND_PROGRAM
                poweroff
                exit 0
            fi
        done
    fi

    if [[ $(date +%T) == $target_clear_time ]]; then
        CLEAR_CACHA
    fi

    sleep 1
done



