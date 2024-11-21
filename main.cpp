#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <queue>
#include <asm-generic/fcntl.h>
#include <iomanip>
#include <thread>
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
    int rear = 0;
    int totalProduced = 0;
    SalesData salesDataVec[10];
};

// input producers, consumers, and buffer size
int main(int argc, char const *argv[])
{
    struct timespec ts;
    time_t startTime, endTime;

    std::srand(SEED);
    statistics stat;

    long unsigned int producers = 1;
    long unsigned int consumers = 1;
    long unsigned int salesDataBufferSize = sizeof(stat);

    if (argc > 3)
    {
        producers = atoi(argv[0]);
        consumers = atoi(argv[1]);
        salesDataBufferSize = sizeof(statistics) + sizeof(SalesData) * atoi(argv[2]);
    }

    sem_t *semaphore;
    // Create and initialize the semaphore
    if ((semaphore = sem_open("pSem", O_CREAT, 0666, 1)) == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    const char *sharedSalesDataLocation = "pcp.conf";
    const int projectID = 65;   


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

    //memcpy(sharedSalesData, &stat, sizeof(stat));
    sharedSalesData->numProduceds = -1;
    sharedSalesData->rear = 0;
    sharedSalesData->totalProduced = 0;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
               printf("clock_gettime");

    ts.tv_sec = 3;
    sem_post(semaphore);
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
       
        std::cout << "printed from parent process " << getpid() << "\n";
        std::cout << "Date\t\tStore ID\tRegester #\tSales Amount\t\n";
        std::cout << "printed from parent process # of produced " << sharedSalesData->numProduceds << "\n";
        std::cout << "total parent " << sharedSalesData->totalProduced << "\n";
        std::cout << "rear parent " << sharedSalesData->rear << "\n";
        SalesData salesTemp;
        while (sharedSalesData->totalProduced <= 10 && sharedSalesData->rear >= -50)
        {
            salesTemp = SalesData();
            salesTemp.storeID = getpid()%6;
            if(sem_timedwait(semaphore,&ts) ==-1){
                printf("parent timed out\n");
            }else {
                if (sharedSalesData->totalProduced <= 10)
                {
                    sharedSalesData->numProduceds++;
                    sharedSalesData->salesDataVec[sharedSalesData->numProduceds] = salesTemp;
                    sharedSalesData->totalProduced = sharedSalesData->totalProduced + 1;
                }
                sem_post(semaphore);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(rand()%35+5));
            sharedSalesData->rear--;
        }
    } 
    else { 
        std::cout << "printed from child process # of consumers " << sharedSalesData->rear << "\n";
        std::cout << "printed from child process " << getpid() << "\n";
        std::cout << "total child " << sharedSalesData->totalProduced << "\n";
        std::cout << "rear " << sharedSalesData->rear << "\n";
        while (sharedSalesData->totalProduced <= 10 && sharedSalesData->rear >= -50)
        {
            if(sem_timedwait(semaphore,&ts) ==-1){
                printf("child timed out\n");
            }else {
                if(sharedSalesData->numProduceds >= 0)
                {
                    std::cout << sharedSalesData->salesDataVec[sharedSalesData->numProduceds] << "\n";
                    sharedSalesData->numProduceds--;
                }
            sem_post(semaphore);
            }
            //std::this_thread::sleep_for(std::chrono::seconds(1));
            //sharedSalesData->rear--;
        }
    }

    
    wait(NULL);
    shmdt(sharedSalesData);
    shmctl(shmid, IPC_RMID, NULL);
    sem_close(semaphore);
    
    return 0;
}
