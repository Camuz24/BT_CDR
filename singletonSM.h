#ifndef SINGLETONSM_H
#define SINGLETONSM_H

#include <QtCore/qobject.h>
#include "shared_memory.h"

class SingletonSM : public QObject
{
    Q_OBJECT

private:
    static SingletonSM* instancePtr;

    SingletonSM();

public:
    SingletonSM(const SingletonSM& obj) = delete;
    shared_memory* shmem;

    static SingletonSM* getInstance(){
        if (instancePtr == NULL)
        {
        instancePtr = new SingletonSM();
        return instancePtr;
        }
        else
        {
        return instancePtr;
        }
    }

    void init_SM();
    void detach_shared_memory();
    shared_memory* get_SM();
};

#endif 