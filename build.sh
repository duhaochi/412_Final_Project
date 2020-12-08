#!/bin/bash

# Linux compilation 
g++ -Wall -w main.cpp gl_frontEnd.cpp utils.cpp -lGL -lglut -lpthread -o main
 
# Mac OS compilation
#clang++ -Wall -Wno-deprecated-declarations -std=c++17 -stdlib=libc++ gl_frontEnd.cpp main.cpp utils.cpp -framework OpenGL -framework GLUT -lpthread -o traveler
