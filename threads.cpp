#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <fstream>
#include <dirent.h>
#include <random>
#include <semaphore.h>
#include <iomanip>
#include <ctime>
#include <string>
#include <unordered_map>
#include "nlohmann/json.hpp"

int K = -1; // Max size of the buffer
int n, m = -1; // Number of consumer and producer threads
int a, b = -1; // Numbers that consumer will choose from
int c, d = -1; // Numbers that producer will choose from
pthread_mutex_t mutex;
sem_t producer;
sem_t consumer;
int failures = 0; // Number of failed attempts to consume/produce

using json = nlohmann::json;

void removePreviousLogs() {
    DIR* dir = opendir("logs");
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {
                std::string filename = "logs/" + std::string(entry->d_name);
                std::remove(filename.c_str());
            }
        }
        closedir(dir);
    }
    std::cout << "Previous logs removed." << std::endl;
}

std::string formatTime(std::time_t timestamp) {
    std::tm* localTime = std::localtime(&timestamp);

    std::ostringstream formattedTime;
    formattedTime << std::put_time(localTime, "%T");

    return formattedTime.str();
}

int generateRandomNumber(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(gen);
}

int getWarehouseContent() {
    std::ifstream warehouse("warehouse.txt");
    if (warehouse.is_open()) {
        int content;
        warehouse >> content;
        warehouse.close();
        return content;
    } else {
        std::cerr << "Error opening file: warehouse.txt" << std::endl;
        return -1;
    }
}

void writeLog(std::string filename, std::string message) {
    message += "; Warehouse content: " + std::to_string(getWarehouseContent());
    std::ofstream logfile(filename, std::ios_base::app);
    if (logfile.is_open()) {
        logfile << message << std::endl;
        logfile.close();
        std::cout << message << std::endl;
    } else {
        std::cerr << "Error opening file: " << filename << std::endl;
    }
}

void setWarehouseContent(int content) {
    std::ofstream warehouse("warehouse.txt");
    if (warehouse.is_open()) {
        warehouse << content;
        warehouse.close();
    } else {
        std::cerr << "Error opening file: warehouse.txt" << std::endl;
    }
}

std::string generateMessage(std::string threadName, std::time_t time, std::string message) {
    std::string formattedTime = formatTime(time);
    return "ID: " + threadName + "; Time stamp: " + formattedTime + "; Message: " + message;
}

void* consumerThread(void* arg) {
    int threadId = *(reinterpret_cast<int*>(arg));
    std::string threadName = "cons_" + std::to_string(threadId); 
    std::string filename = "logs/cons_" + std::to_string(threadId) + "_log.txt";
    int consumedProducts = generateRandomNumber(a, b);
    bool consumerTurn;

    while (true) {
        consumerTurn = true;
        sem_wait(&consumer);
        pthread_mutex_lock(&mutex);
        int warehouse_content = getWarehouseContent();
        if (warehouse_content >= consumedProducts) {
            setWarehouseContent(warehouse_content - consumedProducts);
            std::string message = generateMessage(threadName, std::time(nullptr), "Managed to consume " + std::to_string(consumedProducts) + " products");
            failures = 0;
            writeLog(filename, message);
            consumedProducts = generateRandomNumber(a, b);
            message = generateMessage(threadName, std::time(nullptr), "Will consume " + std::to_string(consumedProducts) + " products next time");
            writeLog(filename, message);
            consumerTurn = (getWarehouseContent() > K / 2) ? true : false;
        } else {
            std::string message = generateMessage(threadName, std::time(nullptr), "Failed to consume " + std::to_string(consumedProducts) + " products");
            writeLog(filename, message);
            consumerTurn = false;
            failures++;
            if (failures > n + m) {
                consumedProducts = generateRandomNumber(a, b);
                std::string message = generateMessage(threadName, std::time(nullptr), "Warehouse jammed. Will try to consume " + std::to_string(consumedProducts) + " products next time");
                writeLog(filename, message);
            }
        }
        sleep(1);
        pthread_mutex_unlock(&mutex);
        if (consumerTurn) {
            sem_post(&consumer);
        } else {
            sem_post(&producer);
        }
        sleep(1);
    }
    
    pthread_exit(nullptr);
}

void* producerThread(void* arg) {
    int threadId = *(reinterpret_cast<int*>(arg));
    std::string threadName = "prod_" + std::to_string(threadId); 
    std::string filename = "logs/prod_" + std::to_string(threadId) + "_log.txt";
    int producedProducts = generateRandomNumber(c, d);
    bool producerTurn;

    while (true) {
        producerTurn = true;
        sem_wait(&producer);
        pthread_mutex_lock(&mutex);
        int warehouse_content = getWarehouseContent();
        if (warehouse_content + producedProducts <= K) {
            setWarehouseContent(warehouse_content + producedProducts);
            std::string message = generateMessage(threadName, std::time(nullptr), "Managed to produce " + std::to_string(producedProducts) + " products");
            failures = 0;
            writeLog(filename, message);
            producedProducts = generateRandomNumber(c, d);
            writeLog(filename, generateMessage(threadName, std::time(nullptr), "Will produce " + std::to_string(producedProducts)) + " products next time");
            producerTurn = (getWarehouseContent() < K / 2) ? true : false;
        } else {
            std::string message = generateMessage(threadName, std::time(nullptr), "Failed to produce " + std::to_string(producedProducts) + " products");
            writeLog(filename, message);
            producerTurn = false;
            failures++;
            if (failures > n + m) {
                producedProducts = generateRandomNumber(c, d);
                std::string message = generateMessage(threadName, std::time(nullptr), "Warehouse jammed. Will try to produce " + std::to_string(producedProducts) + " products next time");
                writeLog(filename, message);
            }
        }
        sleep(1);
        pthread_mutex_unlock(&mutex);
        if (producerTurn) {
            sem_post(&producer);
        } else {
            sem_post(&consumer);
        }
        sleep(1);
    }

    pthread_exit(nullptr);
}

int main(int argc, char *argv[]) {

    // Open config file
    std::ifstream configFile("config.json");

    if (configFile.is_open()) {

        json config;
        try {
            configFile >> config;
        } catch (const json::parse_error& e) {
            std::cerr << "Error parsing config.json: " << e.what() << "\n";
            return 1;
        }

        K = config.value("warehaouseSize", -1);
        n = config.value("numberOfConsumers", -1);
        m = config.value("numberOfProducers", -1);
        a = config.value("a", -1);
        b = config.value("b", -1);
        c = config.value("c", -1);
        d = config.value("d", -1);

    } else {
        std::cerr << "No such file as: config.json. Gathering data from console..." << std::endl;
    }

    std::unordered_map<std::string, int*> argMap = {
        {"-K", &K},
        {"--warehouse-max", &K},
        {"-n", &n},
        {"--num-consumers", &n},
        {"-m", &m},
        {"--num-producers", &m},
        {"-a", &a},
        {"--lower-a-bound", &a},
        {"-b", &b},
        {"--upper-b-bound", &b},
        {"-c", &c},
        {"--lower-c-bound", &c},
        {"-d", &d},
        {"--upper-d-bound", &d}
    };

    // Parse command-line arguments
    for (int i = 1; i < argc; i += 2) {
        std::string arg = argv[i];
        auto it = argMap.find(arg);
        if (it != argMap.end() && i + 1 < argc) {
            *(it->second) = std::stoi(argv[i + 1]);
        } else {
            std::cerr << "Error: Unknown option or missing argument for " << arg << "\n";
            return 1;
        }
    }

    if (K <= 0) {
        std::cerr << "K must be a positive integer. But was given: " << K << std::endl;
        return 1;
    }

    if (n <= 0) {
        std::cerr << "n must be a positive integer. But was given: " << n << std::endl;
        return 1;
    }

    if (a <= 0 || a > K) {
        std::cerr << "a must be a positive integer less than K. But was given: " << a << std::endl;
        return 1;
    }

    if (b < a || b > K) {
        std::cerr << "b must be a positive integer, greater than or equal to a, and less than K. But was given: " << b << std::endl;
        return 1;
    }

    if (m <= 0) {
        std::cerr << "m must be a positive integer. But was given: " << m << std::endl;
        return 1;
    } 

    if (c <= 0 || c > K) {
        std::cerr << "c must be a positive integer less than K. But was given: " << c << std::endl;
        return 1;
    }

    if (d < c || d > K) {
        std::cerr << "d must be a positive integer, greater than or equal to c, and less than K. But was given: " << d << std::endl;
        return 1;
    }

    removePreviousLogs(); // Remove previous logs

    sleep(1);
    std::cout << "Warehouse max size: " << K << "\n";
    std::cout << "Number of consumer threads: " << n << "\n";
    std::cout << "Number of producer threads: " << m << "\n";
    std::cout << "Lower bound to generate package size for consumers: " << a << "\n";
    std::cout << "Upper bound to generate package size for consumers: " << b << "\n";
    std::cout << "Lower bound to generate package size for producers: " << c << "\n";
    std::cout << "Upper bound to generate package size for producers: " << d << "\n";
    std::cout << "Starting simulation...\n";
    sleep(1);

    // Initialize semaphores
    pthread_mutex_init(&mutex, nullptr);
    sem_init(&producer, 0, 1);
    sem_init(&consumer, 0, 0);

    //initialize warehouse content
    setWarehouseContent(0);

    // Create consumer threads
    pthread_t consumer_threads[n];
    for (int i = 0; i < n; ++i) {
        int* threadId = new int(i);
        pthread_create(&consumer_threads[i], nullptr, consumerThread, reinterpret_cast<void*>(threadId));
    }

    // Create producer threads
    pthread_t producer_threads[m];
    for (int i = 0; i < m; ++i) {
        int* threadId = new int(i);
        pthread_create(&producer_threads[i], nullptr, producerThread, reinterpret_cast<void*>(threadId));
    }

    // Wait for consumer threads to finish
    for (int i = 0; i < n; ++i) {
        pthread_join(consumer_threads[i], nullptr);
    }

    // Wait for producer threads to finish
    for (int i = 0; i < m; ++i) {
        pthread_join(producer_threads[i], nullptr);
    }

    // Destroy mutex and semaphores
    pthread_mutex_destroy(&mutex);
    sem_destroy(&consumer);
    sem_destroy(&producer);

    return 0;
}
