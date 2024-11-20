#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <queue>
#include <asm-generic/fcntl.h>
#include <iomanip>
#include <bits/this_thread_sleep.h>
#include <sys/wait.h>
#include <fstream>
#include <string.h>

const int SEED = 5;
const float LO = 0.5;
const float HI = 999.99;

struct Date
{
    int year = 2016;
    int day = std::rand()%29 + 1; // 1-30
    int month = std::rand()%11 + 1; // 1 -12

    Date& operator=(const Date& r){
        year = r.year;
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
    int numProduceds = 0;
    int numConsumers = 0;
    std::array<SalesData, 10> salesDataVec;
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
        salesDataBufferSize = sizeof(statistics) + sizeof(SalesData) * atoi(argv[1]);
    }

    sem_t *semaphore;
    // Create and initialize the semaphore
    if ((semaphore = sem_open("pSem", O_CREAT, 0666, 2)) == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
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
     
    if (shmid == -1) {
        perror("shmget");
        sem_close(semaphore);
        sem_unlink("pSem");
        exit(EXIT_FAILURE);
    }

    // shmat to attach to shared memory
    statistics *sharedSalesData = (statistics*)shmat(shmid, (void*)0, 0);

    if (sharedSalesData == (void*)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        sem_close(semaphore);
        sem_unlink("pSem");
        exit(EXIT_FAILURE);
    }

    sharedSalesData->numProduceds = 0;
    sharedSalesData->numConsumers =0;

/*     statistics stat;
    stat.salesDataVec.reserve(10);
    memcpy(sharedSalesData, &stat, sizeof(stat)); */

    pid_t pid0 = fork();

    if (pid0 == -1) { 
        perror("fork"); 
        shmdt(sharedSalesData);
        shmctl(shmid, IPC_RMID, NULL);
        sem_close(semaphore);
        sem_unlink("pSem");
        exit(EXIT_FAILURE); 
    }

    if (pid0 > 0) {
    sem_post(semaphore);
       
        std::cout << "Date\t\tStore ID\tRegester #\tSales Amount\t\n";
        std::cout << "printed from parent process " << getpid() << "\n";
      
        std::cout << "printed from parent process # of produced " << sharedSalesData->numProduceds << "\n";
       
        while (sharedSalesData->numProduceds < 10)
        {
            sem_wait(semaphore);
            auto temp = SalesData();
            temp.storeID = getpid()%6;
            sharedSalesData->salesDataVec[sharedSalesData->numProduceds]=temp;
            sharedSalesData->numProduceds++;
            sem_post(semaphore);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } 
    else { 
        std::cout << "printed from child process # of consumers " << sharedSalesData->numConsumers << "\n";
        std::cout << "printed from child process " << getpid() << "\n"; 
        while (sharedSalesData->numConsumers < 10)
        {
            sem_wait(semaphore);
            if (sharedSalesData->numConsumers <= sharedSalesData->numProduceds)
            {
               std::cout << sharedSalesData->salesDataVec[sharedSalesData->numConsumers] << "\n";
                
                sharedSalesData->numConsumers++;
            } 
            sem_post(semaphore);
            //std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    
    wait(NULL);
    shmdt(sharedSalesData);
    shmctl(shmid, IPC_RMID, NULL);
    sem_close(semaphore);
    sem_unlink("pSem");
    
    return 0;
}
