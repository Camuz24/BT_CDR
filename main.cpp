#include "headers.h"

#define DEFAULT_LOOP_TIME_NS 1000000L
#define MIN_CADENCE 25

//using std::string;
using namespace std;
powerController FEScontrol;
ConcurrentBtle* btle;
//motor Motor;
pidController Pid;
cadenceblock Cadence;
BcmEncoder Encoder;

float targetPower = 0;
int dc = 0;

// inizializzo variabili per calcolare la cadenza
float angle = 0;
float angle_old = -1;
float cadence;
double filtered_cadence = 0;
double filtered_cadence_old = 0;
vector<double> cadence_vector;
double sum_cadence_vector = 0;
double mean_cadence = 0;
double control_angle = -1;

// data on power and stimulation
float averageTotalPower = 0;

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
    //Motor.stopMotor();
    
    SingletonSM* singletonSM = SingletonSM::getInstance();
    singletonSM->detach_shared_memory();
    exit(signum);
    
}

void powerControl()
{
    qint16 totalPower;
    float powerPidOutput = 0;

    // inizializzo variabili per calcolare la cadenza
    float angle = Encoder.getAngle();
    float angle_old = -1;
    float cadence;
    double filtered_cadence = 0;
    double filtered_cadence_old = 0;

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

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    while(1)
    {
        Pid.on();

        t_next = addition(t_next, t_period); // update t_next (needed for usleep at the end)clock_gettime ( CLOCK_MONOTONIC, &t_now);

        // controllo PID motore
        if(loop_count%500 == 0)
        {
            dc = dc + (int)Pid.PID(MIN_CADENCE, filtered_cadence);
            //dc = dc + (int)Pid.PID(targetCadence, filtered_cadence);
            if(dc < 30) dc = 30;
            //Motor.setPwmMotor(dc);
        }

        if(loop_count%100 == 0)     // con 100 sto andando a 0.1Hz (entro nell'if 10 volte al secondo)
        {          
            if(angle_old == -1) //prima iterazione
            {
                angle_old = angle; //non viene calcolata la cadenza
                control_angle = angle;
            }

            else 
            {
                angle = Encoder.getAngle(); //leggo l'angolo dall'encoder
                cadence = (double)Cadence.computeCadence(angle_old, angle);
                filtered_cadence = Cadence.filterCadence(cadence, filtered_cadence_old);
                filtered_cadence_old = filtered_cadence;
                cadence_vector.push_back(filtered_cadence);
                cout << "Cadenza attuale: " << fixed << setprecision(2) << filtered_cadence << endl;
                angle_old = angle;

                // compute the mean cadence over 360° cycle
                if(control_angle == angle + 360)
                {
                    for (int element : cadence_vector) {
                        sum_cadence_vector += element;
                    }
                    mean_cadence = sum_cadence_vector / cadence_vector.size();
                    cadence_vector.clear();
                    control_angle = angle;
                }            

                if(mean_cadence < MIN_CADENCE)
                {
                    //TODO: disattivare stimolazione
                    Pid.on();             
                }
                else 
                {
                    //TODO: attivare stimolazione
                    Pid.off();
                }            
            }
        }  

        if(btle->newLeftData && btle->newRightData)
        { 
            totalPower = btle->instantaneousLeftPower + btle->instantaneousRightPower;
            cout << "Total power (left + right) over one cycle:" << totalPower << endl;
            powerPidOutput = FEScontrol.PID(targetPower, totalPower);
            cout << "Pid coefficient:" << powerPidOutput << endl;

            powerControlFile << endl << powerPidOutput << "," << totalPower << "," << shmem->data->gear;

            btle->newLeftData = false;
            btle->newRightData = false;
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

    chatServer = new ChatServer();
    chatServer->startServer(address);

    cout << "Set a target total power:" << endl;
    cin >> targetPower;
    shmem->data->single_target_power = (double)targetPower;


    // // need to get the target cadence from the GUI/user
    // cout << "Set Target Cadence:";
    // cin >> targetCadence;
    // cout << "Target Cadence = " << targetCadence << "\n";

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
                                 << "Gear" << endl;
    }
    else if (!powerControlFile.is_open()) {
        cout << "Error: Unable to open the file " << fileName << endl;
    }

    //ConcurrentBtle* btle;
    btle = new ConcurrentBtle();

    thread Thread(powerControl);
    FEScontrol.PidON();

    //TODO: crea due classi separate per polar e pedale sinistro
    
    signal(SIGINT, myInterruptHandler);

    return a.exec();
}
