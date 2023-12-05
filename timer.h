#ifndef MYTIMER_H
#define MYTIMER_H

#include <QTimer>
#include <QFile>

class Timer : public QObject
{
    Q_OBJECT

    public:
        Timer();
        int time = 0;
        bool isStarted = false;

        void time_write();
        void time_read();

    protected:
        QString filePath;

};

#endif // MYTIME
