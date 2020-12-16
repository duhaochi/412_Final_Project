#!/bin/bash


#g++ -Wall -w main.cpp gl_frontEnd.cpp utils.cpp -lGL -lglut -lpthread -o main
g++ -Wall -w version_2.cpp gl_frontEnd.cpp utils.cpp -lGL -lglut -lpthread -o version_2

# Linux compilation 
#g++ -Wall -w main.cpp gl_frontEnd.cpp utils.cpp -lGL -lglut -lpthread -o main
 
# Mac OS compilation
#clang++ -Wall -Wno-deprecated-declarations -std=c++17 -stdlib=libc++ gl_frontEnd.cpp main.cpp utils.cpp -framework OpenGL -framework GLUT -lpthread -o traveler
#<<<<<<< HEAD
#clang++ -Wall -w -Wno-deprecated-declarations -std=c++17 -stdlib=libc++ gl_frontEnd.cpp Version_1.cpp utils.cpp -framework OpenGL -framework GLUT -lpthread -o version_1
#clang++ -Wall -w -Wno-deprecated-declarations -std=c++17 -stdlib=libc++ gl_frontEnd.cpp Version_2.cpp utils.cpp -framework OpenGL -framework GLUT -lpthread -o version_2
#=======

# Version 1
#clang++ -Wall -w -Wno-deprecated-declarations -std=c++17 -stdlib=libc++ gl_frontEnd.cpp Version_1.cpp utils.cpp -framework OpenGL -framework GLUT -lpthread -o version_1

# Version 2
clang++ -Wall -w -Wno-deprecated-declarations -std=c++17 -stdlib=libc++ gl_frontEnd.cpp Version_2.cpp utils.cpp -framework OpenGL -framework GLUT -lpthread -o version_2

#>>>>>>> daba94a8982560df95ce3ff19785f8aff7bf2ca5

