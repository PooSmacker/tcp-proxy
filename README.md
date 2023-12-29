
# Tcp Proxy

Proxy your backend and protect it from DDoS attacks with this script.


## Installation

Just copy and paste each step below.

```bash
1.  sudo apt update
2.  sudo apt upgrade
3.  sudo apt-get install libjson-c-dev libcurl4-openssl-dev
4.  git clone https://github.com/PooSmacker/tcp-proxy.git
5.  cd tcp-proxy
6.  gcc -o proxy proxy.c -ljson-c -lcurl
```


    
## Usage

How to use:

```bash
1.  edit your config to include the port you would like to listen on, your backend ip and your backend port.
2.  to run type ./proxy config.json

```

