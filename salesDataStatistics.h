#include <iostream>
#include <queue>
#include <iomanip>

const int SEED = 5;
const float LO = 0.5;
const float HI = 999.99;




class salesDataStatistics
{
    private:

    struct SalesData{
    //sales date date
    struct Date{
        const int year = 2016;
        int day = std::rand()%29 + 1; // 1-30
        int month = std::rand()%11 + 1; // 1 -12

        Date& operator=(const Date& r){
            day = r.day;
            month = r.month;
            return *this;
        }
    };

    friend std::ostream& operator<<(std::ostream& os, const Date& d) {
        os << d.day << "/" << d.month << "/" << d.year;
        return os;
    }

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
        friend std::ostream& operator<<(std::ostream& os, const SalesData& sd){
            os << sd.date << "\t" << sd.storeID << "\t\t" << sd.regesterNum << "\t\t" << std::fixed << std::setprecision(2) << sd.salesAmount << "\n";
            return os;
        };
    };

    public:
    float storeWideTotalSales = 0;
    float monthWideTotalSales = 0;
    float aggregateSales = 0; //all sales together
    time_t totalTime;
    int NumProduceds = 0;
    int numConsumers = 0;
    std::vector<SalesData> SalesDataQueue;


    
        salesDataStatistics(/* args */);
        ~salesDataStatistics();
        
        
    
};
salesDataStatistics::salesDataStatistics(/* args */)
{
    std::srand(SEED);
  
}

salesDataStatistics::~salesDataStatistics()
{
}





