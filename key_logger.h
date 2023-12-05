#ifndef KEY_LOGGER_H
#define KEY_LOGGER_H

#include <QApplication>
#include <QtNetwork/QNetworkAccessManager>

class KeyLogger
{
    public:
        int INACTIVE_SECONDS = 600;
        int mouse_movements = 0;
        int key_pressed = 0;

        void start(int activity_id);
        void stop(int activity_id);
        void clear(int activity_id);
};

#endif // KEY_LOGGER_H
