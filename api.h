#ifndef API_H
#define API_H

#include <QByteArray>

class Api
{
    public:
        // std::string API_URL = "https://app.communa.network";
        std::string API_URL = "http://0.0.0.0:4000";

        void authStatus();
        void authLogin();
        QByteArray getNonce();
        QByteArray authQrCode(QString nonce);
        void authJwtRefresh();

        void activitySearch();

        void timeCreate();
};

#endif // API_H
