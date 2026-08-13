#!/bin/zsh
cc src/server.c -o server.out \
    -I$(brew --prefix openssl@3)/include \
    -L$(brew --prefix openssl@3)/lib \
    -lssl -lcrypto

cc src/client_backend.c -o client.out \
    -I$(brew --prefix openssl@3)/include \
    -L$(brew --prefix openssl@3)/lib \
    -lssl -lcrypto
