#!/bin/bash

# Linux compilation 
g++ -Wall -w Version_2.cpp gl_frontEnd.cpp utils.cpp -lGL -lglut -lpthread -o version_2
 
# Version 2
#clang++ -Wall -w -Wno-deprecated-declarations -std=c++17 -stdlib=libc++ gl_frontEnd.cpp Version_2.cpp utils.cpp -framework OpenGL -framework GLUT -lpthread -o version_2


