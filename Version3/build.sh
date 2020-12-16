#!/bin/bash


#g++ -Wall -w main.cpp gl_frontEnd.cpp utils.cpp -lGL -lglut -lpthread -o main
#g++ -Wall -w version_1.cpp gl_frontEnd.cpp utils.cpp -lGL -lglut -lpthread -o version_1

# Linux compilation 
#g++ -Wall -w main.cpp gl_frontEnd.cpp utils.cpp -lGL -lglut -lpthread -o main
 
# Mac OS compilation
#clang++ -Wall -Wno-deprecated-declarations -std=c++17 -stdlib=libc++ gl_frontEnd.cpp main.cpp utils.cpp -framework OpenGL -framework GLUT -lpthread -o traveler
clang++ -Wall -w -Wno-deprecated-declarations -std=c++17 -stdlib=libc++ gl_frontEnd.cpp Version_3.cpp utils.cpp -framework OpenGL -framework GLUT -lpthread -o version_3
#clang++ -Wall -w -Wno-deprecated-declarations -std=c++17 -stdlib=libc++ gl_frontEnd.cpp Version_2.cpp utils.cpp -framework OpenGL -framework GLUT -lpthread -o version_2

