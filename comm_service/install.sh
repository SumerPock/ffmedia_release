#!/bin/bash

cp firefly_communication.sh /usr/local/bin/
cp firefly-communication.service /etc/systemd/system/
systemctl enable firefly-communication.service

