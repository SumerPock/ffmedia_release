#!/bin/bash

cp ffmedia_capture.sh /usr/local/bin/
cp libff_media.so /usr/lib/aarch64-linux-gnu/
cp ffmedia-capture.service /etc/systemd/system/
systemctl enable ffmedia-capture.service

