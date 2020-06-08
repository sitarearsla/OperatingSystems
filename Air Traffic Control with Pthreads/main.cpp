#include <iostream>
#include <queue>
#include <fstream>
#include "pthread_sleep.c"
#include <ctime>
#include <pthread.h>
#include <vector>
#include <deque>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>

#define NUM_THREADS 3 //Takeoff, Landing, Tower

using namespace std;

struct realTime{ //real time for planes
    int hour;
    int minute;
    int second;
};

struct Plane { //plane specifications
    int id;
    char direction;
    realTime requestTime;
    realTime runwayTime;
    pthread_cond_t cond;
    pthread_mutex_t lock;
};

struct planeEvent{ //the traffic events
    realTime eventTime;
    deque<Plane> waitingPlanesOnAir;
    queue<Plane> waitingPlanesOnGround;
};

static int threadInitialLanding = 0;
static int threadInitialDeparting = 1;
static int threadControlTower = 2;

double defaultProb = 0.5; //default probability
int simulationTime = 60; //default simulation time
int landingIdCounter = 304; //even id for landing
int departingIdCounter = 1923; // odd id for departing

long numLanding = 0; //number of planes in landing queue
long numDeparting = 0; //number of planes in departing queue
int starvationFlag = 0; //starvation Check Flag
int emergencyFlag = 0; //emergency Check Flag

pthread_mutex_t runwayLock = PTHREAD_MUTEX_INITIALIZER; //lock for the runway
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER; //lock for the queue
pthread_cond_t runwayCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t idLock = PTHREAD_MUTEX_INITIALIZER;

realTime startTime; //simulation begins
realTime finishTime; //simulation ends

deque<Plane> landingQueue; // queue for planes that will land
queue<Plane> departingQueue; // queue for planes that will depart
queue<Plane> finishedQueue; // queue for planes that have landed or departed
queue<planeEvent> eventQueue; //queue of events

bool simulationContinues(tm* now, realTime inputTime);
realTime fixTime(realTime &inputTime);
void calculateSimulationTime();
void *departing_func(void *);
void *landing_func(void *);
void *controlTower(void *);
void logPlane();
void logRunway();
planeEvent createEvent(realTime now);

/* simulationContinues --> checks if the simulation can continue
 * INPUT current time pointer and finish time, RETURNS bool
 */
bool simulationContinues(tm* now, realTime inputTime){
    if(now->tm_hour > inputTime.hour){
        return false;
    } else if(now->tm_hour < inputTime.hour) {
        return true;
    } else{
        if(now->tm_min > inputTime.minute){
            return false;
        } else if(now->tm_min < inputTime.minute){
            return true;
        } else{
            if(now->tm_sec > inputTime.second){
                return false;
            } else if(now->tm_sec < inputTime.second){
                return true;
            }
        }
    }
    return false;
}

/* fixTime --> adjusts the time after using the runway
 * INPUT realtime, RETURNS realtime
 */
realTime fixTime(realTime &inputTime){
    while(inputTime.second > 59 || inputTime.minute > 59 || inputTime.hour > 23){
        if(inputTime.second > 59){
            inputTime.second -= 60;
            inputTime.minute++;
        } else if(inputTime.minute > 59){
            inputTime.minute -= 60;
            inputTime.hour++;
        } else {
            inputTime.hour -= 24;
        }
    }
    return inputTime;
}


//calculateSimulationTime --> calculates the simulation end time
void calculateSimulationTime(){

    time_t t = std::time(0);   // get time now
    tm* now = std::localtime(&t);

    cout << "Simulation Start Time => "
         << now->tm_hour<< ':'
         << (now->tm_min) << ':'
         << now->tm_sec
         << "\n";

    startTime.hour = now->tm_hour;
    startTime.minute = now->tm_min;
    startTime.second = now->tm_sec;

    finishTime.second = startTime.second + simulationTime;
    finishTime.minute = startTime.minute;
    finishTime.hour = startTime.hour;

    realTime adjustedFinish = fixTime(finishTime);

    cout << "Simulation Finish time => "
         << adjustedFinish.hour << ':'
         << adjustedFinish.minute << ':'
         << adjustedFinish.second
         << "\n";

    cout << "--------------------------------------------" << endl;
}

/* createEvent --> creates events to add to the runway log file
 * INPUT real time of the event, RETURNS planeEvent
 */
planeEvent createEvent(realTime now){
    planeEvent tempEvent;
    tempEvent.eventTime.hour = now.hour;
    tempEvent.eventTime.minute = now.minute;
    tempEvent.eventTime.second = now.second;
    tempEvent.waitingPlanesOnAir = landingQueue;
    tempEvent.waitingPlanesOnGround = departingQueue;
    return tempEvent;
}

int main(int argc, char* argv[]) {
    //parse command line arguments
    if(argc == 2){
        defaultProb = atof(argv[1]); //gets user given probability
        cout <<  "No input for simulation time" << endl << "Default 60 seconds" << endl;
        cout << "--------------------------------------------" << endl;
    } else if(argc == 4){
        defaultProb = atof(argv[1]); //gets user given probability
        string flag(argv[2]); //gets user given flag
        if(flag == "-s"){
            simulationTime = atoi(argv[3]); //gets user given simulation time
        } else{
            cout << "INVALID INPUT" << endl;
            return 0;
        }
    } else if(argc == 1){
        cout <<  "No input for train creation probability and simulation time" << endl
             << "Simulation performed for 60 seconds with probability 0.5" << endl;
        cout << "--------------------------------------------" << endl;
    } else{
        cout << "INVALID INPUT" << endl;
        return 0;
    }

    pthread_t threads[NUM_THREADS];
    calculateSimulationTime(); //marks start and finish time of the simulation
    int rc;
    int i;
    for(i = 0; i < NUM_THREADS; i++ ) {
        if(i == threadControlTower) {  //creates ATC thread
            rc = pthread_create(&threads[i], NULL, controlTower, (void *) i);
            cout << "Creating the control tower..." << endl;
        } else if (i == threadInitialDeparting) { //creates the initial departing plane thread
            rc = pthread_create(&threads[i], NULL, departing_func, (void *) i);
            cout << "Creating the initial departing plane..." << endl;
        }
        else if (i == threadInitialLanding) { //creates the initial landing plane thread
            rc = pthread_create(&threads[i], NULL, landing_func, (void *) i);
            cout << "Creating the initial landing plane..." << endl;
        }

        if (rc) { //if the thread failed to be created, gives error
            cout << "Error:unable to create thread," << rc << endl;
            exit(-1);
        }
    }


    cout << "--------------------------------------------" << endl;
    cout << "  ---    ---    RUNWAY READY  ---    ---    " << endl;
    cout << "--------------------------------------------" << endl;

    time_t t = std::time(0);   // get time now
    tm* now = std::localtime(&t);
    pthread_t cThread;

    while(simulationContinues(now, finishTime)) { //works till the end of the simulation time
        int tempStart = startTime.second + startTime.minute * 60 + startTime.hour * 24 * 60;
        int tempTotal = now->tm_sec + now->tm_min * 60 + now->tm_hour * 24 * 60;
        int tempBound = tempTotal + 1;

        double probGenerated = ((double) rand() / (RAND_MAX));
        if (probGenerated <= defaultProb || (tempStart - tempTotal) % 40 == 0) {
            //every 40 second an emergency plane is created
            if((tempStart - tempTotal) % 40 == 0 && tempStart != tempTotal){
                int index = 1; //emergency
                rc = pthread_create(&cThread, NULL, landing_func, (void *)(index));
            } else {
                int index = 0;
                rc = pthread_create(&cThread, NULL, landing_func, (void *)(index));
            }
        } else {
            int *index = (int*) malloc(sizeof(int));
            rc = pthread_create(&cThread, NULL, departing_func, (void *)(index));
        }
        if (rc) { //if the thread failed to be created, gives error
            cout << "Error:unable to create thread," << rc << endl;
            exit(-1);
        }

        while(tempTotal < tempBound){ //works every second
            t = std::time(0);
            now = std::localtime(&t);
            tempTotal = now->tm_sec + now->tm_min*60 + now->tm_hour * 24 * 60;
        }
        pthread_sleep(1);
    }

    cout << "     Details can be found in log files      " << endl;
    cout << "--------------------------------------------" << endl;
    cout << "        Simulation ended Succesfully!       " << endl;
    cout << "--------------------------------------------" << endl;

    pthread_mutex_destroy(&idLock);
    pthread_mutex_destroy(&queueLock);
    pthread_mutex_destroy(&runwayLock);
    pthread_cond_destroy(&runwayCond);
    logPlane();
    logRunway();
    return 0;
}

//logPlane --> the landed or departed planes are reported in the plane.log output file.
void logPlane(){
    ofstream planeLog;
    planeLog.open("plane.log");
    planeLog << "------------------------------------------------------------------------        \n";
    planeLog << "|                     Welcome to the Plane Log!                        |        \n";
    planeLog << "------------------------------------------------------------------------        \n";
    planeLog << " PlaneID     Status     Request Time    Runway Time     Turnaround Time         \n"
             << " -------     ------     ------------    -----------     ---------------         \n";
    while(!(finishedQueue.empty())){
        Plane finishedPlane = finishedQueue.front();
        finishedQueue.pop();
        char direction;

        planeLog << "   " << finishedPlane.id << "\t\t"<< finishedPlane.direction << "\t"
                 << finishedPlane.requestTime.hour << ":" ;
        if(finishedPlane.requestTime.minute >= 10){
            planeLog << finishedPlane.requestTime.minute  << ":" ;
        } else {
            planeLog << "0" << finishedPlane.requestTime.minute  << ":" ;
        }
        if(finishedPlane.requestTime.second >= 10){
            planeLog << finishedPlane.requestTime.second;
        } else {
            planeLog << "0" << finishedPlane.requestTime.second ;
        }
        realTime ft = {finishedPlane.runwayTime.hour, finishedPlane.runwayTime.minute, finishedPlane.runwayTime.second + 2};
        realTime adjustedFinishTime = fixTime(ft);
        planeLog << " \t " << adjustedFinishTime.hour << ":" ;
        if(adjustedFinishTime.minute >= 10){
            planeLog << adjustedFinishTime.minute << ":";
        } else {
            planeLog << "0" << adjustedFinishTime.minute << ":";
        }
        if(adjustedFinishTime.second >= 10){
            planeLog << adjustedFinishTime.second;
        } else {
            planeLog << "0" << adjustedFinishTime.second ;
        }

        int requestTimeinSec = finishedPlane.requestTime.hour*24*60 + finishedPlane.requestTime.minute*60 + finishedPlane.requestTime.second;
        int finishTimeinSec = adjustedFinishTime.hour*24*60 + adjustedFinishTime.minute*60 + adjustedFinishTime.second;
        int turnaroundTime = finishTimeinSec - requestTimeinSec;
        planeLog << "       " << turnaroundTime << " secs \n" ;

    }
    planeLog.close();
}

//logRunway --> the flow of events are reported in the Runway-Records.log output file
void logRunway(){
    ofstream runwayLog;
    runwayLog.open ("Runway-Records.log");
    runwayLog << "------------------------------------------------------------------------       \n";
    runwayLog << "|                     Welcome to the Runway Log!                       |       \n";
    runwayLog << "------------------------------------------------------------------------       \n";
    runwayLog << "|          Initial 2 records show the runway getting ready ...         |       \n";
    runwayLog << "------------------------------------------------------------------------       \n";

 	for (int i = 0; i < eventQueue.size(); i++) { 
        planeEvent tempEvent = eventQueue.front();
        eventQueue.pop();
        runwayLog << "At time ";
        runwayLog << tempEvent.eventTime.hour << ":";
         if (tempEvent.eventTime.minute >= 10) {
             runwayLog << tempEvent.eventTime.minute<< ":";
         } else {
             runwayLog << "0" << tempEvent.eventTime.minute << ":";
         }
        if (tempEvent.eventTime.second >= 10) {
            runwayLog << tempEvent.eventTime.second << "\n";
        } else {
            runwayLog << "0" << tempEvent.eventTime.second << "\n";
        }
        runwayLog << "Waiting on Air:" ;
        vector<int> planeIDs;
        for (int i = 0; i < tempEvent.waitingPlanesOnAir.size(); i++) {
            Plane tempPlane = tempEvent.waitingPlanesOnAir.front();
            tempEvent.waitingPlanesOnAir.pop_front();
            planeIDs.push_back(tempPlane.id);
        }
        if (planeIDs.size() == 0) {
            runwayLog << "\n";
        } else {
            for (int i = 0; i < planeIDs.size(); i++) {
                runwayLog << " " << planeIDs[i];
            }
            runwayLog << "\n";
        }
        runwayLog << "Waiting on Ground:" ;
        vector<int> planeGroundIDs;
        for (int i = 0; i < tempEvent.waitingPlanesOnGround.size(); i++) {
            Plane tempPlane = tempEvent.waitingPlanesOnGround.front();
            tempEvent.waitingPlanesOnGround.pop();
            planeGroundIDs.push_back(tempPlane.id);
        }
        if (planeGroundIDs.size() == 0) {
            runwayLog << "\n";
        } else {
            for (int i = 0; i < planeGroundIDs.size(); i++) {
                runwayLog << " " << planeGroundIDs[i];
            }
            runwayLog << "\n\n";
        }
    }
    runwayLog.close();
}

/* landing_func --> takes in index to check if it is an emergency plane
 * sends signal to ATC Tower for permission to land
 * lands after receiving permission
 * if emergency plane, the plane lands immediately as soon as the runway is emptied
 */
void *landing_func(void * index) {
    time_t t = std::time(0);   // get time now
    tm* now = std::localtime(&t);

    Plane newPlane;
    if((long) index == 1){
        newPlane.direction = 'E'; // emergency direction
    } else {
        newPlane.direction = 'L'; // landing direction
    }
    pthread_cond_init (&newPlane.cond, NULL);
    realTime creationTime = {now->tm_hour, now->tm_min, now->tm_sec};
    newPlane.requestTime = creationTime; //arrival time generated

    pthread_mutex_lock(&idLock);
    newPlane.id = landingIdCounter; //unique id
    landingIdCounter+=2;
    pthread_mutex_unlock(&idLock);

    pthread_mutex_lock(&queueLock);
    landingQueue.push_back(newPlane); //wait in landing queue

    planeEvent tempEvent = createEvent(creationTime);
    eventQueue.push(tempEvent);

    if (newPlane.direction == 'E') {
        emergencyFlag = 1;
        pthread_cond_signal(&runwayCond); //signal immediate landing
    }
    else if(newPlane.id == landingQueue.front().id)  {
        pthread_cond_signal(&runwayCond);//signal the ATC Tower
    }
    pthread_mutex_unlock(&queueLock);

    pthread_mutex_lock(&newPlane.lock);
    pthread_cond_wait(&newPlane.cond, &newPlane.lock); //wait for permission
    pthread_mutex_unlock(&newPlane.lock);


    pthread_exit(NULL);
}

/* departing_func --> sends signal to ATC Tower for permission to depart
 * departs after receiving permission
 */
void *departing_func(void *){
    time_t t = std::time(0);   // get time now
    tm* now = std::localtime(&t);

    Plane newPlane;
    newPlane.direction = 'D'; //direction
    pthread_cond_init (&newPlane.cond, NULL);
    realTime creationTime = {now->tm_hour, now->tm_min, now->tm_sec};
    newPlane.requestTime = creationTime; //arrival time generated

    pthread_mutex_lock(&idLock);
    newPlane.id = departingIdCounter; //unique id
    departingIdCounter+=2;
    pthread_mutex_unlock(&idLock);

    pthread_mutex_lock(&queueLock);
    departingQueue.push(newPlane); //wait in departing queue

    planeEvent tempEvent = createEvent(creationTime);
    eventQueue.push(tempEvent);

    if(newPlane.id == departingQueue.front().id){
        pthread_cond_signal(&runwayCond);//signal the ATC Tower
    }
    pthread_mutex_unlock(&queueLock);

    pthread_mutex_lock(&newPlane.lock);
    pthread_cond_wait(&newPlane.cond, &newPlane.lock); //wait for permission
    pthread_mutex_unlock(&newPlane.lock);

    pthread_exit(NULL);
}

/* controlTower -> wait for signal from plane
 * gives permission to land or depart
 * manages starvation and emergency landing
 */
void *controlTower(void *){
    time_t t = std::time(0);   // get time now
    tm* now = std::localtime(&t);

    pthread_mutex_lock(&runwayLock);
    pthread_cond_wait(&runwayCond, &runwayLock);

    while(simulationContinues(now, finishTime)) { //works till the end of the simulation time
        int tempTotal = now->tm_sec + now->tm_min * 60 + now->tm_hour * 24 * 60;
        int tempBound = tempTotal + 1;
        realTime departingTime = {now->tm_hour, now->tm_min, now->tm_sec};

        pthread_mutex_lock(&queueLock);
        numLanding = landingQueue.size();
        numDeparting = departingQueue.size();

        if(emergencyFlag) {
            emergencyFlag = 0;
            Plane tempPlane = landingQueue.back();
            landingQueue.pop_back();
            tempPlane.runwayTime.hour = departingTime.hour; //runway time generated
            tempPlane.runwayTime.minute = departingTime.minute;
            tempPlane.runwayTime.second = departingTime.second;
            finishedQueue.push(tempPlane);
            pthread_mutex_unlock(&queueLock);
            pthread_cond_signal(&tempPlane.cond);
            pthread_sleep(2);
        }
        else if(numLanding>0 && starvationFlag == 0 ){
            if(numDeparting > 4 ){
                starvationFlag = 1;
            } else {
                starvationFlag = 0;
            }
            Plane tempPlane = landingQueue.front();
            landingQueue.pop_front(); //plane removed from landing queue
            tempPlane.runwayTime.hour = departingTime.hour; //runway time generated
            tempPlane.runwayTime.minute = departingTime.minute;
            tempPlane.runwayTime.second = departingTime.second;
            finishedQueue.push(tempPlane); //plane added to finished queue
            pthread_mutex_unlock(&queueLock);
            pthread_cond_signal(&tempPlane.cond); //signals plane successful landing
            pthread_sleep(2);
        } else if(starvationFlag == 1 || numLanding == 0 || numDeparting > 4){
            starvationFlag = 0;
            Plane tempPlane = departingQueue.front();
            departingQueue.pop(); //plane removed from departing queue
            tempPlane.runwayTime.hour = departingTime.hour; //runway time generated
            tempPlane.runwayTime.minute = departingTime.minute;
            tempPlane.runwayTime.second = departingTime.second;
            finishedQueue.push(tempPlane); //plane added to finished queue
            pthread_mutex_unlock(&queueLock);
            pthread_cond_signal(&tempPlane.cond); //signals plane successful departure
            pthread_sleep(2);
        }

        while(tempTotal < tempBound){ //works every second
            t = std::time(0);
            now = std::localtime(&t);
            tempTotal = now->tm_sec + now->tm_min*60 + now->tm_hour * 24 * 60;
        }
        pthread_mutex_unlock(&runwayLock);
    }
}
