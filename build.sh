#!/bin/bash

g++ -Wall -w main.cpp gl_frontEnd.cpp utils.cpp -lGL -lglut -lpthread -o main
g++ -Wall -w version_1.cpp gl_frontEnd.cpp utils.cpp -lGL -lglut -lpthread -o version_1
