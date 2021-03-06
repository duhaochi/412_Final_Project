//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2020-12-01
//    This is public domain code.  By all means appropriate it and change is to your
//    heart's content.
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
        drawTraveler(travelerList[k]);
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
int main(int argc, char** argv)
{
    //    We know that the arguments  of the program  are going
    //    to be the width (number of columns) and height (number of rows) of the
    //    grid, the number of travelers, etc.
    //    So far, I hard code-some values

    /* Version 1  LARAW */
    numCols= 6; // == INIT_WIN_X (gl_frontEnd.cpp)
    numRows = 2; // == INIT_WIN_Y (gl_frontEnd.cpp)
    numTravelers = 1; // Single-traveler per hand-out
    growSegAfterNumOfMove = 5; // arbitrary setting. can be set to any num
    numLiveThreads = 0; // why is it ZERO?
    numTravelersDone = 0;  // why is it ZERO?
    /* END */ 

    

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


void initializeApplication(void)
{
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
    exitPos = getNewFreePosition();
    grid[exitPos.row][exitPos.col] = EXIT;
    
    //    Generate walls and partitions
    //_______________________________________in version one here i disable all the wall generation
    //generateWalls();
    //generatePartitions();
    
    //    Initialize traveler info structs
    //    You will probably need to replace/complete this as you add thread-related data
    pthread_t pipe_listener;
    int error_code = pthread_create(&pipe_listener, NULL, create_travelers, NULL);
    if (error_code != 0)
    {
        printf("ERROR: Failed to create create_travelers thread %d\n", error_code);
    }
    //    free array of colors
}


void* create_travelers(void*){
    //creating all the travlers
    thread_array = new ThreadInfo[numTravelers];
    //creating all the travlers thread
    for (int i = 0; i < numTravelers; i++) {
        printf("0ops___________\n");
        thread_array[i].threadIndex = i;
        int error_code = pthread_create(&thread_array[i].threadID, NULL, player_behaviour, thread_array+i);
        if (error_code != 0)
        {
            printf("ERROR: Failed to create thread %d with error_code=%d\n", i, error_code);
        }
        //make this number bigger if seg falut happens when creating muiltiple traveler
        sleep(1);
    }
    
    return NULL;
}

void* player_behaviour(void* traveler_index){
    Traveler traveler;
    travelerList.push_back(traveler);
    
    TravelerSegment newSeg;
    
    ThreadInfo* traveler_thread = (ThreadInfo*) traveler_index;
    int index = traveler_thread->threadIndex;
    travelerList[index].index = index;
    travelerList[index].travelling = true;
    float** travelerColor = createTravelerColors(numTravelers);
    GridPosition pos = getNewFreePosition();
    //    Note that treating an enum as a sort of integer is increasingly
    //    frowned upon, as C++ versions progress
    Direction dir = static_cast<Direction>(segmentDirectionGenerator(engine));
    TravelerSegment seg = {pos.row, pos.col, dir};
    travelerList[index].segmentList.push_back(seg);
    grid[pos.row][pos.col] = TRAVELER;
    cout << "Traveler " << traveler_thread->threadIndex << " at (row=" << pos.row << ", col=" <<
    pos.col << "), direction: " << dirStr(dir) <<  endl;
    cout << "\t";
    for (unsigned int c=0; c<4; c++){
        travelerList[index].rgba[c] = travelerColor[travelerList[traveler_thread->threadIndex].index][c];
    }

    bool still_travelling = true;
    //    I add 0-n segments to my travelers
    while (still_travelling){
        TravelerSegment currSeg = travelerList[index].segmentList[0];
        dir = static_cast<Direction>(segmentDirectionGenerator(engine));
        still_travelling = moveTraveler(index, newDirection(currSeg.dir), true);
        // if(!still_travelling){
        //     while(travelerList[index].segmentList.size() > 1){
        //         travelerList[index].segmentList.pop_back();
        //         sleep(1);
        //     }
        //     travelerList.erase(travelerList.begin()+index);
        // }

        sleep(1);
    }
    
    // Threads [5 ] or [4];
    // travelerList [4];
    for (unsigned int k=0; k<numTravelers; k++)
        delete []travelerColor[k];
    delete []travelerColor;
    
    return NULL;
}

bool moveTraveler(unsigned int index, Direction dir, bool growTail)
{
    /*
        Return True or False:
            True == Traveler is still travelling.
            False == Traveler reached EXIT.
    */
    cout << "Moving traveler " << index << " at (" <<
            travelerList[index].segmentList[0].row << ", " <<
            travelerList[index].segmentList[0].col << ", " <<
            dirStr(travelerList[index].segmentList[0].dir) << ")" <<
            " in direction: " << dirStr(dir) << endl;
    Direction forbiden_dir = NORTH;
    // no testing

    if (travelerList[index].travelling){

        switch (dir)
        {
            case NORTH: {
                if (travelerList[index].segmentList[0].row > 0 && grid[travelerList[index].segmentList[0].row-1][travelerList[index].segmentList[0].col] == EXIT){
                    //travelerList.erase(travelerList.begin()+index);
                    travelerList[index].travelling = false;
                    return true;
                }

                if (travelerList[index].segmentList[0].row > 0 && grid[travelerList[index].segmentList[0].row-1][travelerList[index].segmentList[0].col] == FREE_SQUARE)
                {
                    TravelerSegment newSeg = {    travelerList[index].segmentList[0].row-1,
                                                travelerList[index].segmentList[0].col,
                                                NORTH};
                    travelerList[index].segmentList.push_front(newSeg);
                    grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                }else{
                    forbiden_dir = SOUTH;
                }
            }
            break;

            case WEST: {
                if (travelerList[index].segmentList[0].col > 0 && grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col-1] == EXIT){
                    //travelerList.erase(travelerList.begin()+index);
                    travelerList[index].travelling = false;
                    return true;
                }
                if (travelerList[index].segmentList[0].col > 0 && grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col-1] == FREE_SQUARE) {
                    TravelerSegment newSeg = {    travelerList[index].segmentList[0].row,
                                                travelerList[index].segmentList[0].col-1,
                                                WEST};
                    travelerList[index].segmentList.push_front(newSeg);
                    grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                }else{
                    forbiden_dir = EAST;
                }
            }
            break;
            case EAST: {
                if(travelerList[index].segmentList[0].col < numCols-1 && grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col+1] == EXIT){
                    //travelerList.erase(travelerList.begin()+index);
                    travelerList[index].travelling = false;
                    return true;
                }
                if(travelerList[index].segmentList[0].col < numCols-1 && grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col+1] == FREE_SQUARE){
                    TravelerSegment newSeg = {    travelerList[index].segmentList[0].row,
                                                travelerList[index].segmentList[0].col+1,
                                                EAST};
                    travelerList[index].segmentList.push_front(newSeg);
                    grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                }else{
                    forbiden_dir = WEST;
                }
            }
            break;
            
            case SOUTH: {
                if (travelerList[index].segmentList[0].row < numRows-1 && grid[travelerList[index].segmentList[0].row+1][travelerList[index].segmentList[0].col] == EXIT){
                    //travelerList.erase(travelerList.begin()+index);
                    travelerList[index].travelling = false;
                    return true;
                } 
                if (travelerList[index].segmentList[0].row < numRows-1 && grid[travelerList[index].segmentList[0].row+1][travelerList[index].segmentList[0].col] == FREE_SQUARE) {
                    TravelerSegment newSeg = {    travelerList[index].segmentList[0].row+1,
                                                travelerList[index].segmentList[0].col,
                                                SOUTH};
                    travelerList[index].segmentList.push_front(newSeg);
                    grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = TRAVELER;
                }else{
                    forbiden_dir = NORTH;
                }
            }
            default:
                //this need to be changed
                return true;
            break;
        }
    }// if travelling block
    else{
        if(travelerList[index].segmentList.size() > 1){
            travelerList[index].segmentList.pop_back();
            grid[travelerList[index].segmentList[0].row][travelerList[index].segmentList[0].col] = FREE_SQUARE;
            //sleep(1);
            return true;
        }else{
            travelerList.erase(travelerList.begin()+index);
            return false;
        }   

    }

    
    if (!growTail){
        travelerList[index].segmentList.pop_back();
    }

    return true;
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
                }
            }
        }
    }
}

