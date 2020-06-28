#!/bin/sh

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/lib
chmod 777 dispInit
./dispInit & 
