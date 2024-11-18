#include <cstdlib>
#include <iostream>
#include <ctime>
#include <vector>
#include <iomanip>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <chrono>
#include <thread>
#include <semaphore.h>
#include <fstream>
#include <sys/wait.h>
#include <queue>

const int SEED = 5;
const float LO = 0.5;
const float HI = 999.99;

struct Date
{
    const int year = 2016;
    int day = std::rand()%29 + 1; // 1-30
    int month = std::rand()%11 + 1; // 1 -12

    Date& operator=(const Date& r){
        day = r.day;
        month = r.month;
        return *this;
    }

};

std::ostream& operator<<(std::ostream& os, const Date& d) {
    os << d.day << "/" << d.month << "/" << d.year;
    return os;
};


struct SalesData
{
    //sales date date
    Date date;

    //store ID int
    //storeID = producersID
    int storeID = -1; 

    //register# int
    int regesterNum = std::rand()%5 + 1;

    //sales amount float .. really should be an int when dealing with money
    float salesAmount = LO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(HI-LO)));

    SalesData &operator=(SalesData const& r){
        date = r.date;
        regesterNum = r.regesterNum;
        salesAmount = r.salesAmount;
        storeID = r.storeID;
        return *this;
    }
};

std::ostream& operator<<(std::ostream& os, const SalesData& sd) {
    os << sd.date << "\t" << sd.storeID << "\t\t" << sd.regesterNum << "\t\t" << std::fixed << std::setprecision(2) << sd.salesAmount << "\n";
    return os;
};

struct statistics
{
    float storeWideTotalSales;
    float monthWideTotalSales = 0;
    float aggregateSales = 0; //all sales together
    time_t totalTime;
    int NumProduceds = 0;
    int numConsumers = 0;
    std::vector<SalesData> salesDataVec;
};

// input producers, consumers, and buffer size
int main(int argc, char const *argv[])
{
    std::srand(SEED);

    long unsigned int producers = 1;
    long unsigned int consumers = 1;
    long unsigned int salesDataBufferSize = sizeof(statistics)+sizeof(SalesData)*10;

    if (argc > 3)
    {
        producers = atoi(argv[0]);
        consumers = atoi(argv[1]);
    }
    salesDataBufferSize = sizeof(statistics) + sizeof(SalesData) * atoi(argv[1]);

    sem_t *semaphore;
    semaphore = sem_open("pSem", O_CREAT, 0644, 1);

    if(semaphore == SEM_FAILED){
        sem_close(semaphore);
        perror("Failed to open semphore for full");
        exit(-1);
    }

    const char *sharedSalesDataLocation = "pcp.conf";
    const int projectID = 65;   

    time_t startTime, endTime;

    std::ofstream outfile(sharedSalesDataLocation);
    outfile.close();

    // ftok to generate unique key
    key_t key = ftok(sharedSalesDataLocation, projectID);

    if (key == -1)
    {
        perror("ftok");
        exit(1);
    }
    
    // shmget returns an identifier in shmid
     int shmid = shmget(key, salesDataBufferSize, 0666 | IPC_CREAT);
     
     if (shmid == -1)
     {
        perror("shmget");
        exit(1);
     }

    // shmat to attach to shared memory
    statistics *sharedSalesData = (statistics*)shmat(shmid, (void*)0, 0);

    if ((void*) sharedSalesData == (void *) -1)
    {
        perror("shmat");
        exit(1);
    }

    sharedSalesData->salesDataVec.reserve()

    pid_t pid0 = fork();

    if (pid0 == -1) { 
        perror("fork"); 
        exit(EXIT_FAILURE); 
    }

    if (pid0 > 0) {  
       
        std::cout << "Date\t\tStore ID\tRegester #\tSales Amount\t\n";
        std::cout << "printed from parent process " << getpid() << "\n";
      
        std::cout << "printed from parent process # of produced " << sharedSalesData->NumProduceds << "\n";
       
        while (sharedSalesData->NumProduceds < 10)
        {
            std::cout << "printed from parent process " << getpid() << "\n";
            if(sem_trywait(semaphore)){
                //std::this_thread::sleep_for(std::chrono::seconds(1));
                sharedSalesData->salesDataVec.emplace(SalesData());
                sharedSalesData->NumProduceds++;
                // Release mutex sem: V (mutex_sem)
                if (sem_post (semaphore) == -1) {
                    perror ("sem_post: mutex_sem"); exit (1);
                }
            } else
            {
                std::cout << "lock0 failed\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
           
        }
    } 
    else { 
        std::cout << "printed from child process # of consumers " << sharedSalesData->numConsumers << "\n";
        while (sharedSalesData->numConsumers < 10)
        {
            std::cout << "printed from child process " << getpid() << "\n"; 
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if(sem_trywait(semaphore) && !sharedSalesData->salesDataQueue.empty()){
                std::cout << sharedSalesData->salesDataQueue.front() << "\n";
                sharedSalesData->numConsumers++;
                sharedSalesData->salesDataQueue.pop();
                //sharedSalesData->salesDataQueue.pop_back();
                //memcpy(sharedSalesData, &stat, sizeof(stat));
                // Release mutex sem: V (mutex_sem)
                if (sem_post (semaphore) == -1) {
                    perror ("sem_post: mutex_sem"); exit (1);
                }
            } else
            {
                 std::cout << "lock1 failed\n";
                 std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            
        }
    }

    sem_unlink("pSem");
    shmdt(semaphore);
    // detach from shared memory
    shmdt(sharedSalesData);
    wait(NULL);
    shmctl(shmid,IPC_RMID,NULL);
    
    return 0;
}
