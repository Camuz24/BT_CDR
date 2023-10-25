#include "headers.h"

#define DEFAULT_LOOP_TIME_NS 1000000L
#define MIN_CADENCE 25
#define SINGLE_POWER_TARGET 15

//using std::string;
using namespace std;
powerController FEScontrol;
ConcurrentBtle* btle;

float totalTargetPower = 2*SINGLE_POWER_TARGET;
int dc = 0;

// inizializzo variabili per calcolare la cadenza
float angle = 0;
float prev_angle = -1;
float cadence = 0;
double filtered_cadence = 0;
double filtered_cadence_old = 0;
vector<double> cadence_vector;
double sum_cadence_vector = 0;
double mean_cadence = 0;
double control_angle = -1;

// data on power and stimulation
float averageTotalPower = 0;
float actual_fake_current = 0;
float fake_current = 30;
float current_toSum = 0;

ofstream powerControlFile;
string fileName;    // create a name for the file output

SingletonSM* SingletonSM::instancePtr = NULL;

timespec addition(timespec a, timespec b) {
    timespec r;

    if(a.tv_nsec + b.tv_nsec <= 999999999) {
        r.tv_nsec = a.tv_nsec + b.tv_nsec;
        r.tv_sec = a.tv_sec + b.tv_sec;
    }
    else {
        int c = (a.tv_nsec + b.tv_nsec)/1000000000;
        r.tv_nsec = a.tv_nsec + b.tv_nsec - 1000000000*c;
        r.tv_sec = a.tv_sec + b.tv_sec + c;
    }

    return r;
}

void myInterruptHandler (int signum) {

    printf ("ctrl-c has been pressed. Programs will be terminated in sequence.\n");
    FEScontrol.PidOFF();
    SingletonSM* singletonSM = SingletonSM::getInstance();
    singletonSM->detach_shared_memory();
    exit(signum);
    
}

// void *powerControl(void* a)

void powerControl()
{
    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    int totalPower = 0;
    float powerPidOutput = 0;

    struct timespec t_now;
    struct timespec t_next;
    struct timespec t_period;
    struct timespec t_wait;
    struct timespec pause_duration;

    unsigned long int loop_count = 0;

    // t_period defines duration of one "running" cycle (1 ms in this case)
    t_period.tv_sec = 0;
    t_period.tv_nsec = DEFAULT_LOOP_TIME_NS;

    // pause_duration defines duration of pause considered when a buttton is pressed (1 s in this case)

    clock_gettime( CLOCK_MONOTONIC, &t_next);

    while(1)
    {
        t_next = addition(t_next, t_period); // update t_next (needed for usleep at the end)clock_gettime ( CLOCK_MONOTONIC, &t_now);

        // if(loop_count%200 == 0)
        // {
        //     if(prev_angle == -1) //prima iterazione
        //     {
        //         prev_angle = angle; //non viene calcolata la cadenza
        //         control_angle = angle;
        //     }
        //     else 
        //     {
        //         angle = Encoder.getAngle(); //leggo l'angolo dall'encoder
        //         cadence = (double)Cadence.computeCadence(prev_angle, angle);
        //         filtered_cadence = Cadence.filterCadence(cadence, filtered_cadence_old);
        //         filtered_cadence_old = filtered_cadence;
        //         cadence_vector.push_back(filtered_cadence);
        //         cout << "La cadenza attauale è di " << fixed << setprecision(2) << filtered_cadence << endl;
        //         angle_old = angle;

        //         // compute the mean cadence over 360° cycle
        //         if(control_angle == angle + 360)
        //         {
        //             for (int element : cadence_vector) {
        //                 sum_cadence_vector += element;
        //             }
        //             mean_cadence = sum_cadence_vector / cadence_vector.size();
        //             cadence_vector.clear();
        //             control_angle = angle;
        //         }            

        //         if(mean_cadence < MIN_CADENCE)
        //         {
        //             //TODO: disattivare stimolazione
        //             //Pid.on();             
        //         }
        //         else 
        //         {   //Pid.off();
        //             //TODO: attivare stimolazione
        //             FEScontrol.PidON();
        //         }                      
        //     }
        // }

        if(loop_count%100 == 0)
        {
            if(shmem->data->pid)
            {
                if(shmem->data->new_left_data && shmem->data->new_right_data)
                { 
                    totalPower = shmem->data->average_left_power + shmem->data->average_right_power;
                    shmem->data->total_power = totalPower;
                    cout << "Total power (left + right) over one cycle:" << totalPower << endl;
                    powerPidOutput = FEScontrol.PID(totalTargetPower, totalPower);
                    shmem->data->pid_coeff = (double)powerPidOutput;
                    cout << "Pid coefficient:" << powerPidOutput << endl;
                    
                    // current_toSum = powerPidOutput * (100 - fake_current);
                    // if(current_toSum + fake_current <= 100)  actual_fake_current =  current_toSum + fake_current;
                    // else actual_fake_current = 100;
                    
                    // fake_current = actual_fake_current;
                    //cout << "Fake current output: " << actual_fake_current << endl;

                    powerControlFile << endl << fixed << setprecision(2) << powerPidOutput << ",\t" << totalPower << ",\t" << actual_fake_current << ",\t" << shmem->data->gear << ",\t" << cadence;

                    shmem->data->new_left_data = false;
                    shmem->data->new_right_data = false;
                }
            }
            else    shmem->data->pid_coeff = 0;

        }

        loop_count++;

        clock_nanosleep ( CLOCK_MONOTONIC, TIMER_ABSTIME, &t_next, nullptr ); // waits until time t_next is reached
        clock_gettime ( CLOCK_MONOTONIC, &t_now);
    }
}

int main(int argc, char *argv[]){
    std::cout << "Starting server...!\n";

    QCoreApplication a(argc, argv);

    QList<QBluetoothHostInfo> localAdapters;
    localAdapters = QBluetoothLocalDevice::allDevices();
    string myMAC = localAdapters.at(0).address().toString().toStdString();
    std::cout << myMAC << std::endl;

    QBluetoothAddress address(myMAC.c_str());
    
    ChatServer* chatServer;
    
    SingletonSM* singletonSM = SingletonSM::getInstance();
    singletonSM->init_SM();
    shared_memory* shmem = singletonSM->get_SM();

    // cout << "Set a single target power:" << endl;
    // cin >> targetPower;
    // shmem->data->single_target_power = (double)targetPower;

    shmem->data->single_target_power = SINGLE_POWER_TARGET;
    shmem->data->total_target_power = totalTargetPower;

    chatServer = new ChatServer();
    chatServer->startServer(address);

    time_t t = time(nullptr);
    struct tm * now = localtime( & t );
    char buffer [80];

    // Log directory
    fileName = strftime (buffer,80,"/home/pi/Desktop/BT_CDR/FilePowerControlCamilla/AcquiredData-%Y-%m-%d-%H-%M-%S.csv",now);
    powerControlFile.open(buffer);
    // write the file headers
    if(powerControlFile.is_open())
    {
        powerControlFile << endl << "PID coefficient" << "," 
                                 << "Total Power" << "," 
                                 << "Stimul Current" << ","
                                 << "Gear" << ","
                                 << "Cadence" << endl;
    }
    else if (!powerControlFile.is_open()) {
        cout << "Error: Unable to open the file " << fileName << endl;
    }

    btle = new ConcurrentBtle();

    //pthread_t thread;
    // pthread_create(&thread, NULL, threadFunction, nullptr);
    // pthread_join(thread, NULL);     // serve per aspettare il termine del thread prima di eseguire le righe di codice successive

    thread Thread(powerControl);
    FEScontrol.PidON();
    
    signal(SIGINT, myInterruptHandler);

    return a.exec();
}
