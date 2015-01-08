#!/bin/bash

unzip json-cpp-master.zip
tar -zxvf scons-local-2.0.1.tar.gz -C json-cpp-master/

cd json-cpp-master
python scons.py platform=linux-gcc

