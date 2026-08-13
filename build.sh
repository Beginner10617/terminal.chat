#!/bin/zsh
cc server.c -o server.out \
    -I$(brew --prefix openssl@3)/include \
    -L$(brew --prefix openssl@3)/lib \
    -lssl -lcrypto

cc client_backend.c -o client.out \
    -I$(brew --prefix openssl@3)/include \
    -L$(brew --prefix openssl@3)/lib \
    -lssl -lcrypto
