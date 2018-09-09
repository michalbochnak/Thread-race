//
// Michal Bochnak
// Netid: mbochn2
// Homework #5
// Dec 5, 2017
//
// raceTest-mbochn2.cpp
//

//
//  Needed libraries
//
#include "raceTest-mbochn2.h"

//
//  Function prototypes
//
void* worker(void* worker_struct);      //  Worker thread prototype
void createThreads(pthread_t *tids, int nWorkers, worker_struct *workerStruct);
void joinThreads(pthread_t *tids, int nWorkers);
void showBuffers(int* buffers, int nBuffers);
void processReadOper(worker_struct* workerStruct, int accessedBufferIndex);
void processWriteOper(worker_struct* workerStruct, int accessedBufferIndex);
void showBadBits(int bufferVal, int desiredVal, int numOfBits);
vector<string> getBadBits (int val, int desiredVal, int numOfBits);
void showBadBits(vector<string> badBits);


//
//  main
//
int main(int argc, char** argv) {

    int     nBuffers,   nWorkers,
            randSeed    = 0;    // 0 indicates default value
    int     screenMutex, writeSem;
    double  sleepMin    = 1.0,  sleepMax = 5.0;
    bool    lock        = false;
    clock_t startTime = clock();
    union   semun semUn;
    semUn.val = 1;
    struct timeval before, after;

    // create semaphore to control screen output
    if ((screenMutex = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600 )) < 0) {
        perror("semget error");
        exit(-1);
    }
    if (semctl(screenMutex, 0, SETVAL, semUn) < 0) {
        perror("semctl error");
        exit(-1);
    }

    gettimeofday(&before, NULL);

    showAuthorInfo();
    parseCmdLineArgs(argc, argv, nBuffers, nWorkers,
                     sleepMin, sleepMax, randSeed, lock);

    // create array of semaphores with size of nBuffers
    if ((writeSem = semget(IPC_PRIVATE, nBuffers, IPC_CREAT | 0600 )) < 0) {
        perror("semget error");
        exit(-1);
    }
    // initialize all
    for (int i = 0; i < nBuffers; i++) {
        if (semctl(writeSem, i, SETVAL, semUn) < 0) {
            perror("semctl error");
            exit(-1);
        }
    }

    // create buffers array and initialize buffers to 0's
    int* buffers = initializeBuffers(nBuffers);

    // create worker structs and initialize them
    worker_struct* workerStruct = initializeWorkerStructs(nBuffers, nWorkers,
            sleepMin, sleepMax, randSeed, buffers, screenMutex, writeSem, lock);

    // create arrays of thread id's, start threads, and join them
    pthread_t tids[nWorkers];
    createThreads(tids, nWorkers, workerStruct);
    joinThreads(tids, nWorkers);

    // grab value that should be in buffers if there were no errors
    int correctValue = (1 << nWorkers) - 1;
    // create variables to count errors
    int readErrors = 0;
    int writeErrors = 0;

    // count read errors
    for (int i = 0; i < nWorkers; ++i) {
        readErrors += workerStruct[i].nReadErrors;
    }

    // count write errors
    cout << endl;
    for (int i = 0; i < nBuffers; ++i) {
        cout << "Buffer " << left << setw(2) << i << " value: " << left << setw(10) << buffers[i];
        if (buffers[i] != correctValue) {
            vector<string> badBits = getBadBits(buffers[i], correctValue, nWorkers);
            showBadBits(badBits);
            writeErrors += badBits.size();
        }
        cout << endl;
    }

    cout << endl << "Read errors detected: " << readErrors << endl;
    cout << "Write errors detected: " << writeErrors << endl << endl;

    semctl(screenMutex, 0, IPC_RMID);
    semctl(writeSem, 0, IPC_RMID);

    gettimeofday(&after, NULL);

    cout << "Execution time: " <<
                               ((after.tv_sec - before.tv_sec) + (after.tv_usec / ((double)1000000)))
         << " seconds" << endl << endl;

    // done
    cout << "\n   |------|\n   | Done |\n   |------|\n" << endl;
    return 0;
}


//
//  Functions definitions
//
void showBadBits(vector<string> badBits) {

    cout << "(Bad bits:";

    for (int i = 0; i < badBits.size(); ++i) {
        cout << " " << badBits[i];
    }

    cout << ")";
}

//
// Checks which bits are bad and return vector with them
//
vector<string> getBadBits (int val, int desiredVal, int numOfBits) {

    vector<string> badBits;

    for (int i = 0; i < numOfBits; ++i) {

        if ( ((int)(val & 1) == 0) && ((int)(desiredVal & 1) == 1) ) {
            char str[20];
            sprintf(str, "-%d", i);
            badBits.push_back(str);
        }
        else if ( ((int)(val & 1) == 1) && ((int)(desiredVal & 1) == 0) ) {
            char str[20];
            sprintf(str, "+%d", i);
            badBits.push_back(str);
        }

        val >>= 1;
        desiredVal >>= 1;
    }
    return badBits;
}

//
// Worker thread
//
void* worker(void* workerStructParam) {
    // cast void pointer to the workerStruct
    worker_struct* workerStruct = (worker_struct*)workerStructParam;
    int accessedBufferIndex = workerStruct->workerID;
    // initialize struct
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;

    for (int i = 0; i < (workerStruct->nBuffers * 3); ++i) {

        int semToLock = accessedBufferIndex;

        // lock
        if (workerStruct->lock) {
            sb.sem_num = semToLock;
            sb.sem_op = -1;
            sb.sem_flg = 0;
            if ((semop(workerStruct->semID, &sb, 1)) == -1) {
                perror("semop");
                exit(-1);
            }
        }

        // reading access
        if ((i + 1) % 3 != 0) {

            if (workerStruct->workerID == 3) {
                cout << "r " << accessedBufferIndex << endl;
            }

            int initial = workerStruct->buffers[accessedBufferIndex];
            usleep((workerStruct->sleepTime) * 1000000);
            int final = workerStruct->buffers[accessedBufferIndex];

            if (initial != final) {

                // lock
                sb.sem_num = 0;
                sb.sem_op = -1;
                sb.sem_flg = 0;
                if (semop(workerStruct->mutexID, &sb, 1) == -1) {
                    perror("semop");
                    exit(-1);
                }

                // display
                cout << "Worker number " << left << setw(2) << workerStruct->workerID
                     << " reported change from  " << setw(10) << initial << "  to  "
                     << setw(10) << final << "  in buffer " << setw(2)
                     << accessedBufferIndex << "  ";

                // grab bad bits vector
                vector<string> badBits = getBadBits(final, initial, workerStruct->nWorkers);
                showBadBits(badBits);
                cout << endl;

                // unlock
                sb.sem_num = 0;
                sb.sem_op = 1;
                sb.sem_flg = 0;
                if (semop(workerStruct->mutexID, &sb, 1) == -1) {
                    perror("semop");
                    exit(-1);
                };

                workerStruct->nReadErrors += badBits.size();
            }
        }
        // writing access
        else {

            if (workerStruct->workerID == 3) {
                cout << "w " << accessedBufferIndex << endl;
            }

            int initial = workerStruct->buffers[accessedBufferIndex];
            //cout << "--Worker " << workerStruct->workerID << " read " << initial << " from " << accessedBufferIndex << endl;
            usleep((workerStruct->sleepTime) * 1000000);

            initial += (1 << (workerStruct->workerID - 1));
            workerStruct->buffers[accessedBufferIndex] = initial;

            //cout << "--Worker " << workerStruct->workerID << " writing " << initial << " to " << accessedBufferIndex << endl;
        }

        // unlock
        if (workerStruct->lock) {
            sb.sem_num = semToLock;
            sb.sem_op = 1;
            sb.sem_flg = 0;
            if (semop(workerStruct->semID, &sb, 1) == -1) {
                perror("semop");
                exit(-1);
            }
        }



        // update index
        accessedBufferIndex = (accessedBufferIndex + workerStruct->workerID)
                              % (workerStruct->nBuffers);
    }

    pthread_exit(NULL);
}

//
// Start all the threads
//
void createThreads(pthread_t *tids, int nWorkers, worker_struct *workerStruct) {
    for (int i = 0; i < nWorkers; ++i) {
        pthread_create(&tids[i], NULL, worker, &(workerStruct[i]));
    }
}

//
// Join all the threads
//
void joinThreads(pthread_t *tids, int nWorkers) {
    for (int i = 0; i < nWorkers; ++i) {
        pthread_join(tids[i], NULL);
    }
}

//
// Show buffers content
//
void showBuffers(int* buffers, int nBuffers) {
    cout << endl;
    for (int i = 0; i < nBuffers; ++i) {
        cout << "Buffers[" << setw(2) << i << "]: " << buffers[i] << endl;
    }
}

