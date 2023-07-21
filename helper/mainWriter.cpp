#include <iostream>
using namespace std;
#include "shared_memory.h"
#include <signal.h>

shared_memory shmem;

void myInterruptHandler (int signum) {

    printf ("ctrl-c has been pressed. Programs will be terminated in sequence.\n");
    
    shmem.detach_shared_memory();
    exit(signum);
    
}

void init_shared_memory(){

    if(shmem.init())
    {
        std::cout << "User interface shared memory initialized with key " << std::hex << shmem.get_shared_memory_key() <<  std::endl;
    }

    else{

        std::cout << "User Interface shared memory initialization has been failed\n";
        shmem.detach_shared_memory();
    }

}

int main() {
    std::cout << "Hello, from writer!\n";
    init_shared_memory();
    signal(SIGINT, myInterruptHandler);
    while (true) {
        int user_input;
        
        // Ask for input
        cout << "cur %: ";
        
        // Attempt to read user input
        if (!(cin >> user_input)) {
            // If reading failed (non-numeric value), break out of the loop
            break;
        }
        
        // Process the user input
        shmem.data->current_percentage = user_input;
    }

    cout << "Loop ended. Exiting the program." << endl;
    return 0;
}