#ifndef API_H
#define API_H

#include <QByteArray>

class Api
{
    public:
        void authStatus();
        void authLogin();
        QByteArray authQrCode();
        void authJwtRefresh();

        void activitySearch();

        void timeCreate();
};

#endif // API_H
