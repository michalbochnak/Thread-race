//
// Michal Bochnak
// Netid: mbochn2
// Homework #5
// Dec 5, 2017
//
// raceTest-mbochn2.h
//

//
//  Needed libraries
//
#include <iostream>     // cout/cin etc
#include <iomanip>      // setw
#include <cstdlib>      // atoi
#include <string>       //
#include <stdlib.h>     // srand, rand
#include <time.h>       // time
#include <sys/time.h>   // time
#include <pthread.h>    // thread
#include <unistd.h>     // sleep
#include <sys/sem.h>    // semafor
#include <semaphore.h>  // sem_getvalue
#include <sys/types.h>  // semafor
#include <sys/ipc.h>    // IPC
#include <stdio.h>      // perror
#include <vector>       // vector

using namespace std;


//
//  To allow compatibility
//
#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
    int val; /* value for SETVAL */
    struct semid_ds *buf; /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* array for GETALL, SETALL */
    /* Linux specific part: */
    struct seminfo *__buf; /* buffer for IPC_INFO */
};
#endif


//
//  Worker struct
//
struct worker_struct {
    int     nBuffers,
            workerID,
            semID,
            mutexID,
            nWorkers,
            nReadErrors;
    double  sleepTime;
    int*    buffers;
    bool    lock;
};


//
//  Function Definitions
//

//
// Displays author information
//
void showAuthorInfo() {
    cout << endl;
    cout << "   |----------------|" << endl;
    cout << "   | Michal Bochnak |" << endl;
    cout << "   | Netid: mbochn2 |" << endl;
    cout << "   | Homework #4    |" << endl;
    cout << "   | Nov 16, 2017   |" << endl;
    cout << "   |----------------|" << endl << endl;
}

//
//  Shows info about command line usage
//
void showUsageInfo() {
    cout << "Please follow the structure below for command line usage:" << endl << endl;
    cout << "<< raceTest nBuffers nWorkers [sleepMin sleepMax] "
            "[randSeed] [-lock | -nolock] >>" << endl << endl;
    cout << "- nBuffers must be prime number in range 2 - 32" << endl;
    cout << "- nWorkers must be positive and less than nBuffers" << endl;
    cout << "- sleepMin and sleepMax must be positive and sleepMin < sleepMax" << endl;
    cout << "- randSeed must be positive integer" << endl << endl;
    exit(-1);
}

//
//  Checks if given number is prime,
//  assumes number in the range 3 - 31 (enough for this program purposes)
//
bool isPrime(int num) {
    int primeNums[] = {3, 5, 7, 11, 13, 17, 19, 23, 29, 31};
    for (int i = 0; i < 10; ++i) {
        if (primeNums[i] == num)
            return true;
    }

    return false;
}

//
//  Parses command line arguments
//
void parseCmdLineArgs(int argc, char** argv, int &nBuffers, int &nWorkers,
                      double &sleepMin, double &sleepMax, int &randSeed, bool &lock) {

    if (argc < 3) {
        showUsageInfo();
    }
    else {
        nBuffers = atoi(argv[1]);
        if (!isPrime(nBuffers)) showUsageInfo();
        nWorkers = atoi(argv[2]);
        if (nWorkers >= nBuffers) showUsageInfo();
        if (argc >= 4) sleepMin = atof(argv[3]);
        if (argc >= 5) sleepMax = atof(argv[4]);
        if (argc >= 6) randSeed = atoi(argv[5]);
        if (argc >= 7) {
            string temp(argv[6]);
            if (temp.compare("-lock") == 0) lock = true;
            else lock = false;
        };

    }

}

//
//  Initialize buffers to zeros nad returns pointer to them
//
int* initializeBuffers(int nBuffers) {
    int* temp = (int*)malloc(sizeof(int) * nBuffers);
    for (int i = 0; i < nBuffers; ++i) {
        temp[i] = 0;
    }
    return temp;
}

//
//  Generates and returns random double
//
double generateRandom(double sleepMin, double sleepMax, int randSeed) {
    if (randSeed > 0)
        srand(randSeed);
    else
        srand(time(NULL));

    return (sleepMin + (sleepMax - sleepMin) * rand() / RAND_MAX);
}

//
//  Initializes the worker structs array and return pointer to them
//
worker_struct* initializeWorkerStructs(int nBuffers, int nWorkers, double sleepMin,
               double sleepMax, int randSeed, int* buffers, int screenMutex,
                                       int writeSem, bool lock) {

    worker_struct* temp = (worker_struct*)malloc(sizeof(worker_struct) * nWorkers);

    for (int i = 0; i < nWorkers; ++i) {
        temp[i].nBuffers = nBuffers;
        temp[i].workerID = i + 1;
        temp[i].semID = writeSem;
        temp[i].mutexID = screenMutex;
        temp[i].nWorkers = nWorkers;
        temp[i].nReadErrors = 0;
        temp[i].sleepTime = generateRandom(sleepMin, sleepMax, randSeed);
        temp[i].buffers = buffers;
        temp[i].lock = lock;
    }

    return temp;
}

