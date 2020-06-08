# COMP 304 : Operating Systems
# Air Traffic Control Simulation with PThreads

The project simulates independently acting planes asking for permission to depart or land from the ATC Tower. The simulation is implemented in C++ as a scheduling algorithm with mutex locks.

## Completion of the Project

All parts of the project are implemented and the simulation works succesfully.

## Running the Project

To run the project in Linux environment or on Terminal: 

```
g++ -o runway.o main.cpp –lpthread
```

The output file accepts two parameters. The first parameter is the probability and the second one is the simulation duration time in seconds. 

```
./runway.o 0.5 -s 60
```

If the user does not enter the simulation time, it is set to 60 seconds by default. 

```
./runway.o 0.5 
```

If the user does not enter any parameters, the default values for the parameters are 0.5 for probability and 60 seconds for the simulation time duration. 

```
./runway.o
```

For ease of use, I included a Makefile in my project. To compile the code, 

```
make runway
```

To execute the simulation with predefined parameters of 0.5 probability and 60 second duration, 

```
make run
```

## Designing the Project

### Structs
Three structs **realTime**, **Plane** and **planeEvent** are defined in this project:
* The **realTime** is used to represent the current time. It consists of hour, minute and second. 
* The **Plane** is used to represent the independent planes. It consists of an id number, direction that indicates whether it will land or depart. requestTime which notifies the time it requested to use the resource which is the runway, runwayTime which notifies the end of the runway's usage and its own mutex and conditional locks. 
*  The **planeEvent** is used to log the events. It consists of eventTime, a double ended queue (or deque) of waiting planes on air and a queue of waiting planes on ground. 

### Threads
The project starts with 3 thread: one plane waiting to land, one plane waiting to depart and the air control tower. Each plane is represented as a thread and the air control tower has its own seperate thread. A plane arrives to the airport with probability p every 1 second and notifies the tower it is ready to land. In addition, a plane becomes ready to take off with probability 1-p every 1 second and notifies the tower it is ready to take-off. This is implemented through creation of a new thread every 1 second. 

### Functions
_main_:
* parses the command line arguments.
* marks the start and finish time of the simulation.
* creates the air control tower thread, initializes the runway with one landing and one departing plane
* generates a new plane every 1 second 

_logPlane_:
* the landed or departed planes are reported in the plane.log output file

_logRunway_:
* the flow of events are reported in the Runway-Records.log output file

_landing_func_:
* takes in index to check if it is an emergency plane 
* sends signal to ATC Tower for permission to land
* lands after receiving permission
* if emergency plane, the plane lands immediately as soon as the runway is emptied

_departing_func_:
* sends signal to ATC Tower for permission to depart
* departs after receiving permission

_controlTower_ :
* waits for signal from plane
* gives permission to land or depart
* manages starvation and emergency landing

_fixTime_: 
* adjusts the time after using the runway

_calculateSimulationTime_:
* calculates the simulation end time

_createEvent_:
* creates events to add to the runway log file

_simulationContinues_ :
* checks if the simulation can continue

### Locks
Three mutex locks  **runwayLock**, **queueLock** and **idLock** and one conditional lock **runwayCond** are defined in this project. In addition, each plane has its own mutex and conditional lock defined in its struct. The mutex locks are used when shared variables were updated. The globally defined runwayCond was used to notify the tower. Each plane's conditional locks were used to received back signal from the tower, regarding their permission. Once the signal was received, the plane stopped waiting. 

## Solving The Starvation Problem
### Part II Implementation Explained

The control tower favors the landing planes and lets the waiting planes to land until either no more planes are waiting to land or 5 or more planes are waiting to take-off from the ground. However this causes starvation for the planes on the air because, as a new plane arrives every second, the planes on the ground will have the turn to take off due to the increased size of its queue. From a point in time, the length of the departing planes will be always bigger than 5 causing them to always take the turn to depart and not giving a turn to the landing planes. In order to solve this, I decided to give the next turn to the plane that is in the queue which did not get turn in the previous runway usage. It could be summarized as prioritizing the landing planes until it does not cause starvation. Once the starvation begins, the planes start taking turns. If the previous turn was given to tne plane from the landing queue, the next turn should be given to the departing queue. The landing queue is still prioritized, so the fuel efficiency continues. 


## Author of the Project

* Sitare Arslantürk
