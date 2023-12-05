#ifndef HTTP_H
#define HTTP_H

#include <QApplication>
#include <QtNetwork/QNetworkAccessManager>

class Http
{
    public:
        std::string URL = "https://app.communa.network/api/";
        QByteArray request(const QString& url);
};

#endif // HTTP_H
