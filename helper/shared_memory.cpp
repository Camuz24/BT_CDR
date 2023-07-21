#include "shared_memory.h"



shared_memory::shared_memory()
{
    key=ftok("/bin", DEFAULT_KEY_ID);
}

shared_memory::~shared_memory(){

    detach_shared_memory();
}

bool shared_memory::init(){

    // shmget returns an identifier in shmid
    shared_memory_packet temp;
    shmid = shmget(key, sizeof(temp), 0666|IPC_CREAT);

    // shmat to attach to shared memory
    data= (shared_memory_packet*) shmat(shmid, (void*)0,0);
    data -> stop=false;
    data -> use_ros=false;
    if(shmid > 0 && key >0) return true;
    else return false;


}

void shared_memory::detach_shared_memory(){

    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);
}
