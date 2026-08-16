#!/bin/zsh
cc $1.c src/storage.c -o $1.out \
    -I$(brew --prefix openssl@3)/include \
    -L$(brew --prefix openssl@3)/lib \
    -lssl -lcrypto

