#include "manager.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <dirent.h>
#include <vector>
#include <unistd.h> 
#include <thread>
#include "messageTypeDefinitions.h"
#include <cstdlib> // for std::rand() and std::srand()
#include <ctime>   // for std::time()
#include "pugixml-1.13/src/pugixml.hpp"

using namespace pugi;
using std::string;
using std::to_string;


manager::manager()
{
    stopThread = false;
    stopSend = false;
    std::srand(static_cast<unsigned>(std::time(0)));
}

manager::~manager()
{

}


void manager::writeOnSM(const QString &sender, const QString &message){
    stopSend = true;
    std::cout << "stop send" << std::endl;
    std::cout << message.toStdString().c_str() << std::endl;
    xml_document doc;
    xml_parse_result parsedMessage = doc.load_string(message.toStdString().c_str());
    string type;
    string payload;

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    if(!parsedMessage){
        //TODO: send message to client telling that message is not parseable
        std::cout << "Parsing error";
        return;
    }

    std::cout << "Received message: " << std::endl;
    xml_node tools = doc.child("message");

    for (xml_node_iterator it = tools.begin(); it != tools.end(); ++it){
        for (xml_attribute_iterator ait = it->attributes_begin();
                ait != it->attributes_end(); ++ait){

            //std::cout << " " << ait->name() << "=" << ait->value();

            std::stringstream ssName;
            ssName << ait->name();
            std::string strName = ssName.str();

            std::stringstream ssValue;
            ssValue << ait->value();
            std::string strValue = ssValue.str();

            if(strName == "type"){
                type=strValue;
            }
            if(strName == "payload"){
                payload=strValue;
            }
        }

        //std::cout << std::endl;

        std::cout << "\tType: \t\t" << type << std::endl;
        std::cout << "\tPayload: \t" << payload << std::endl;
        if(type=="upAndDown"){
            shmem->data->up = payload=="plus";
            shmem->data->down = payload=="minus";
        }else if(type=="startAndStop"){
            shmem->data->start_training = payload=="start";
        }else if(type=="pid"){
            shmem->data->pid=payload=="1";
        }else if(type=="calibrationCurrent"){
            //TODO
        }else if(type=="calibrationFrequency"){
            //TODO
        }else if(type=="calibrationPulseWidth"){
            //TODO
        }else if(type=="calibrationMuscle"){
            //TODO
        }else if(type=="calibrationStartAndStop"){
            shmem->data->start_training = payload=="start";
        }else if(type=="calibrationRightLeft"){
            //TODO
        }else if(type=="request"){
            std::string directory = "../../data/"; //TODO: change here accordingly of where the executable file will be!!!
            std::string extension = ".json";
            if(payload== "users"){
                std::string fileNames;

                DIR* dir;
                struct dirent* ent;

                if ((dir = opendir(directory.c_str())) != nullptr) {
                    while ((ent = readdir(dir)) != nullptr) {
                        std::string fileName = ent->d_name;
                        size_t dotPos = fileName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            std::string extension = fileName.substr(dotPos);
                            if (extension == ".json") {
                                // Extract the file name without extension
                                std::string nameWithoutExtension = fileName.substr(0, dotPos);

                                if (!fileNames.empty()) {
                                    fileNames += ",";
                                }
                                fileNames += nameWithoutExtension;
                            }
                        }
                    }
                    closedir(dir);
                    std::cout << "JSON Files in " << directory << ": " << fileNames << std::endl;
                    std::vector<string> types;
                    std::vector<string> payloads;
                    types={"request"};
                    payloads={fileNames};
                    string xmlMessage = buildXMLMessage(types, payloads);
                    emit sendToClient(QString::fromStdString(xmlMessage));
                } else {
                    std::cout << "Error opening directory " << directory << std::endl;
                }
            }else{
                std::fstream userFile;
                userFile.open(directory + payload + extension, std::ios::in);
                if(!userFile){
                    std::cout << "Requested inextistent file " << payload << std::endl;
                }else{
                    std::ifstream t(directory + payload + extension);
                    std::stringstream buffer;
                    buffer << t.rdbuf();
                    std::vector<string> types;
                    std::vector<string> payloads;
                    types={"request"};
                    payloads={buffer.str()};
                    string xmlMessage = buildXMLMessage(types, payloads);
                    emit sendToClient(QString::fromStdString(xmlMessage));
                }
            }
        }
    }
    stopSend = false;
    std::cout << "start send" << std::endl;
}

/**
 * Read periodically from shared memory and send message to client view
*/
void manager::threadReadFromSM(){
    float hz = 15;
    int counter_motivational = 0;
    int motivational_msg_delay = 7; //delay between each motivational msg
    float hz_high_priority = 30;
    int wait_cycles = (int) (hz_high_priority/hz); // send low priority message every wait_cycles cycles
    int counter_low_priority = 0;
    std::vector<string> types;
    std::vector<string> payloads;

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    std::cout << "Updating client view at " << hz << " Hz" << std::endl;
    usleep(1 * 1e6);
    while(!stopThread){

        if(!stopSend){
            
            if(counter_low_priority == wait_cycles){
                counter_low_priority = 0;
                types = {
                START_STOP, PID, CURRENT_CADENCE, ANGLE_ENCODER, CHECK_STIM1, CHECK_STIM2,
                CHECK_PEDAL_LEFT, CHECK_PEDAL_RIGHT, CHECK_CARDIO, TRG_CAD,
                PID_PERCENTAGE, CURRENT_PERCENTAGE,
                QUAD_L, GLU_L, HAM_L, GAS_L,
                QUAD_R, GLU_R, HAM_R, GAS_R,
                HEART_RATE, STIMULATOR_FREQUENCY, STIMULATOR_PULSEWIDTH, CURRENT_OR_TARGET
                };
            
                payloads = {to_string(shmem->data->start_training),
                to_string(shmem->data->pid), to_string((int) shmem->data->current_cadence),
                to_string((int) shmem->data->angle_encoder), to_string(shmem->data->check_stim1),
                to_string(shmem->data->check_stim2), to_string(shmem->data->check_pedal_left),
                to_string(shmem->data->check_pedal_right), to_string(shmem->data->check_cardio),
                to_string((int) shmem->data->trg_cad), to_string(shmem->data->pid_percentage),
                to_string(shmem->data->current_percentage), to_string((int) shmem->data->theorCurrentsL[0]),
                to_string((int) shmem->data->theorCurrentsL[1]), to_string((int) shmem->data->theorCurrentsL[2]),
                to_string((int) shmem->data->theorCurrentsL[3]), to_string((int) shmem->data->theorCurrentsR[0]),
                to_string((int) shmem->data->theorCurrentsR[1]), to_string((int) shmem->data->theorCurrentsR[2]),
                to_string((int) shmem->data->theorCurrentsR[3]), to_string((int) shmem->data->heart_rate),
                to_string(shmem->data->stimulation_frequency), to_string(shmem->data->pulse_width_from_gui)};

                if(shmem->data->pid){
                    payloads.push_back(to_string((int) shmem->data->trg_cad));
                }else{
                    payloads.push_back(
                        to_string((int) (shmem->data->theorCurrentsL[0] > shmem->data->theorCurrentsR[0] ? shmem->data->theorCurrentsL[0] : shmem->data->theorCurrentsR[0])));
                }
            }else{
                types = {ANGLE_ENCODER};
                payloads = {to_string((int) shmem->data->angle_encoder)};

            }
            counter_low_priority++;
            

            if(counter_motivational % (int)(hz_high_priority*motivational_msg_delay) == 0){
                types.push_back(ERROR);
                payloads.push_back(getMotivation());
                counter_motivational = 0;
            }
            counter_motivational++;

            string xmlMessage = buildXMLMessage(types, payloads);
            emit sendToClient(QString::fromStdString(xmlMessage));
        }
        usleep((int) (1.0/hz_high_priority * 1e6));
    }

    std::cout << "Thread terminated" << std::endl;
}

void manager::startThread() {
        
    // Create a new thread and pass a member function as the entry point
    std::thread t(&manager::threadReadFromSM, this);

    // Optionally, you can detach the thread if you don't need to join it later
    t.detach();
    
    }

string manager::buildXMLMessage(const std::vector<string>& types, const std::vector<string>& payloads) {
    if (types.size() != payloads.size()) {
        std::cout << "Types and payloads sizes are different. Cannot send message"; 
        return "";
    }

    std::string XMLmsg = "<m>";
    for (size_t i = 0; i < types.size(); ++i) {
        XMLmsg += "<t>" + types[i] + "</t>";
        XMLmsg += "<p>" + payloads[i] + "</p>";
    }
    XMLmsg += "</m>";

    return XMLmsg;
}

string manager::getMotivation(){
    std::vector<std::string> sentences = {
        "La perseveranza conquista tutto.",
        "Vai oltre i tuoi limiti.",
        "Niente è impossibile se ci credi.",
        "Ogni passo conta.",
        "Vinci la tua mente, vinci il gioco.",
        "Allenamento duro, vittoria facile.",
        "Il successo è un'abitudine.",
        "La fatica si trasforma in forza.",
        "Fai un passo alla volta.",
        "Migliora ogni giorno.",
        "Le sfide ti rendono più forte.",
        "Non mollare mai!",
        "Il dolore è temporaneo, la vittoria è eterna.",
        "Sogna in grande, allenati duro.",
        "Vola alto, raggiungi le vette.",
        "Vinci con la tua dedizione.",
        "Il lavoro di squadra è la chiave.",
        "La disciplina ti guida al successo.",
        "Impara dagli errori, cresci.",
        "Tu sei il capitano della tua nave.",
        "La determinazione ti rende imbattibile.",
        "Il successo è una scelta.",
        "Non smettere mai di credere in te stesso.",
        "La tua mentalità è la tua forza.",
        "La resilienza è la tua arma segreta.",
        "L'allenamento è l'investimento nella tua vittoria.",
        "Affronta le sfide con coraggio.",
        "Ogni giorno è una nuova opportunità per eccellere.",
        "Sii il migliore che puoi essere.",
        "Il tuo impegno è il tuo marchio di fabbrica.",
        "Sii costante nella tua dedizione.",
        "Il successo è una questione di costanza.",
        "Non arrenderti mai, i risultati arriveranno.",
        "Lotta per i tuoi obiettivi.",
        "Sii audace nella tua determinazione.",
        "Le difficoltà costruiscono il tuo carattere.",
        "Mantieni il focus sui tuoi obiettivi.",
        "Ogni allenamento ti avvicina alla vittoria.",
        "Sii il campione del tuo destino.",
        "Mantieni la testa alta, anche nelle sconfitte.",
        "La passione è la tua spinta più grande.",
        "Mira alle stelle, raggiungerai la luna.",
        "Ogni sacrificio vale la pena.",
        "Sei un atleta, sei un vincente.",
        "Le tue azioni parlano più delle tue parole.",
        "Affronta le avversità con grinta.",
        "L'allenamento è la tua chiave del successo.",
        "Sii un leader nel campo.",
        "Il duro lavoro batte il talento.",
        "La tua resilienza è la tua arma segreta.",
        "Sii un modello per gli altri.",
        "Sii il protagonista della tua storia.",
        "Sogna in grande, sii straordinario.",
        "Vinci con il tuo cuore e la tua mente.",
        "Mai accontentarsi del secondo posto.",
        "Lascia un segno nella storia dello sport.",
        "Sii il miglior rivale di te stesso.",
        "La tua passione ti spinge oltre i limiti.",
        "Le vittorie iniziano nella tua mente.",
        "La tua dedizione è la tua arma segreta.",
        "Il percorso è difficile, ma la meta vale la pena.",
        "Sii ambizioso, sii grande.",
        "L'obiettivo è chiaro: la vittoria.",
        "Sii implacabile nella tua determinazione.",
        "Affronta le sfide con fiducia.",
        "Mantieni la calma, vai avanti.",
        "Le tue azioni parlano per te.",
        "Lotta, vinci, ripeti.",
        "Non hai limiti se credi in te stesso.",
        "Sii grato per ogni opportunità di migliorare.",
        "Sii un guerriero del tuo destino.",
        "La tua forza interiore ti rende invincibile.",
        "Ogni giorno è una nuova possibilità di crescita.",
        "Sii il maestro del tuo destino.",
        "Raggiungi nuove vette ogni giorno.",
        "Il tuo impegno è la tua moneta più preziosa.",
        "Vinci con l'amore per il gioco.",
        "Sii un leader di te stesso.",
        "La tua dedizione è la tua firma.",
        "Il tuo coraggio ti rende unico.",
        "Sii un guerriero nel campo di battaglia.",
        "La tua costanza ti porta alla gloria.",
        "Mantieni la fede nella tua capacità.",
        "Ogni sforzo conta verso la vittoria.",
        "Sii un ispiratore per gli altri atleti.",
        "Le sconfitte ti rendono più forte.",
        "Sii il pilota del tuo destino.",
        "La tua passione ti spinge oltre i confini.",
        "Sii audace, sii indomabile.",
        "Il duro lavoro ti conduce alla grandezza.",
        "Le tue azioni riflettono il tuo carattere.",
        "Non smettere mai di sfidarti.",
        "Sii un vincitore nella vita e nello sport.",
        "La tua dedizione è la tua bussola.",
        "Vivi per il gioco, vinci per il gioco.",
        "Ogni giorno è una nuova possibilità di eccellere.",
        "Sii un campione di umiltà e determinazione.",
        "Il tuo talento si sviluppa con la pratica costante.",
        "La tua resilienza supera le avversità.",
        "Sii il protagonista della tua leggenda."
    };

    int randomIndex = std::rand() % sentences.size();
    return sentences[randomIndex];
}
