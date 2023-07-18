#include "manager.h"
#include <iostream>
#include <string>
#include <unistd.h> 
#include <thread>
#include "pugixml-1.13/src/pugixml.hpp"
using namespace pugi;

manager::manager()
{
    cadence = 0;
}

manager::~manager()
{

}


void manager::writeOnSM(const QString &sender, const QString &message){
    //std::cout << message.toStdString() << std::endl;
    //TODO: if else if ... per scrivere su shared memory
    xml_document docString;
    xml_parse_result parsedMessage = docString.load(message.toStdString().c_str());

    if(!parsedMessage){
        //TODO: send message to client telling that message is not parseable
        std::cout << "return";
        return;
    }

    for(auto&& field: docString.children("message")){
        std::cout << "Received message: " << std::endl;
        std::cout << "\tType: \t\t" << field.child("type").text().as_string() << std::endl;
        std::cout << "\tPayload: \t" << field.child("payload").text().as_string() << std::endl;
    }
}

void manager::threadReadFromSM(){
    int hz = 1;
    while(true){
        usleep(5000000);
        std::string s = std::to_string(cadence++);
        //if (cadence%2 == 0)
            //emit sendToClient(QString::fromStdString("Cardio sensor not connected and cassano is the best player in the world"));
        //else
            //emit sendToClient(QString::fromStdString("Connection failed"));
        //TODO refactor
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