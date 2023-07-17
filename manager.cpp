#include "manager.h"
#include <iostream>
#include <string>
#include <unistd.h> 
#include <thread>

manager::manager()
{
    cadence = 0;
}

manager::~manager()
{

}


void manager::writeOnSM(const QString &sender, const QString &message){
    std::cout << message.toStdString() << std::endl;
    //TODO: if else if ... per scrivere su shared memory
}

void manager::threadReadFromSM(){
    int hz = 1;
    while(true){
        usleep(5000000);
        std::string s = std::to_string(cadence++);
        emit sendToClient(QString::fromStdString(s));
    }
}

void manager::startThread() {
        
    // Create a new thread and pass a member function as the entry point
    std::thread t(&manager::threadReadFromSM, this);

    // Optionally, you can detach the thread if you don't need to join it later
    // t.detach();

    // Alternatively, you can join the thread to wait for it to finish
    t.detach();
    
    }

/*
#include <iostream>
#include <chrono>
#include <ctime>

int main() {
    // Get the current time point
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    // Convert the current time point to a time_t object
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    // Convert the time_t object to a string representation
    char buffer[26];
    ctime_s(buffer, sizeof(buffer), &currentTime);

    // Print the current time
    std::cout << "Current time: " << buffer;

    return 0;
}

*/