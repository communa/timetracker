#ifndef MYTIMER_H
#define MYTIMER_H

#include <QTimer>

class Timer : public QObject
{
    Q_OBJECT

public:
    Timer();
    int time = 0;
    bool isStarted = false;

    void start();
    void stop();
    void time_write();
    void time_read();

protected:
    QTimer *timer;

protected slots:
    void TimerSlot();
};

#endif // MYTIME
