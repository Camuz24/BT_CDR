#ifndef PIDCONTROLLER_H
#define PIDCONTROLLER_H


class pidController
{
    private:
        double kp=0.2;
        double ki=0.001;
        double kd=0.6;

        bool pidEnable=0;

        double max=5;
        double min=-5;
        double error=0;
        double integral=0;
        double oldError=0;

        double output=0;
    public:
        pidController();
        double PID(double target, double cadence);
        void off();
        void on();

};

#endif // PIDCONTROLLER_H
