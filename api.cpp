#include "api.h"
#include "http.h"

void Api::authStatus()
{
    QString url = QString::fromStdString(this->API_URL) + "/api/auth/status";
    Http http;

    qDebug() << url;
}

QByteArray Api::getNonce()
{
    QString url = QString::fromStdString(this->API_URL) + "/api/auth/nonceQr";
    Http http;

    QByteArray data = http.request(url);

    return data;
}

QByteArray Api::authQrCode(QString nonce)
{
    QString url = QString::fromStdString(this->API_URL) + "/api/auth/qr/" + nonce;
    Http http;

    QByteArray data = http.request(url);

    return data;
}

void Api::authJwtRefresh()
{
    QString url = QString::fromStdString(this->API_URL) + "/api/auth/refresh";
    qDebug() << url;
}

void Api::activitySearch()
{
    QString url = QString::fromStdString(this->API_URL) + "/api/activity/search";
    qDebug() << url;
}

void Api::timeCreate()
{
    QString url = QString::fromStdString(this->API_URL) + "/api/time";
    qDebug() << url;
}
