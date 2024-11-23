#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <vector>
#include <iomanip>
#include <thread>
#include <sys/wait.h>
#include <fstream>
#include <string.h>
#include <sys/mman.h>
#include <asm-generic/fcntl.h>

const int SEED = 5;
const float LO = 0.5;
const float HI = 999.99;

struct Date
{
    int year = 2016;
    int day = std::rand()%30 + 1; // 1-30
    int month = std::rand()%12 + 1; // 1 -12

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
    int regesterNum = std::rand()%6 + 1;

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
    float *storeWideTotalSales; // p size
    float monthWideTotalSales[12];
    float aggregateSales = 0; //all sales together
    time_t totalTime;
    int buffer = 0;
    int totalProduced = 0;
    SalesData *salesDataArr;
    sem_t *semaphore;
};

void consumersFun(statistics *sharedSalesData);

void ProducersFun(statistics *sharedSalesData);

// input producers, consumers, and buffer size
int main(int argc, char const *argv[])
{
    time_t startTime, endTime;

    std::srand(SEED);

    long unsigned int producers = 1;
    long unsigned int consumers = 1;
    long unsigned int salesDataBufferSize = sizeof(statistics)*10;

    if (argc > 3)
    {
        producers = atoi(argv[0]);
        consumers = atoi(argv[1]);
        salesDataBufferSize = sizeof(statistics) + sizeof(SalesData) * atoi(argv[2])+ sizeof(float)*producers;
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
        exit(EXIT_FAILURE);
    }

    // shmat to attach to shared memory
    statistics *sharedSalesData = (statistics*)shmat(shmid, (void*)0, 0);

    if (sharedSalesData == (void*)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    sharedSalesData->storeWideTotalSales = (float *)mmap(NULL, sizeof(float) * producers, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sharedSalesData->storeWideTotalSales == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    sharedSalesData->salesDataArr = (SalesData *)mmap(NULL, sizeof(SalesData) * producers, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sharedSalesData->salesDataArr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    sem_unlink("pSem");// just in case of a crash can be cleared

    sharedSalesData->semaphore = (sem_t*)mmap(NULL,sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sharedSalesData->semaphore == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
        // Create and initialize the semaphore
        if ((sharedSalesData->semaphore = sem_open("pSem", O_CREAT | O_RDWR | O_EXCL, 0666, 1)) == SEM_FAILED) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }

    sharedSalesData->buffer = -1;
    sharedSalesData->totalProduced = 0;
    sem_post(sharedSalesData->semaphore);
    
    pid_t pid0 = fork();

    if (pid0 == -1) { 
        perror("fork"); 
        shmdt(sharedSalesData);
        shmctl(shmid, IPC_RMID, NULL);
        sem_close(sharedSalesData->semaphore);
        sem_unlink("pSem");
        exit(EXIT_FAILURE); 
    }

    if (pid0 > 0) {
        std::cout << "printed from parent process " << getpid() << "\n";
        std::cout << "printed from parent process # of produced " << sharedSalesData->buffer << "\n";
        std::cout << "total parent " << sharedSalesData->totalProduced << "\n";
        std::vector<std::thread> producerThreads;
        for (int i = 0; i < producers; i++)
        {
            producerThreads.emplace_back(ProducersFun,sharedSalesData);
        }
        
        for (auto& t : producerThreads)
        {
            t.join();
        }
    } 
    else {
        std::cout << "printed from child process " << getpid() << "\n";
        std::cout << "total child " << sharedSalesData->totalProduced << "\n";
        std::vector<std::thread> consumerThreads;
        for (int i = 0; i < consumers; i++)
        {
            consumerThreads.emplace_back(consumersFun,sharedSalesData);
        }
        
        for (auto& t : consumerThreads)
        {
            t.join();
        }
        
        exit(0);//child exits
    }

    
    wait(NULL);
    for (int i = 0; i < 12; i++)
    {
        std::cout << "Month " << i+1 << " total sales ";
        for (int j = 0; j < 1/*size p*/; j++)
        {
            std::cout << "for store " << j << " " << sharedSalesData->monthWideTotalSales[i] << "\n";
        }
    }
    
    std::cout << "Aggregate sales " << sharedSalesData->aggregateSales << "\n";


    sem_close(sharedSalesData->semaphore);
    sem_unlink("pSem");
    shmdt(sharedSalesData);
    shmctl(shmid, IPC_RMID, NULL);
    
    return 0;
}

    void ProducersFun(statistics *sharedSalesData)
{
    SalesData salesTemp;
    while (sharedSalesData->totalProduced <= 100)
    {
        salesTemp = SalesData();
        salesTemp.storeID = getpid() % 6;
        sem_wait(sharedSalesData->semaphore);
        if (sharedSalesData->buffer < 10)
        {
            sharedSalesData->buffer++;
            sharedSalesData->salesDataArr[sharedSalesData->buffer] = salesTemp;
            sharedSalesData->totalProduced++;
        }
        sem_post(sharedSalesData->semaphore);

        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 35 + 5));
    }
}

void consumersFun(statistics *sharedSalesData)
{
    SalesData conSalesTemp;
    while (sharedSalesData->totalProduced <= 100)
    {
        sem_wait(sharedSalesData->semaphore);
        if (sharedSalesData->buffer >= 0)
        {
            std::cout << "Date\t\tStore ID\tRegester #\tSales Amount\t\n";
            std::cout << sharedSalesData->salesDataArr[sharedSalesData->buffer];
            conSalesTemp = sharedSalesData->salesDataArr[sharedSalesData->buffer];
            sharedSalesData->storeWideTotalSales[0] += conSalesTemp.salesAmount;
            sharedSalesData->aggregateSales += conSalesTemp.salesAmount;
            sharedSalesData->monthWideTotalSales[conSalesTemp.date.month - 1] += conSalesTemp.salesAmount;

            std::cout << "Store-wide total sales: " << sharedSalesData->storeWideTotalSales[0] << "\n";
            sharedSalesData->buffer--;
        }
        sem_post(sharedSalesData->semaphore);

        // std::this_thread::sleep_for(std::chrono::seconds(1));
        // sharedSalesData->rear--;
    }
    exit(0); // child exits
}
