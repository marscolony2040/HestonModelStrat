#!/bin/bash
echo "Compiling System"
g++ trader.cpp -std=c++11 -lcurl -lpthread -lcpprest -lcrypto -lssl
echo "System Compiled"
exit 0
