#include "manager.h"
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h> 
#include <thread>
#include "pugixml-1.13/src/pugixml.hpp"

using namespace pugi;
using std::string;
using std::to_string;

manager::manager()
{
    shmem.init();
    shmem.data->start_training = 0;
    shmem.data->pid = 0;
    stopThread = false;
    stopSend = false;
}

manager::~manager()
{

}


void manager::writeOnSM(const QString &sender, const QString &message){
    //std::cout << message.toStdString() << std::endl;
    //TODO: if else if ... per scrivere su shared memory
    stopSend = true;
    std::cout << "stop send" << std::endl;
    xml_document docString;
    xml_parse_result parsedMessage = docString.load(message.toStdString().c_str());
    string type;
    string payload;

    if(!parsedMessage){
        //TODO: send message to client telling that message is not parseable
        std::cout << "return";
        return;
    }

    for(auto&& field: docString.children("message")){
        std::cout << "Received message: " << std::endl;
        std::cout << "\tType: \t\t" << field.child("type").text().as_string() << std::endl;
        std::cout << "\tPayload: \t" << field.child("payload").text().as_string() << std::endl;
         
        type=field.child("type").text().as_string();
        payload=field.child("payload").text().as_string();
    }

    if(type=="upAndDown"){
        shmem.data->up = payload=="plus";
        shmem.data->down = payload=="minus";
    }else if(type=="startAndStop"){
        shmem.data->start_training = payload=="start";
        // shmem.data->stop_training = payload=="stop";
    }else if(type=="pid"){
        shmem.data->pid=payload=="1";
    }
    stopSend = false;
    std::cout << "start send" << std::endl;
}
/**
 * Read periodically from shared memory and send message to client view
*/
void manager::threadReadFromSM(){
    float hz = 1;
    std::cout << "Updating client view at " << hz << " Hz" << std::endl;
    while(!stopThread){

        //TODO: startAndStop not ok to send to tablet! Need something like training ongoing
        if(!stopSend){
            std::vector<string> types = {"startAndStop"};//, "pid", "current_cadence"};
            std::vector<string> payloads = {to_string(shmem.data->start_training)};//, to_string(shmem.data->pid), to_string(shmem.data->current_cadence)};

            std::cout << "start -> " << shmem.data->start_training << std::endl;
            std::cout << "pid   -> " << shmem.data->pid << std::endl;

            string xmlMessage = buildXMLMessage(types, payloads);
            // std::cout << xmlMessage << std::endl;
        
            emit sendToClient(QString::fromStdString(xmlMessage));
        }
        usleep((int) (1.0/hz * 1e6));
    }

    std::cout << "Thread terminated" << std::endl;
}

void manager::startThread() {
        
    // Create a new thread and pass a member function as the entry point
    std::thread t(&manager::threadReadFromSM, this);

    // Optionally, you can detach the thread if you don't need to join it later
    // t.detach();

    // Alternatively, you can join the thread to wait for it to finish
    t.detach();
    
    }

string manager::buildXMLMessage(const std::vector<string>& types, const std::vector<string>& payloads) {
    if (types.size() != payloads.size()) {
        std::cout << "Types and payloads sizes are different. Cannot send message"; 
        return "";
    }

    std::string XMLmsg = "<messages>\n";
    for (size_t i = 0; i < types.size(); ++i) {
        XMLmsg += "  <message>\n";
        XMLmsg += "    <type>" + types[i] + "</type>\n";
        XMLmsg += "    <payload>" + payloads[i] + "</payload>\n";
        XMLmsg += "  </message>\n";
    }
    XMLmsg += "</messages>\n";

    return XMLmsg;
}

/*
<messages>
  <message>
    <type>TYPE_1</type>
    <payload>MESSAGE_1_PAYLOAD</payload>
  </message>
  <message>
    <type>TYPE_2</type>
    <payload>MESSAGE_2_PAYLOAD</payload>
  </message>
</messages>
*/


/*
for(auto&& field: docString.child("messages").children("message")){
        std::cout << "Received message: " << std::endl;
        std::cout << "\tType: \t\t" << field.child("type").text().as_string() << std::endl;
        std::cout << "\tPayload: \t" << field.child("payload").text().as_string() << std::endl;
         
        type=field.child("type").text().as_string();
        payload=field.child("payload").text().as_string();

        if(type=="upAndDown"){
            shmem.data->up=payload=="plus";
            shmem.data->down=payload=="minus";
        }else if(type=="startAndStop"){
            shmem.data->start_training=payload=="start";
        }else if(type=="pid"){
            shmem.data->pid=payload=="on";
        }
    }
*/
