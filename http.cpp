#include "http.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QByteArray>

QByteArray Http::request(const QString& url)
{
    QNetworkAccessManager nam;
    QEventLoop loop;
    QObject::connect(&nam,&QNetworkAccessManager::finished,&loop,&QEventLoop::quit);
    QNetworkReply *reply = nam.get(QNetworkRequest(url));
    loop.exec();

    QByteArray data = reply->readAll();

    delete reply;

    return data;
}
