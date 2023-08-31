#include "singletonSM.h"


SingletonSM::SingletonSM(){}

void SingletonSM::init_SM(){
    shmem = new shared_memory();
    shmem->init();
}

void SingletonSM::detach_shared_memory(){
    shmem->detach_shared_memory();
}

shared_memory* SingletonSM::get_SM(){
    if (instancePtr == NULL){
        return nullptr;
    }else{
        return shmem;
    }
}
