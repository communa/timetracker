#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QSslConfiguration>
#include <QQueue>
#include <QPointer>
#include <QByteArray>

#include "HttpRequestArgs.h"

class QJsonDocument;
class QJsonObject;
class QJsonArray;

class HttpRequest : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(HttpRequest)

    Q_PROPERTY(QStringList urlSchemes READ urlSchemes    CONSTANT FINAL)
    Q_PROPERTY(bool      sslAvailable READ sslAvailable  CONSTANT FINAL)
    Q_PROPERTY(QString  sslVerCompile READ sslVerCompile CONSTANT FINAL)
    Q_PROPERTY(QString  sslVerRuntime READ sslVerRuntime CONSTANT FINAL)

    Q_PROPERTY(QUrl   url READ url    NOTIFY urlChanged FINAL)
    Q_PROPERTY(int status READ status NOTIFY statusChanged FINAL)

public:
    explicit HttpRequest(QObject *parent = nullptr);
    ~HttpRequest();

    enum Method { MethodGet, MethodPost, MethodPut, MethodDelete };
    Q_ENUM(Method)

    enum Status { Idle, Ready, Busy, Error };
    Q_ENUM(Status)

    QStringList urlSchemes() const;
    static bool sslAvailable();
    static QString sslVerCompile();
    static QString sslVerRuntime();

    QUrl url() const;
    int status() const;

    Q_INVOKABLE void send(int method, const QUrl &url);
    Q_INVOKABLE void sendObject(int method, const QUrl &url,
                                const QString &token, const QJsonObject &json);
    Q_INVOKABLE void sendArray(int method, const QUrl &url,
                               const QString &token, const QJsonArray &json);

public slots:
    void cancel();

signals:
    void urlChanged();
    void statusChanged();
    void recvTokens(const QString &access, const QString &refresh);
    void recvArray(const QUrl &url, const QJsonArray &json);
    void recvObject(const QUrl &url, const QJsonObject &json);
    void recvError(const QString &text);

private:
    void setUrl(const QUrl &url);
    void setStatus(Status state);
    void sendData(int method, const QUrl &url, const QString &token, const QJsonDocument &json);
    bool sendData(const HttpRequestArgs &data);
    void setListenReply(bool on);
    void receiveData();
    void replyFinished();
    void replyDestroyed(QObject *reply);

    QNetworkAccessManager net_mgr;
    QSslConfiguration ssl_conf;
    QQueue<HttpRequestArgs> request_queue;
    QUrl request_url;
    int request_state;

    QPointer<QNetworkReply> last_reply;
    QByteArray reply_data;
};

#endif // HTTPREQUEST_H
