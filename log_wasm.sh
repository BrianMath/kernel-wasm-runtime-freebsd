#!/bin/bash

dmesg | grep "kernel" | cut -d':' -f2 | paste -d',' -s 
