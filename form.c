// The function declarations were inspired by @donnemartin who has written an elevator simulation example in C++.        
// His github repository link-https://github.com/donnemartin/elevator-simulator
//Other sources used to work on this assignment include University of Illinois Chicago(UIC) header file homework 5 assignment 
// Here is the link -> https://www.cs.uic.edu/bin/view/CS385spring11/Homework5
// @Yue Wang github link -> https://github.com/Mooophy/Lift-Simulator
// And lastly, Kosoki Motohiro, 
// github link->https://stash.phytec.com/projects/PUB/repos/linux-phytec-fslc/browse/block/elevator.c?at=ad888a1f07a72fc7d19286b4ce5c154172a06eed
// To implement the project, I used the same function declarations and elevators&users structure definitions but I modified,and added 
// new functionality such as  inputting users destinations, randomly selecting the users from floors, initializing the elevators, structures, used barriers instead of conditional mutex variables, controlling the time using sleep functions,  and simulating the-
// elevators while using print statements.



#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


// Maximum number of users in a given elevator
#define CAPACITY 1

// Number of elevators in the building
#define ELEVATORS 2

// Number of floors in the buildingi i.e 0,1,2,3,4,5,6,7 floors
#define FLOORS 8 

// Total number of users in the building

#define userS 10

// A given user will use the elevator once before the simulation ends.
#define TRIPS 1

// Global array to store user Destination requests.
int arr[FLOORS];

// Keeps track of the number of users 
int user_count = 0;


// Program's Function Declarations

void dispatcher();
void user_request(int user, int from_floor, int to_floor, void (*enter)(int, int), void (*exit)(int, int));
void run_elevator(int elevator, int at_floor, void (*move_direction)(int, int), void (*door_open)(int), void (*door_close)(int));


// Represents the users requesting for elevators in the building.
struct user{
    int from_floor;
    int to_floor;
    int use_elevator;
    pthread_mutex_t lock;
    enum{
        WAITING,
        ENTERED,
        EXITED
    }state;

    int id;
    int in_elevator;
    pthread_barrier_t barr;

}users[userS];


// Represents the Elevator Structure in the building.
struct Elevator{    
    int open;
    int current_floor;
    int users;
    int trips;
    int direction;
    int at_floor;
    int id;
    int current_action_no;
    int previous_action_no;
    int in_elevator;
    int floor;
    int occupancy;
    pthread_mutex_t lock;
    struct user *p;
    enum{
        ELEVATOR_ARRIVED = 1,
        ELEVATOR_OPEN = 2,
        ELEVATOR_CLOSED = 3
    }state;
}elevators[ELEVATORS];


// Initializes the Elevators and Users structures
void dispatcher(){
    int i;

    // Initialize the 2 elevator

    for (i = 0; i < ELEVATORS; i++)
    {
        pthread_mutex_init(&elevators[i].lock, NULL);
        elevators[i].current_floor = 0;
        elevators[i].direction = -1;     
        elevators[i].occupancy = 0;
        elevators[i].state = ELEVATOR_CLOSED;
        elevators[i].p = NULL;
    }

    // Initialize the 10 users 

    for (i = 0; i < userS; i++)
    {
        pthread_barrier_init(&users[i].barr, NULL, 2);
    }
}

// when a user requests the elevator
void user_request(int user, int from_floor, int to_floor,void (*enter)(int, int),void (*exit)(int, int)){
    users[user].use_elevator = user % ELEVATORS;
    pthread_mutex_lock(&elevators[users[user].use_elevator].lock);
    pthread_mutex_unlock(&elevators[users[user].use_elevator].lock);
    pthread_mutex_lock(&elevators[users[user].use_elevator].lock);

    int index = users[user].use_elevator;
    users[user].id = user;
    int pass_id = users[user].id;
    users[user].from_floor = from_floor;
    users[user].to_floor = to_floor;
    elevators[index].p = &users[user];
    users[user].state = WAITING;    
    user_count++;


    // wait for the elevator to arrive at our original floor, then get in
    pthread_barrier_wait(&users[pass_id].barr);
    enter(pass_id, users[pass_id].use_elevator);
    elevators[index].occupancy++;
    users[user].state = ENTERED;
    pthread_barrier_wait(&users[pass_id].barr);

    pthread_mutex_unlock(&elevators[index].lock);

    // wait for the elevator at our destination floor, then get out
    pthread_mutex_lock(&elevators[index].lock);

    pthread_barrier_wait(&users[pass_id].barr);
    exit(pass_id, users[pass_id].use_elevator);
    users[user].state = EXITED;
    elevators[index].state = ELEVATOR_ARRIVED;
    elevators[index].occupancy--;
    elevators[index].p = NULL;

    pthread_mutex_unlock(&elevators[index].lock);
    pthread_barrier_wait(&users[pass_id].barr);
}

void run_elevator(int elevator, int at_floor,void (*move_direction)(int, int),void (*door_open)(int), void (*door_close)(int)){

    if (elevators[elevator].state == ELEVATOR_CLOSED){
        int difference;
        if (elevators[elevator].occupancy == 1){
            difference = (elevators[elevator].p->to_floor - elevators[elevator].current_floor);
            elevators[elevator].current_floor = elevators[elevator].p->to_floor;
        }
        else{if (elevators[elevator].p == NULL){
                return;
            }
            difference = (elevators[elevator].p->from_floor - elevators[elevator].current_floor);
            elevators[elevator].current_floor = elevators[elevator].p->from_floor;
        }
        move_direction(elevator, difference);
        elevators[elevator].state = ELEVATOR_ARRIVED;
    }

    if (elevators[elevator].state == ELEVATOR_OPEN){
        pthread_barrier_wait(&elevators[elevator].p->barr);

        door_close(elevator);
        elevators[elevator].state = ELEVATOR_CLOSED;
    }

    if (elevators[elevator].state == ELEVATOR_ARRIVED){
        door_open(elevator);
        elevators[elevator].state = ELEVATOR_OPEN;
        pthread_barrier_wait(&elevators[elevator].p->barr);
    }
}

static int flag = 0;

void elevators_status(int elevator)
{

    // Check the number of users in the Elevator
    if (elevators[elevator].users > CAPACITY){
        printf("Elevator exceeded the number of People. Remove %d user(s) ",(elevators[elevator].users-CAPACITY));
        EXIT_FAILURE;
    }else{
        elevators[elevator].previous_action_no = elevators[elevator].current_action_no;
    }

}

void move_elevators(int elevator, int direction)
{
    elevators_status(elevator);
    printf("Moving elevator %d %s from floor %d\n", elevator, (direction == -1 ? "DOWN" : "UP"), elevators[elevator].floor);

    if (elevators[elevator].floor >= FLOORS || elevators[elevator].floor < 0)
    {
        printf("ELevator can't move beyond Floor 7!\n");
        EXIT_FAILURE;
    }else{
    // Elevator takes 2 Seconds to move between floors.
    sleep(2);
    elevators[elevator].floor += direction;
    }


}

void open_door(int elevator){
    elevators_status(elevator);
    printf("Opening  elevator %d on floor %d.\n", elevator, elevators[elevator].floor);
    elevators[elevator].open = 1;
    // Takes 1 second for elevator to open the door
    sleep(1);
}

void close_door(int elevator){
    elevators_status(elevator);
    printf("Closing elevator %d on floor %d.It has %d user(s) in the elevator.\n", elevator, elevators[elevator].floor, elevators[elevator].users);
    elevators[elevator].open = 0;
    // Takes 1 second for elevator to close the door
    sleep(1);
}

void *start_elevator(void *arg){
    size_t elevator = (size_t)arg;
    struct Elevator *e = &elevators[elevator];
    (*e).previous_action_no = 0;
    (*e).current_action_no = 1;
    (*e).users = 0;
    (*e).trips = 0;
    printf("\nStarting elevator %ld...\n", elevator);

    (*e).floor = 0;
    while (!flag){
        (*e).current_action_no++;
        run_elevator(elevator, (*e).floor, move_elevators, open_door, close_door);
    }
}

void enter_elevator(int user, int elevator){
    // Check to see if the current elevator floor matches the user's current floor.
    // If it does, open the elevator door and let the user in.
    // If not, exit program.
    if (users[user].from_floor != elevators[elevator].floor){
        printf("Elevator at floor %d does not match the user %d current floor! %d.\n", users[user].id, users[user].from_floor, elevators[elevator].floor);
        EXIT_FAILURE;
    }else{

        printf("user %d has entered elevator %d on floor %d\n", users[user].id, elevator, users[user].from_floor);
        elevators[elevator].users++;
        users[user].in_elevator = elevator;
        users[user].state = ENTERED;

        sleep(2);

    }
  

}

void exit_elevator(int user, int elevator)
{
    if (users[user].to_floor != elevators[elevator].floor)
    {
        printf("User %d exited at the wrong floor (%d).His/Her destination was %d.\n", users[user].id, users[user].to_floor, elevators[elevator].floor);
        EXIT_FAILURE;
    }else{
        printf("user %d got off elevator %d at their requested floor %d.\n", users[user].id, elevator, users[user].to_floor);
        elevators[elevator].users--;
        elevators[elevator].trips++;
        users[user].in_elevator = -1;
        users[user].state = EXITED;
        sleep(1);
    }
}

void *start_user(void *arg)
{
    size_t user = (size_t)arg;
    struct user *p = &users[user];
    printf("\n\nStarting user %lu thread\n", user);
    (*p).from_floor = random() % FLOORS;
    (*p).id = user; 
    int trips = TRIPS;
    (*p).in_elevator = -1;

    
    while (!flag && trips-- > 0)
    {  
        printf("user %lu requesting elevator from floor %d to floor %d\n",
               user, (*p).from_floor, (*p).to_floor);

      
        users[user].state = WAITING;
        user_request(user, (*p).from_floor, (*p).to_floor, enter_elevator, exit_elevator);
        printf("\n\nuser %lu arrived at their requested floor!\n\n", user);

        (*p).from_floor = (*p).to_floor;
        sleep(1);
    }
}

int main(int argc, char *argv[]){


        printf("\nThe Destination of all the elevator Users(10 in total) is entered as an array.");
        printf("\nTheir from floor/ current locations are randomly chosen by the program."
        "\nSeparate the array inputs with a space e.g array[Destination] = 1 2 3 4 5 6 7 and so on."
        "\nInput the 10 Users Requests(number between 0 & 7) and then press enter:");
        // Users Destination input values e.g 1 2 3 4 5 6 7 1 2 3
        for(int i = 0; i < userS; i++){
            scanf("%d", &arr[i]);
        }


        // Populates the destination variable: to_floor with users input array values.
        for(int i=0; i < userS;i++){        
        users[i].to_floor = arr[i];
        }
        

        dispatcher();

        // Creates users threads
            pthread_t users[userS];
            for (size_t i = 0; i < userS; i++){
                pthread_create(&users[i], NULL, &start_user, (void *)i);
            }
            sleep(1);

        // Creates elevators threads
            pthread_t elevators[ELEVATORS];
            for (size_t i = 0; i < ELEVATORS; i++){
                pthread_create(&elevators[i], NULL, &start_elevator, (void *)i);
            }

            for (int i = 0; i < userS; i++){
                pthread_join(users[i], NULL);
            }
            flag = 1;

            for (int i = 0; i < ELEVATORS; i++){
                pthread_join(elevators[i], NULL);
            }
            printf("All %d users reached their destinations!\n", userS);
     
    return 0;
}
