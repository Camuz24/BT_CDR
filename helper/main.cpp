#include <iostream>
#include "shared_memory.h"

shared_memory shmem;

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

int main(int, char**){
    std::cout << "Hello, from receiver!\n";
    init_shared_memory();
    bool input=0;
    bool old=1;
    bool old_start_training=0;
    

    while (1)
    {
        if(shmem.data->up){
            std::cout<<"up\n";
            shmem.data->up=0;
        }
        if(shmem.data->down){
            std::cout<<"down\n";
            shmem.data->down=0;
        }
        if(shmem.data->start_training && !old_start_training){
            std::cout<<"Start\n";
            old_start_training = shmem.data->start_training;
        }
        if(!shmem.data->start_training && old_start_training){
            std::cout<<"Stop\n";
            old_start_training = shmem.data->start_training;
        }

        
        
    }

}
