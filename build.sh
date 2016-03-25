#!/bin/bash

if [ ! -d build ]; then
    mkdir -p build
fi

gcc wii-record.c -o build/wii-record -lxwiimote -std=c99
