//
//  Version_4.cpp
//  final_pro
//
//  Created by duhaochi on 12/18/20.
//  Copyright © 2020 duhaochi. All rights reserved.
//

#include <stdio.h>


#include <iostream>
#include <string>
#include <random>
#include <pthread.h>
//
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
//
#include "gl_frontEnd.h"

using namespace std;

//==================================================================================
//    Function prototypes
//==================================================================================
void initializeApplication(void);
GridPosition getNewFreePosition(void);
Direction newDirection(Direction forbiddenDir = NUM_DIRECTIONS);
TravelerSegment newTravelerSegment(const TravelerSegment& currentSeg, bool& canAdd);
void generateWalls(void);
void generatePartitions(void);

//==================================================================================
//    Application-level global variables
//==================================================================================

//    Don't rename any of these variables
//-------------------------------------
//    The state grid and its dimensions (arguments to the program)
SquareType** grid;
unsigned int numRows = 0;    //    height of the grid
unsigned int numCols = 0;    //    width
unsigned int numTravelers = 0;    //    initial number
unsigned int numTravelersDone = 0;
unsigned int numLiveThreads = 0;        //    the number of live traveler threads
unsigned int growSegAfterNumOfMove = 0; // the number of N moves after which a traveler should grow a new segment.
vector<Traveler> travelerList;
vector<SlidingPartition> partitionList;
GridPosition    exitPos;    //    location of the exit

//    travelers' sleep time between moves (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int travelerSleepTime = 100000;

//    An array of C-string where you can store things you want displayed
//    in the state pane to display (for debugging purposes?)
//    Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;
time_t launchTime;

//    Random generators:  For uniform distributions
const unsigned int MAX_NUM_INITIAL_SEGMENTS = 6;
random_device randDev;
default_random_engine engine(randDev());
uniform_int_distribution<unsigned int> unsignedNumberGenerator(0, numeric_limits<unsigned int>::max());
uniform_int_distribution<unsigned int> segmentNumberGenerator(0, MAX_NUM_INITIAL_SEGMENTS);
uniform_int_distribution<unsigned int> segmentDirectionGenerator(0, NUM_DIRECTIONS-1);
uniform_int_distribution<unsigned int> headsOrTails(0, 1);
uniform_int_distribution<unsigned int> rowGenerator;
uniform_int_distribution<unsigned int> colGenerator;

//===============================functions I added==================================
void* create_travelers(void*);
void* player_behaviour(void*);
bool moveTraveler(unsigned int index, Direction dir, bool growTail);
void initialize_travelers();
void initialize_segLocks();
void initialize_gridLocks();
void movePartition(int partition_index);

bool movePartition(int partition_index,GridPosition traveler_current_position);
int search_partition_index(int row, int col);
//==================================================================================

//===============================virables I added==================================
typedef struct ThreadInfo
{
    //    you probably want these
    pthread_t threadID;
    int threadIndex;
    pthread_mutex_t locked;
    //
    //    whatever other input or output data may be needed
    //
    
} ThreadInfo;

pthread_mutex_t LOCK;
// pthread_mutex_t GRIDLOCK[][];

// here is for version5  grid locks
pthread_mutex_t** grid_locks;
//end_______

deque<pthread_mutex_t> segmentLocks;
//indecate which traveler you have control over;
int traveler_control_index = 0;
ThreadInfo* thread_array;

//==================================================================================



//==================================================================================
//    These are the functions that tie the simulation with the rendering.
//    Some parts are "don't touch."  Other parts need your intervention
//    to make sure that access to critical section is properly synchronized
//==================================================================================

void drawTravelers(void)
{
    //-----------------------------
    //    You may have to sychronize things here
    //-----------------------------
    for (unsigned int k=0; k<travelerList.size(); k++)
    {
        //    here I would test if the traveler thread is still live
        if(travelerList[k].segmentList.size() > 0){
            //version4 locks
            pthread_mutex_lock(&segmentLocks[k]);
            drawTraveler(travelerList[k]);
            pthread_mutex_unlock(&segmentLocks[k]);
        }
    }
}

void updateMessages(void)
{
    //    Here I hard-code a few messages that I want to see displayed
    //    in my state pane.  The number of live robot threads will
    //    always get displayed.  No need to pass a message about it.
    unsigned int numMessages = 4;
    sprintf(message[0], "We created %d travelers", numTravelers);
    sprintf(message[1], "%d travelers solved the maze", numTravelersDone);
    sprintf(message[2], "I like cheese");
    sprintf(message[3], "Simulation run time is %ld", time(NULL)-launchTime);
    
    //---------------------------------------------------------
    //    This is the call that makes OpenGL render information
    //    about the state of the simulation.
    //
    //    You *must* synchronize this call.
    //
    //---------------------------------------------------------
    drawMessages(numMessages, message);
}

void handleKeyboardEvent(unsigned char c, int x, int y)
{
    int ok = 0;

    switch (c)
    {
        //    'esc' to quit
        case 27:
            exit(0);
            break;

        //    slowdown
        case ',':
            slowdownTravelers();
            ok = 1;
            break;

        //    speedup
        case '.':
            speedupTravelers();
            ok = 1;
            break;
        
        // controling traveler
        case 'w':
            moveTraveler(traveler_control_index, NORTH, true);
            break;
        case 's':
            moveTraveler(traveler_control_index, SOUTH, true);
            break;
        case 'a':
            moveTraveler(traveler_control_index, WEST, true);
            break;
        case 'd':
            moveTraveler(traveler_control_index, EAST, true);
            break;
        
        // controling traveler index 0-9
        case '0':
            traveler_control_index = 0;
            break;
        case '1':
            traveler_control_index = 1;
            break;
        case '2':
            traveler_control_index = 2;
            break;
        case '3':
            traveler_control_index = 3;
            break;
        case '4':
            traveler_control_index = 4;
            break;
        case '5':
            traveler_control_index = 5;
            break;
        case '6':
            traveler_control_index = 6;
            break;
        case '7':
            traveler_control_index = 7;
            break;
        case '8':
            traveler_control_index = 8;
            break;
        case '9':
            traveler_control_index = 9;
            break;
    
        
            
        default:
            ok = 1;
            break;
    }
    if (!ok)
    {
        //    do something?
    }
}


//------------------------------------------------------------------------
//    You shouldn't have to touch this one.  Definitely if you don't
//    add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupTravelers(void)
{
    //    decrease sleep time by 20%, but don't get too small
    int newSleepTime = (8 * travelerSleepTime) / 10;
    
    if (newSleepTime > MIN_SLEEP_TIME)
    {
        travelerSleepTime = newSleepTime;
    }
}

void slowdownTravelers(void)
{
    //    increase sleep time by 20%
    travelerSleepTime = (12 * travelerSleepTime) / 10;
}

//------------------------------------------------------------------------
//    You shouldn't have to change anything in the main function besides
//    initialization of the various global variables and lists
//------------------------------------------------------------------------
int main(int argc, char** argv){
    //    We know that the arguments  of the program  are going
    //    to be the width (number of columns) and height (number of rows) of the
    //    grid, the number of travelers, etc.
    //    So far, I hard code-some values

    /* Version 1  LARAW */
    // numCols= 11; // == INIT_WIN_X (gl_frontEnd.cpp)
    // numRows = 5; // == INIT_WIN_Y (gl_frontEnd.cpp)
    // numTravelers = 2; // Single-traveler per hand-out
    // growSegAfterNumOfMove = 2; // arbitrary setting. can be set to any num
    // numLiveThreads = numTravelers; // Once they finish, thread--;
    // numTravelersDone = 0;  // why is it ZERO? Nobody finishes yet.
    /* END */



    /* Version 2 LARAW*/

    if(argc != 5){
        printf("Usage: <executable>");
        printf(" <width of the grid>");
        printf(" <height N of the grid>");
        printf(" <number of travelers> ");
        printf(" <number of N moves after which a traver should grow a new segment>\n");
        exit(1);
    }

    numCols= atoi(argv[1]);
    numRows = atoi(argv[2]);
    numTravelers = atoi(argv[3]);
    growSegAfterNumOfMove = atoi(argv[4]);
    numLiveThreads = numTravelers;
    numTravelersDone = 0;

    /* end */
    

    //    Even though we extracted the relevant information from the argument
    //    list, I still need to pass argc and argv to the front-end init
    //    function because that function passes them to glutInit, the required call
    //    to the initialization of the glut library.
    initializeFrontEnd(argc, argv);
    
    //    Now we can do application-level initialization
    initializeApplication();

    launchTime = time(NULL);

    //    Now we enter the main loop of the program and to a large extend
    //    "lose control" over its execution.  The callback functions that
    //    we set up earlier will be called when the corresponding event
    //    occurs
    
    //sleep(1);
    glutMainLoop();
    
    //    Free allocated resource before leaving (not absolutely needed, but
    //    just nicer.  Also, if you crash there, you know something is wrong
    //    in your code.
    for (unsigned int i=0; i< numRows; i++)
        free(grid[i]);
    free(grid);
    for (int k=0; k<MAX_NUM_MESSAGES; k++)
        free(message[k]);
    free(message);
    
    //    This will probably never be executed (the exit point will be in one of the
    //    call back functions).
    return 0;
}


//==================================================================================
//
//    This is a function that you have to edit and add to.
//
//==================================================================================


void initializeApplication(void){
    pthread_mutex_init(&LOCK, NULL);
    //    Initialize some random generators
    rowGenerator = uniform_int_distribution<unsigned int>(0, numRows-1);
    colGenerator = uniform_int_distribution<unsigned int>(0, numCols-1);

    //    Allocate the grid
    grid = new SquareType*[numRows];
    for (unsigned int i=0; i<numRows; i++)
    {
        grid[i] = new SquareType[numCols];
        for (unsigned int j=0; j< numCols; j++)
            grid[i][j] = FREE_SQUARE;
        
    }

    message = new char*[MAX_NUM_MESSAGES];
    for (unsigned int k=0; k<MAX_NUM_MESSAGES; k++)
        message[k] = new char[MAX_LENGTH_MESSAGE+1];
        
    //---------------------------------------------------------------
    //    All the code below to be replaced/removed
    //    I initialize the grid's pixels to have something to look at
    //---------------------------------------------------------------
    //    Yes, I am using the C random generator after ranting in class that the C random
    //    generator was junk.  Here I am not using it to produce "serious" data (as in a
    //    real simulation), only wall/partition location and some color
    srand((unsigned int) time(NULL));

    //    generate a random exit
    for (int i=0; i < 50; i++){
        exitPos = getNewFreePosition();
        grid[exitPos.row][exitPos.col] = EXIT;
    }
    // HARDCODED EXIT
    // GridPosition pos;
    // pos.row = 2;
    // pos.col = 5;
    // exitPos = pos;
    // grid[exitPos.row][exitPos.col] = EXIT;
    // HARDCODED END

    //    Generate walls and partitions
    //_______________________________________in version one here i disable all the wall generation
    generateWalls();
    generatePartitions();
    initialize_segLocks();
	initialize_gridLocks();
	initialize_travelers();

	//    Initialize traveler info structs
    //    You will probably need to replace/complete this as you add thread-related data
    pthread_t travelerThread;
    int error_code = pthread_create(&travelerThread, NULL, create_travelers, NULL);
    if (error_code != 0)
    {
        printf("ERROR: Failed to create create_travelers thread %d\n", error_code);
    }
    //    free array of colors
}

void initialize_segLocks(){
    /*
        Initialize segment locks.
        Purpose:
            Drawing function and travelling use the segments.
            We don't want the draw function until we update the segments.
    */


    for (int i = 0; i < numTravelers; i++){
        pthread_mutex_t lock;
        pthread_mutex_init(&lock, NULL);
        segmentLocks.push_back(lock);
    }
}

void initialize_gridLocks(){
    /*
        Initializing locks for each cell on the grid.
        Purpose:
            To prevent from two entities (traver or partition) going to the same spot at the same time. 

    
        grid = new SquareType*[numRows];
        
        for (unsigned int i=0; i<numRows; i++)
        {
            grid[i] = new SquareType[numCols];
            
            for  (unsigned int j=0; j< numCols; j++)
            grid[i][j] = FREE_SQUARE;
        }

    */

    grid_locks = new pthread_mutex_t*[numRows];
    for (int i = 0; i < numRows; i++){
		grid_locks[i] = new pthread_mutex_t[numCols];
		for (int j = 0; j < numCols; j++)
		{
			pthread_mutex_t lock;
			pthread_mutex_init(&lock, NULL);
			grid_locks[i][j] = lock;
		}
	}
}

void initialize_travelers(){
    for (int i = 0; i < numTravelers; i++){
        Traveler traveler;
        travelerList.push_back(traveler);
        travelerList[i].index = i;
        travelerList[i].travelling = true;
        travelerList[i].move_counter = 0;

        float** travelerColor = createTravelerColors(numTravelers);
    
        TravelerSegment newSeg;
        // HARDCODING START POINT
        GridPosition pos = getNewFreePosition();
        //    Note that treating an enum as a sort of integer is increasingly
        //    frowned upon, as C++ versions progress
        Direction dir = static_cast<Direction>(segmentDirectionGenerator(engine));
        // Direction dir;
        // GridPosition pos;
        // if (index ==0 ){
        //     pos.col = 0;
        //     pos.row = 2;
        //     dir = EAST;
        // }
        // else
        // {
        //     pos.col = 0;
        //     pos.row = 4;
        //     dir = NORTH;
        // }

        TravelerSegment seg = {pos.row, pos.col, dir};
        travelerList[i].segmentList.push_back(seg);

        for (unsigned int c=0; c<4; c++){
            travelerList[i].rgba[c] = travelerColor[i][c];
        }

        grid[pos.row][pos.col] = TRAVELER;
    
    }
}

void* create_travelers(void*){
    //creating all the travlers-
    thread_array = new ThreadInfo[numTravelers];
    //creating all the travlers thread
    for (int i = 0; i < numTravelers; i++) {
        thread_array[i].threadIndex = i;
        int error_code = pthread_create(&thread_array[i].threadID, NULL, player_behaviour, thread_array+i);
        if (error_code != 0)
        {
            printf("ERROR: Failed to create thread %d with error_code=%d\n", i, error_code);
        }
        //make this number bigger if seg falut happens when creating muiltiple traveler
        usleep(100);
    }
    return NULL;
}

void* player_behaviour(void* traveler_index){

    ThreadInfo* traveler_thread = (ThreadInfo*) traveler_index;
    int index = traveler_thread->threadIndex;


    //cout << "Traveler " << traveler_thread->threadIndex << " at (row=" << pos.row << ", col=" <<
    //pos.col << "), direction: " << dirStr(dir) <<  endl;
    //cout << "\t";

    // HARDCODED START
    // vector<Direction> secondThdir = { NORTH, NORTH, NORTH, NORTH, EAST, SOUTH,SOUTH, SOUTH, SOUTH, WEST};
    // int temp=0;
    // HARDCODED END

    bool still_travelling = true;
    //    I add 0-n segments to my travelers
    while (still_travelling){
        TravelerSegment currSeg = travelerList[index].segmentList[0];
        
        // HARDCODED START
        // if(index == 1){
        //     still_travelling = moveTraveler(index, secondThdir.at(temp++), true);
        //     if(temp==10){
        //         temp= 0;
        //     }
        // }else{
        //     still_travelling = moveTraveler(index, dir, true);
        // }
        // HARDCODED END

        // UNCOMMENT THIS ONCE IT'S WORKING
        if (index != traveler_control_index){
            pthread_mutex_lock(&LOCK);
            still_travelling = moveTraveler(index, newDirection(currSeg.dir), true);
            pthread_mutex_unlock(&LOCK);
            usleep(100000);
        }
    }

    numLiveThreads--;
    numTravelersDone++;
    
    //for (unsigned int k=0; k<numTravelers; k++)
    //   delete []travelerColor[k];
    //delete []travelerColor;
    
    return NULL;
}

bool moveTraveler(unsigned int index, Direction dir, bool growTail)
{
    //pervent seg fault
    if(index >= numTravelers){
        printf("player index does not exist\n");
        return false;
    }
    GridPosition travelerPosition;
    travelerPosition.row = travelerList[index].segmentList[0].row;
    travelerPosition.col = travelerList[index].segmentList[0].col;
    /*
        Return True or False:
            True == Traveler is still travelling.
            False == Traveler reached EXIT.
    */
//    cout << "Moving traveler " << index << " at (" <<
//            travelerList[index].segmentList[0].row << ", " <<
//            travelerList[index].segmentList[0].col << ", " <<
//            dirStr(travelerList[index].segmentList[0].dir) << ")" <<
//            " in direction: " << dirStr(dir) << endl;
    // no testing

  
    bool locked = true;
    int lock_row;
    int lock_col;
    if (travelerList[index].travelling){

        switch (dir)
        {

             
            case NORTH: {
                if (travelerList[index].segmentList[0].row > 0 && grid[travelerList[index].segmentList[0].row-1][travelerList[index].segmentList[0].col] == EXIT){
                    travelerList[index].travelling = false;
                    return true;
                }

                // lock[i][j] here&
                lock_row = travelerList[index].segmentList[0].row - 1;
                lock_col = travelerList[index].segmentList[0].col;
                // lock this 
                //      grid[travelerList[index].segmentList[0].row-1][travelerList[index].segmentList[0].col
                if (travelerList[index].segmentList[0].row > 0){
                    pthread_mutex_lock(&grid_locks[lock_row][lock_col]);
                }else{
                    locked = false;
                }
				if (travelerList[index].segmentList[0].row > 0 && (grid[travelerList[index].segmentList[0].row - 1][travelerList[index].segmentList[0].col] == FREE_SQUARE || grid[travelerList[index].segmentList[0].row - 1][travelerList[index].segmentList[0].col] == HORIZONTAL_PARTITION))
				{
					//here I put the version 4 lock
                    if (grid[travelerList[index].segmentList[0].row-1][travelerList[index].segmentList[0].col] == FREE_SQUARE) {
                        TravelerSegment newSeg = {    travelerList[index].segmentList[0].row-1,
                                                    travelerList[index].segmentList[0].col,
                                                    NORTH};
                        //version4 locks
                        pthread_mutex_lock(&segmentLocks[index]);
                        travelerList[index].segmentList.push_front(newSeg);
                        pthread_mutex_unlock(&segmentLocks[index]);
                        //--------------
                        travelerList[index].move_counter += 1;
                        grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                    }else{
                        // if the moving dir is a partition
                        bool moved = movePartition(search_partition_index(travelerList[index].segmentList[0].row-1, travelerList[index].segmentList[0].col), travelerPosition);
                        if(moved){
                            TravelerSegment newSeg = {    travelerList[index].segmentList[0].row-1,
                                                        travelerList[index].segmentList[0].col,
                                                        NORTH};

                            //version4 locks
                            pthread_mutex_lock(&segmentLocks[index]);
                            travelerList[index].segmentList.push_front(newSeg);
                            pthread_mutex_unlock(&segmentLocks[index]);
                            //---------------
                            travelerList[index].move_counter += 1;
                            grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                        }else{
                            growTail = false;
                        }
                    }
				}
				else
				{
					growTail = false;
				}
				//unlock here
                if (travelerList[index].segmentList[0].row >= 0 && locked){
                    pthread_mutex_unlock(&grid_locks[lock_row][lock_col]);
                }
            }
            break;

            case WEST: {
                
                if (travelerList[index].segmentList[0].col > 0 && grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col-1] == EXIT){
                    travelerList[index].travelling = false;
                    return true;
                }
                lock_row = travelerList[index].segmentList[0].row;
                lock_col = travelerList[index].segmentList[0].col-1;

                if (travelerList[index].segmentList[0].col > 0){
                    pthread_mutex_lock(&grid_locks[lock_row][lock_col]);
                }else{
                    locked = false;
                }
                if (travelerList[index].segmentList[0].col > 0 && (grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col-1] == FREE_SQUARE || grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col-1] == VERTICAL_PARTITION)) {
                    if (grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col-1] == FREE_SQUARE) {
                        TravelerSegment newSeg = {    travelerList[index].segmentList[0].row,
                                                    travelerList[index].segmentList[0].col-1,
                                                    WEST};
                        //version4 locks
                        pthread_mutex_lock(&segmentLocks[index]);
                        travelerList[index].segmentList.push_front(newSeg);
                        pthread_mutex_unlock(&segmentLocks[index]);
                        //---------------
                        
                        
                        travelerList[index].move_counter += 1;
                        grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                    }else{
                        bool moved = movePartition(search_partition_index(travelerPosition.row, travelerPosition.col-1), travelerPosition);
                        if(moved){
                            TravelerSegment newSeg = {    travelerList[index].segmentList[0].row,
                                                        travelerList[index].segmentList[0].col-1,
                                                        WEST};
                            //version4 locks
                            pthread_mutex_lock(&segmentLocks[index]);
                            travelerList[index].segmentList.push_front(newSeg);
                            pthread_mutex_unlock(&segmentLocks[index]);
                            //---------------
                            travelerList[index].move_counter += 1;
                            grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                        }else{
                            growTail = false;
                        }
                    }
                    
                }else{
                    growTail = false;
                }

                if (travelerList[index].segmentList[0].col >= 0 && locked){
                    pthread_mutex_unlock(&grid_locks[lock_row][lock_col]);
                }
            }
            break;
    
            case EAST: {
                if(travelerList[index].segmentList[0].col < numCols-1 && grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col+1] == EXIT){
                    travelerList[index].travelling = false;
                    return true;
                }
                lock_row = travelerList[index].segmentList[0].row;
                lock_col = travelerList[index].segmentList[0].col + 1;
                if (travelerList[index].segmentList[0].col < numCols - 1){
                    pthread_mutex_lock(&grid_locks[lock_row][lock_col]);
                }else{
                    locked = false;
                }
                if(travelerList[index].segmentList[0].col < numCols-1 && (grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col+1] == FREE_SQUARE || grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col+1] == VERTICAL_PARTITION)){
                    if(grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col+1] == FREE_SQUARE){
                        TravelerSegment newSeg = {    travelerList[index].segmentList[0].row,
                            travelerList[index].segmentList[0].col+1,
                            EAST};
                        //version4 locks
                        pthread_mutex_lock(&segmentLocks[index]);
                        travelerList[index].segmentList.push_front(newSeg);
                        pthread_mutex_unlock(&segmentLocks[index]);
                        //---------------
                        travelerList[index].move_counter += 1;
                        grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                    }else{
                        bool moved = movePartition(search_partition_index(travelerPosition.row, travelerPosition.col+1), travelerPosition);
                        if(moved){
                            TravelerSegment newSeg = {    travelerList[index].segmentList[0].row,
                                                        travelerList[index].segmentList[0].col+1,
                                                        EAST};
                            //version4 locks
                            pthread_mutex_lock(&segmentLocks[index]);
                            travelerList[index].segmentList.push_front(newSeg);
                            pthread_mutex_unlock(&segmentLocks[index]);
                            //---------------
                            travelerList[index].move_counter += 1;
                            grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                        }else{
                            growTail = false;
                        }
                    }
                    
                }else{
                    growTail = false;
                }
                if (travelerList[index].segmentList[0].col <= numCols - 1 && locked){
                    pthread_mutex_unlock(&grid_locks[lock_row][lock_col]);
                }
            }
            break;
            
            case SOUTH: {
                if (travelerList[index].segmentList[0].row < numRows-1 && grid[travelerList[index].segmentList[0].row+1][travelerList[index].segmentList[0].col] == EXIT){
                    travelerList[index].travelling = false;
                    return true;
                }
                lock_row = travelerList[index].segmentList[0].row + 1;
                lock_col = travelerList[index].segmentList[0].col;
                if (travelerList[index].segmentList[0].row < numRows - 1){
                    pthread_mutex_lock(&grid_locks[lock_row][lock_col]);
                }else{
                    locked = false;
                }
                if (travelerList[index].segmentList[0].row < numRows-1 && (grid[travelerList[index].segmentList[0].row+1][travelerList[index].segmentList[0].col] == FREE_SQUARE ||
                    grid[travelerList[index].segmentList[0].row+1][travelerList[index].segmentList[0].col] == HORIZONTAL_PARTITION)) {
                    
                    if (grid[travelerList[index].segmentList[0].row+1][travelerList[index].segmentList[0].col] == FREE_SQUARE) {
                        TravelerSegment newSeg = {    travelerList[index].segmentList[0].row+1,
                                                    travelerList[index].segmentList[0].col,
                                                    SOUTH};
                        
                        //version4 locks
                        pthread_mutex_lock(&segmentLocks[index]);
                        travelerList[index].segmentList.push_front(newSeg);
                        pthread_mutex_unlock(&segmentLocks[index]);
                        //---------------
                        travelerList[index].move_counter += 1;
                        grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                    }else{
                        bool moved = movePartition(search_partition_index(travelerPosition.row+1, travelerPosition.col), travelerPosition);
                        if(moved){
                            TravelerSegment newSeg = {    travelerList[index].segmentList[0].row+1,
                                                        travelerList[index].segmentList[0].col,
                                                        SOUTH};
                            //version4 locks
                            pthread_mutex_lock(&segmentLocks[index]);
                            travelerList[index].segmentList.push_front(newSeg);
                            pthread_mutex_unlock(&segmentLocks[index]);
                            //---------------
                            
                            travelerList[index].move_counter += 1;
                            grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                        }else{
                            growTail = false;
                        }
                    }
                
                }else{
                    growTail = false;
                }

                if (travelerList[index].segmentList[0].row <= numRows - 1 && locked){
                    pthread_mutex_unlock(&grid_locks[lock_row][lock_col]);
                }
            }
            break;
            default:
                //this need to be changed
                return true;
            break;
        } //switch end
    }// if travelling block
    else{
        if(travelerList[index].segmentList.size() > 0){
            grid[travelerList[index].segmentList.back().row][travelerList[index].segmentList.back().col] = FREE_SQUARE;
            travelerList[index].segmentList.pop_back();
            return true;
        }else{
            return false;
        }

    }

    /**
     *  This code is only reached traveler move to the next dir inside case.
     * @brief if the travler has not reached growSegAfterNumOf Move:
     *              tail should be poped.
     *        growTail is guarding for when traveler cannot move, don't pop seg.
     *        without it, program will keep popping.
     *
     */
    if (travelerList[index].move_counter<growSegAfterNumOfMove && growTail){
        //free square
        grid[travelerList[index].segmentList.back().row][travelerList[index].segmentList.back().col] = FREE_SQUARE;
        travelerList[index].segmentList.pop_back();
    }
    else if(travelerList[index].move_counter == growSegAfterNumOfMove)
    {
        travelerList[index].move_counter = 0;
    }
    return true;
}

//this function finds the index of a partition, given one of it's segments
int search_partition_index(int row, int col){
    //printf("searching index %d\n", partitionList.size());
    bool vertical;
    bool not_found = true;
    int index = 0;
    //see if it is a vertical or horizontal partition to shorten the search time
    if (grid[row][col] == VERTICAL_PARTITION){
        vertical = true;
    }else{
        vertical = false;
    }

    for(int i = 0; i < partitionList.size(); i++){
        int j =0;
        if (vertical && partitionList[i].isVertical && partitionList[i].blockList[0].col == col && not_found) {
            while(j < partitionList[i].blockList.size() && not_found){
                if (partitionList[i].blockList[j].row == row) {
                    not_found = false;
                    index = i;
                }
                j++;
            }
        }else if(!vertical && !partitionList[i].isVertical && partitionList[i].blockList[0].row == row && not_found){
            while(j < partitionList[i].blockList.size() && not_found){
                if (partitionList[i].blockList[j].col == col) {
                    not_found = false;
                    index = i;
                }
                j++;
            }
        }
    }
    // for(int i = 0; i < 5;i++){
    //     printf("row: %d, col: %d\n", partitionList[index].blockList[i].row, partitionList[index].blockList[i].col);
    // }
    // printf("found index at %d\n", index);
    return index;
}

//returns true if the segment is moved so the traveler can move into it,
//returns false if the segment is not moveable so the traveler will seek for new position to move to/

bool movePartition(int partition_index,GridPosition traveler_current_position){
    /*
    
    [
        -->
        0 0 0 0 0 w w
        0 p p p 0 0 1
        0 0 0 0 0 0 0
        0 0 0 0 0 0 0

        0 0 0 0 0 0
        0 0 p p p 0
        0 0 0 0 0 0

    ]
    
    */

    bool movable = true;
    int row = partitionList[partition_index].blockList[0].row;
    int col = partitionList[partition_index].blockList[0].col;
    int length = partitionList[partition_index].blockList.size();
    // if partition is vertical I only have to work with rows here, cols will always be the same
    if(partitionList[partition_index].isVertical){
        //can i move partition up?
        //printf("list size = %d - %d\n",traveler_current_position.row, partitionList[partition_index].blockList[0].row-1);
        int moves_counter = partitionList[partition_index].blockList[length-1].row+1 - traveler_current_position.row;
        if(partitionList[partition_index].blockList[0].row >= moves_counter){
        //for loop (locks)
            for(int i = 1; i <= moves_counter; i++){
                if (grid[partitionList[partition_index].blockList[0].row - i][col] != FREE_SQUARE){
					movable = false;
				}
				//printf("checking row: %d, col:%d\n", partitionList[partition_index].blockList[0].row - i, col);
            }
            //printf("done checking\n");
            //if I can I will
            //printf("list size = %d - %d\n",traveler_current_position.row, partitionList[partition_index].blockList[0].row-1);
            if(movable){
                for(int i = 1; i <= moves_counter; i++){
                    //printf("moving up\n");
                    GridPosition Gpos;
                    int new_row = partitionList[partition_index].blockList[0].row - 1;
                    Gpos.row = new_row;
                    Gpos.col = col;
                    grid[new_row][col] = VERTICAL_PARTITION;
                    grid[partitionList[partition_index].blockList[length-1].row][col] = FREE_SQUARE;
                    
                    partitionList[partition_index].blockList.push_front(Gpos);
                    partitionList[partition_index].blockList.pop_back();
                    //printf("finished moving up%d\n",i);
                }
            }// movable checked END
            // unlock the grid here??
        }// row checking with moves_counter END
        else{
            movable=false;
        }
        //if can't be moved up, then I will check if  I move partition down?
        if(!movable){
            movable = true;
            int moves_counter = traveler_current_position.row+1 - partitionList[partition_index].blockList[0].row;
            if (moves_counter < numRows - partitionList[partition_index].blockList[length-1].row) {
                //lock(grids)
                //printf("list size = %d - %d\n",traveler_current_position.row, partitionList[partition_index].blockList[0].row-1);
                for(int i = 1; i <= moves_counter; i++){
                    //printf("%d\n",moves_counter);
                    if (grid[partitionList[partition_index].blockList[length-1].row + i][col] != FREE_SQUARE){
                        movable = false;
                    }
                    //printf("down -> checking row: %d, col:%d\n", partitionList[partition_index].blockList[length-1].row + i, col);
                }
                //printf("down -> done checking\n");
                if(movable){
                    for(int i = 1; i <= moves_counter; i++){
                        //printf("moving down\n");
                        GridPosition Gpos;
                        int new_row = partitionList[partition_index].blockList[length-1].row + 1;
                        Gpos.row = new_row;
                        Gpos.col = col;
                        
                        partitionList[partition_index].blockList.push_back(Gpos);
                        grid[new_row][col] = VERTICAL_PARTITION;
                        grid[partitionList[partition_index].blockList[0].row][col] = FREE_SQUARE;
                        
                        partitionList[partition_index].blockList.pop_front();
                        //printf("finished moving down\n");
                    }
                }// moable check END
                // UNLOCK GRIND HERE
            }else{
                movable = false;
            }
        }
    // if partition is horizontal
    }else{
        //first can I move left?
        int moves_counter = partitionList[partition_index].blockList[length-1].col+1 - traveler_current_position.col;
        if(partitionList[partition_index].blockList[0].col >= moves_counter){
            //lock here
            for(int i = 1; i <= moves_counter; i++){
                if (grid[partitionList[partition_index].blockList[0].row][col-i] != FREE_SQUARE){
                    movable = false;
                }
                //printf("left -> checking row: %d, col:%d\n", partitionList[partition_index].blockList[0].row, col - i);
            }
            //printf("done checking\n");
            //if I can I will
            //printf("list size = %d - %d\n",traveler_current_position.row, partitionList[partition_index].blockList[0].row-1);
            if(movable){
                for(int i = 1; i <= moves_counter; i++){
                    //printf("moving up\n");
                    GridPosition Gpos;
                    int new_col = partitionList[partition_index].blockList[0].col - 1;
                    Gpos.row = row;
                    Gpos.col = new_col;
                    grid[row][new_col] = HORIZONTAL_PARTITION;
                    grid[row][partitionList[partition_index].blockList[length-1].col] = FREE_SQUARE;
                    
                    partitionList[partition_index].blockList.push_front(Gpos);
                    partitionList[partition_index].blockList.pop_back();
                    //printf("finished moving up%d\n",i);
                }
            }
            //unlock
        }else{
            movable = false;
        }
        //check if i can move right
        if(!movable){
            movable = true;
            int moves_counter = traveler_current_position.col+1 - partitionList[partition_index].blockList[0].col;
            if (moves_counter < numCols - partitionList[partition_index].blockList[length-1].col) {
                //printf("list size = %d - %d\n",traveler_current_position.row, partitionList[partition_index].blockList[0].row-1);
                for(int i = 1; i <= moves_counter; i++){
                    //printf("%d\n",moves_counter);
                    if (grid[row][partitionList[partition_index].blockList[length-1].col + i] != FREE_SQUARE){
                        movable = false;
                        //printf("ops\n");
                    }
                    //printf("right -> checking row: %d, col:%d\n", row, partitionList[partition_index].blockList[length-1].col + i);
                }
                //printf("right -> done checking\n");
                if(movable){
                    for(int i = 1; i <= moves_counter; i++){
                        //printf("moving down\n");
                        GridPosition Gpos;
                        int new_col = partitionList[partition_index].blockList[length-1].col + 1;
                        Gpos.col = new_col;
                        Gpos.row = row;
                        
                        partitionList[partition_index].blockList.push_back(Gpos);
                        grid[row][new_col] = HORIZONTAL_PARTITION;
                        grid[row][partitionList[partition_index].blockList[0].col] = FREE_SQUARE;
                        
                        partitionList[partition_index].blockList.pop_front();
                        //printf("finished moving down\n");
                    }
                }
            }else{
                movable = false;
            }
        }
    }
	//unlock(locks);

	return movable;
}




//------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Generation Helper Functions
#endif
//------------------------------------------------------

GridPosition getNewFreePosition(void){
    GridPosition pos;

    bool noGoodPos = true;
    while (noGoodPos)
    {
        unsigned int row = rowGenerator(engine);
        unsigned int col = colGenerator(engine);
        if (grid[row][col] == FREE_SQUARE)
        {
            pos.row = row;
            pos.col = col;
            noGoodPos = false;
        }
    }
    return pos;
}

Direction newDirection(Direction forbiddenDir){
    switch (forbiddenDir)
    {
        case NORTH:
            forbiddenDir = SOUTH;
            //    no more segment
            break;

        case SOUTH:
            forbiddenDir = NORTH;
            //    no more segment
            break;

        case WEST:
            forbiddenDir = EAST;
            //    no more segment
            break;

        case EAST:
            forbiddenDir = WEST;
            //    no more segment
            break;
        
        default:
            forbiddenDir = NORTH;
    }
    
    bool noDir = true;

    Direction dir = NUM_DIRECTIONS;
    while (noDir)
    {
        dir = static_cast<Direction>(segmentDirectionGenerator(engine));
        noDir = (dir==forbiddenDir);
    }
    return dir;
}


TravelerSegment newTravelerSegment(const TravelerSegment& currentSeg, bool& canAdd)
{
    TravelerSegment newSeg;
    switch (currentSeg.dir)
    {
        case NORTH:
            if (    currentSeg.row < numRows-1 &&
                    grid[currentSeg.row+1][currentSeg.col] == FREE_SQUARE)
            {
                newSeg.row = currentSeg.row+1;
                newSeg.col = currentSeg.col;
                newSeg.dir = newDirection(SOUTH);
                grid[newSeg.row][newSeg.col] = TRAVELER;
                canAdd = true;
            }
            //    no more segment
            else
                canAdd = false;
            break;

        case SOUTH:
            if (    currentSeg.row > 0 &&
                    grid[currentSeg.row-1][currentSeg.col] == FREE_SQUARE)
            {
                newSeg.row = currentSeg.row-1;
                newSeg.col = currentSeg.col;
                newSeg.dir = newDirection(NORTH);
                grid[newSeg.row][newSeg.col] = TRAVELER;
                canAdd = true;
            }
            //    no more segment
            else
                canAdd = false;
            break;

        case WEST:
            if (    currentSeg.col < numCols-1 &&
                    grid[currentSeg.row][currentSeg.col+1] == FREE_SQUARE)
            {
                newSeg.row = currentSeg.row;
                newSeg.col = currentSeg.col+1;
                newSeg.dir = newDirection(EAST);
                grid[newSeg.row][newSeg.col] = TRAVELER;
                canAdd = true;
            }
            //    no more segment
            else
                canAdd = false;
            break;

        case EAST:
            if (    currentSeg.col > 0 &&
                    grid[currentSeg.row][currentSeg.col-1] == FREE_SQUARE)
            {
                newSeg.row = currentSeg.row;
                newSeg.col = currentSeg.col-1;
                newSeg.dir = newDirection(WEST);
                grid[newSeg.row][newSeg.col] = TRAVELER;
                canAdd = true;
            }
            //    no more segment
            else
                canAdd = false;
            break;
        
        default:
            canAdd = false;
    }
    
    return newSeg;
}

void generateWalls(void)
{
    const unsigned int NUM_WALLS = (numCols+numRows)/4;

    //    I decide that a wall length  cannot be less than 3  and not more than
    //    1/4 the grid dimension in its Direction
    const unsigned int MIN_WALL_LENGTH = 3;
    const unsigned int MAX_HORIZ_WALL_LENGTH = numCols / 3;
    const unsigned int MAX_VERT_WALL_LENGTH = numRows / 3;
    const unsigned int MAX_NUM_TRIES = 20;

    bool goodWall = true;
    
    //    Generate the vertical walls
    for (unsigned int w=0; w< NUM_WALLS; w++)
    {
        goodWall = false;
        
        //    Case of a vertical wall
        if (headsOrTails(engine))
        {
            //    I try a few times before giving up
            for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
            {
                //    let's be hopeful
                goodWall = true;
                
                //    select a column index
                unsigned int HSP = numCols/(NUM_WALLS/2+1);
                unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*HSP;
                unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_WALL_LENGTH-MIN_WALL_LENGTH+1);
                
                //    now a random start row
                unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
                for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
                {
                    if (grid[row][col] != FREE_SQUARE)
                        goodWall = false;
                }
                
                //    if the wall first, add it to the grid
                if (goodWall)
                {
                    for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
                    {
                        grid[row][col] = WALL;
                    }
                }
            }
        }
        // case of a horizontal wall
        else
        {
            goodWall = false;
            
            //    I try a few times before giving up
            for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
            {
                //    let's be hopeful
                goodWall = true;
                
                //    select a column index
                unsigned int VSP = numRows/(NUM_WALLS/2+1);
                unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*VSP;
                unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_WALL_LENGTH-MIN_WALL_LENGTH+1);
                
                //    now a random start row
                unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
                for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
                {
                    if (grid[row][col] != FREE_SQUARE)
                        goodWall = false;
                }
                
                //    if the wall first, add it to the grid
                if (goodWall)
                {
                    for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
                    {
                        grid[row][col] = WALL;
                        
                    }
                }
            }
        }
    }
}


void generatePartitions(void)
{
    const unsigned int NUM_PARTS = (numCols+numRows)/4;

    //    I decide that a partition length  cannot be less than 3  and not more than
    //    1/4 the grid dimension in its Direction
    const unsigned int MIN_PARTITION_LENGTH = 3;
    const unsigned int MAX_HORIZ_PART_LENGTH = numCols / 3;
    const unsigned int MAX_VERT_PART_LENGTH = numRows / 3;
    const unsigned int MAX_NUM_TRIES = 20;

    bool goodPart = true;

    for (unsigned int w=0; w< NUM_PARTS; w++)
    {
        goodPart = false;
        
        //    Case of a vertical partition
        if (headsOrTails(engine))
        {
            //    I try a few times before giving up
            for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
            {
                //    let's be hopeful
                goodPart = true;
                
                //    select a column index
                unsigned int HSP = numCols/(NUM_PARTS/2+1);
                unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2-2))*HSP + HSP/2;
                unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_PART_LENGTH-MIN_PARTITION_LENGTH+1);
                
                //    now a random start row
                unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
                for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
                {
                    if (grid[row][col] != FREE_SQUARE)
                        goodPart = false;
                }
                
                //    if the partition is possible,
                if (goodPart)
                {
                    //    add it to the grid and to the partition list
                    SlidingPartition part;
                    part.isVertical = true;
                    for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
                    {
                        grid[row][col] = VERTICAL_PARTITION;
                        GridPosition pos = {row, col};
                        part.blockList.push_back(pos);
                        
                    }
                    partitionList.push_back(part);
                }
                
                
                
            }
        }
        // case of a horizontal partition
        else
        {
            goodPart = false;
            
            //    I try a few times before giving up
            for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
            {
                //    let's be hopeful
                goodPart = true;
                
                //    select a column index
                unsigned int VSP = numRows/(NUM_PARTS/2+1);
                unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2-2))*VSP + VSP/2;
                unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_PART_LENGTH-MIN_PARTITION_LENGTH+1);
                
                //    now a random start row
                unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
                for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
                {
                    if (grid[row][col] != FREE_SQUARE)
                        goodPart = false;
                }
                
                //    if the wall first, add it to the grid and build SlidingPartition object
                if (goodPart)
                {
                    SlidingPartition part;
                    part.isVertical = false;
                    for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
                    {
                        grid[row][col] = HORIZONTAL_PARTITION;
                        GridPosition pos = {row, col};
                        part.blockList.push_back(pos);
                    }
                    partitionList.push_back(part);
                }
            }
        }
    }
}

