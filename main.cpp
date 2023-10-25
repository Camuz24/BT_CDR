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

    int leftPower = 0;
    int rightPower = 0;
    float leftPowerPidOutput = 0;
    float rightPowerPidOutput = 0;

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

        if(loop_count%100 == 0)
        {
            if(shmem->data->pid)
            {
                if(shmem->data->new_left_data)
                { 
                    leftPower = btle->instantaneousLeftPower;
                    shmem->data->left_power = leftPower;
                    leftPowerPidOutput = FEScontrol.PID(SINGLE_POWER_TARGET, leftPower);
                    shmem->data->left_pid_coeff = (double)leftPowerPidOutput;
                    cout << "Left PID coefficient:" << leftPowerPidOutput << endl;
                    
                    // current_toSum = leftPowerPidOutput * (100 - fake_current);
                    // if(current_toSum + fake_current <= 100)  actual_fake_current =  current_toSum + fake_current;
                    // else actual_fake_current = 100;
                    
                    // fake_current = actual_fake_current;
                    //cout << "Fake current output: " << actual_fake_current << endl;

                    powerControlFile << endl << fixed << setprecision(2) << leftPowerPidOutput << ",\t" 
                                                                         << rightPowerPidOutput << ",\t"
                                                                         << leftPower << ",\t"
                                                                         << rightPower << ",\t" 
                                                                         << actual_fake_current << ",\t" 
                                                                         << shmem->data->gear << ",\t" 
                                                                         << cadence;

                    shmem->data->new_left_data = false;
                }

                if(shmem->data->new_right_data)
                { 
                    rightPower = btle->instantaneousRightPower;
                    shmem->data->right_power = rightPower;
                    rightPowerPidOutput = FEScontrol.PID(SINGLE_POWER_TARGET, rightPower);
                    shmem->data->right_pid_coeff = (double)rightPowerPidOutput;
                    cout << "Right PID coefficient:" << rightPowerPidOutput << endl;
                    
                    // current_toSum = rightPowerPidOutput * (100 - fake_current);
                    // if(current_toSum + fake_current <= 100)  actual_fake_current =  current_toSum + fake_current;
                    // else actual_fake_current = 100;
                    
                    // fake_current = actual_fake_current;
                    //cout << "Fake current output: " << actual_fake_current << endl;

                    powerControlFile << endl << fixed << setprecision(2) << leftPowerPidOutput << ",\t" 
                                                                         << rightPowerPidOutput << ",\t"
                                                                         << leftPower << ",\t"
                                                                         << rightPower << ",\t"  
                                                                         << shmem->data->gear << ",\t" 
                                                                         << cadence;
                    shmem->data->new_right_data = false;
                }
            }
            else
            {
               shmem->data->left_pid_coeff = 0;
               shmem->data->right_pid_coeff = 0;
            }    
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
        powerControlFile << endl << "Left PID coefficient" << "," 
                                 << "Right PID coefficient" << "," 
                                 << "Left Power" << ","
                                 << "RIght Power" << "," 
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
