#!/bin/bash

sudo apt update
sudo apt upgrade -y

sudo apt-get install -y libjson-c-dev libcurl4-openssl-dev

read -p "Enter Backend IP: " backend_ip
read -p "Enter Backend Port: " backend_port
read -p "Enter Proxy Port: " proxy_port

# Update config.json with user input
sed -i "s/BACKEND_HERE/$backend_ip/" config.json
sed -i "s/BACKEND_PORT_HERE/$backend_port/" config.json
sed -i "s/PORT_HERE/$proxy_port/" config.json

gcc -o proxy proxy.c -ljson-c -lcurl

echo "Setup was successful! To run, type './proxy config.json' or to screen type 'screen -S proxy_session ./proxy config.json'"
