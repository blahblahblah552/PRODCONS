
#include "main.h"

void consumersFun(statistics *sharedSalesData);

void ProducersFun(statistics *sharedSalesData);

std::mutex mut;

// input producers, consumers, and buffer size
int main(int argc, char const *argv[])
{
    auto startTime = std::chrono::high_resolution_clock::now();

    std::srand(SEED);

    int producers = 2;
    int consumers = 2;
    int capacity = 3;

    if (argc > 3)
    {
        producers = atoi(argv[1]);
        consumers = atoi(argv[2]);
        capacity  = atoi(argv[3]);
    }

    std::ofstream outfile(SALES_LOCATION);
    outfile.close();

    // ftok to generate unique key
    key_t key = ftok(SALES_LOCATION, PROJECT_ID);
    if (key == -1)
    {
        perror("ftok");
        exit(1);
    }
    
    // shmget returns an identifier in shmid
     int shmid = shmget(key, SHARED_MEMORY_SIZE, 0666 | IPC_CREAT);
     
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
    for (int i = 0; i < producers; i++)
    {
        sharedSalesData->monthWideTotalSales[i] = (float *)mmap(NULL, sizeof(float) * producers, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (sharedSalesData->monthWideTotalSales[i] == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
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
        if ((sharedSalesData->semaphore = sem_open(SNAME, O_CREAT | O_RDWR | O_EXCL, 0666, 1)) == SEM_FAILED) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }

    sharedSalesData->buffer = -1;
    sharedSalesData->totalProduced = 0;
    sharedSalesData->capacity = capacity;
    //sem_post(sharedSalesData->semaphore);
    
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
        std::vector<std::thread> consumerThreads;
        std::cout << "Date\t\tStore ID\tRegester #\tSales Amount\t\n";
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
    for (int i = 0; i < producers; i++)
    {
        std::cout << "Store " << i+1 <<" total sales: " << sharedSalesData->storeWideTotalSales[i] << "\n";
    }
    
    for (int i = 0; i < 12; i++)
    {
        std::cout << "Month " << i+1 << " total sales \n";
        for (int j = 0; j < producers; j++)
        {
            std::cout << "for store " << j+1 << " " << sharedSalesData->monthWideTotalSales[j][i] << "\n";
        }
    }
    
    std::cout << "Aggregate sales " << sharedSalesData->aggregateSales << "\n";

    sem_close(sharedSalesData->semaphore);
    sem_unlink("pSem");
    shmdt(sharedSalesData);
    shmctl(shmid, IPC_RMID, NULL);
    
    //save time
    std::ofstream timeFile(TIME_LOACATION, std::ios::app);
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> durationMS = endTime - startTime;
    std::cout << "Program execution time: " << durationMS.count() << "ms\n";
    timeFile <<producers << "\t" << consumers << "\t" << capacity << "\t" << durationMS.count() <<"ms\n";
    timeFile.close();
    return 0;
}

    void ProducersFun(statistics *sharedSalesData)
{
    SalesData tempProSales;
    sem_wait(sharedSalesData->semaphore);
    int tempID = sharedSalesData->tempID;
    sharedSalesData->tempID++;
    sem_post(sharedSalesData->semaphore);
    while (sharedSalesData->totalProduced <= NUM_OF_ITEMS)
    {
        tempProSales = SalesData();
        tempProSales.storeID = tempID;
        sem_wait(sharedSalesData->semaphore);
        if (sharedSalesData->buffer < sharedSalesData->capacity)
        {
            sharedSalesData->buffer++;
            sharedSalesData->salesDataArr[sharedSalesData->buffer] = tempProSales;
            sharedSalesData->totalProduced++;
        }
        sem_post(sharedSalesData->semaphore);

        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 35 + 5));
    }
}

void consumersFun(statistics *sharedSalesData)
{
    SalesData conSalesTemp;
    float localStat = 0.0f;
    while (sharedSalesData->totalProduced <= NUM_OF_ITEMS)
    {
        sem_wait(sharedSalesData->semaphore);
        if (sharedSalesData->buffer >= 0)
        {
            std::cout << sharedSalesData->salesDataArr[sharedSalesData->buffer];
            conSalesTemp = sharedSalesData->salesDataArr[sharedSalesData->buffer];
            sharedSalesData->storeWideTotalSales[conSalesTemp.storeID] += conSalesTemp.salesAmount;
            localStat= conSalesTemp.salesAmount;
            sharedSalesData->aggregateSales += conSalesTemp.salesAmount;
            sharedSalesData->monthWideTotalSales[conSalesTemp.storeID][conSalesTemp.date.month - 1] += conSalesTemp.salesAmount;
            sharedSalesData->buffer--;
        }
        sem_post(sharedSalesData->semaphore);
    }
    try
    {
        std::lock_guard<std::mutex> lock(mut);
        std::cout<< "Local statistics " << localStat << " ID: " << std::this_thread::get_id() << "\n";
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}
