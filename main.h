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

#define SALES_LOCATION "pcp.conf"
#define SNAME "pSem"
#define PROJECT_ID 65
#define SEED 5
#define LO 0.5f
#define HI 999.99f
#define SHARED_MEMORY_SIZE 1024


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
    float *monthWideTotalSales[12];
    float aggregateSales = 0; //all sales together
    time_t totalTime;
    int buffer = 0;
    int tempID = 1;
    int totalProduced = 0;
    SalesData *salesDataArr;
    sem_t *semaphore;
    int capacity = 0;
};