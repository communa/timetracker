#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslSocket>
#include <QSslError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

#include "HttpRequest.h"

HttpRequest::HttpRequest(QObject *parent)
    : QObject(parent)
    , request_state(Idle)
{
    qRegisterMetaType<HttpRequestArgs>("HttpRequestArgs");

#ifndef QT_NO_SSL
    ssl_conf.setDefaultConfiguration(QSslConfiguration::defaultConfiguration());
    ssl_conf.setProtocol(QSsl::TlsV1_2OrLater);
    ssl_conf.setPeerVerifyMode(QSslSocket::VerifyNone);
#endif
}

HttpRequest::~HttpRequest()
{
    if (last_reply) delete last_reply;
}

QStringList HttpRequest::urlSchemes() const
{
    return net_mgr.supportedSchemes();
}

// static
bool HttpRequest::sslAvailable()
{
#ifdef QT_NO_SSL
    return false;
#else
    return (QSslSocket::supportsSsl() &&
            !QSslSocket::sslLibraryBuildVersionString().isEmpty() &&
            !QSslSocket::sslLibraryVersionString().isEmpty());
#endif
}

// static
QString HttpRequest::sslVerCompile()
{
#ifdef QT_NO_SSL
    return QString();
#else
    return QSslSocket::sslLibraryBuildVersionString();
#endif
}

// static
QString HttpRequest::sslVerRuntime()
{
#ifdef QT_NO_SSL
    return QString();
#else
    return QSslSocket::sslLibraryVersionString();
#endif
}

QUrl HttpRequest::url() const
{
    return request_url;
}

void HttpRequest::setUrl(const QUrl &url)
{
    if (url != request_url) {
        request_url = url;
        emit urlChanged();
    }
}

int HttpRequest::status() const
{
    return request_state;
}

void HttpRequest::setStatus(Status state)
{
    if (state != request_state) {
        request_state = state;
        emit statusChanged();
    }
}

void HttpRequest::send(int method, const QUrl &url)
{
    sendData(method, url, QString(), QJsonDocument(QJsonObject()));
}

void HttpRequest::sendObject(int method, const QUrl &url, const QString &token, const QJsonObject &json)
{
    sendData(method, url, token, QJsonDocument(json));
}

void HttpRequest::sendArray(int method, const QUrl &url, const QString &token, const QJsonArray &json)
{
    sendData(method, url, token, QJsonDocument(json));
}

void HttpRequest::sendData(int method, const QUrl &url, const QString &token, const QJsonDocument &json)
{
    //qDebug() << Q_FUNC_INFO << method << url << token << json;

    if (method < MethodGet || method > MethodDelete) {
        qWarning() << Q_FUNC_INFO << "Invalid method specified" << method;
        return;
    }
    if (!url.isValid() || url.host().isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Invalid URL specified" << url;
        return;
    }
    bool ssl = url.scheme().toLower().endsWith('s');
    if (ssl && !sslAvailable()) {
        qWarning() << Q_FUNC_INFO << "No SSL support for" << url.scheme();
        return;
    }
    if (last_reply) {
        request_queue.enqueue(HttpRequestArgs(method, url, token, json));
        return;
    }

    QNetworkRequest req(url);
    if (ssl) req.setSslConfiguration(ssl_conf);

    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    if (!token.isEmpty())
        req.setRawHeader(QByteArrayLiteral("Authorization"), token.toLatin1());

    switch (method) {
    case MethodPost: {
        QByteArray data = json.toJson(QJsonDocument::Compact);
        req.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
        req.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/json"));
        last_reply = net_mgr.post(req, data);
    }   break;
    case MethodPut: {
        QByteArray data = json.toJson(QJsonDocument::Compact);
        req.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
        req.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/json"));
        last_reply = net_mgr.put(req, data);
    }   break;
    case MethodDelete:
        last_reply = net_mgr.deleteResource(req);
        break;
    case MethodGet:
    default:
        last_reply = net_mgr.get(req);
    }

    setUrl(url);
    setListenReply(true);
    setStatus(Busy);
}

bool HttpRequest::sendData(const HttpRequestArgs &data)
{
    if (!data.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid queue data";
        return false;
    }
    sendData(data.method(), data.url(), data.token(), data.json());
    return true;
}

void HttpRequest::setListenReply(bool on)
{
    reply_data.clear();

    if (!last_reply) return;
    if (on) {
        connect(last_reply, &QIODevice::readyRead, this, &HttpRequest::receiveData);
        connect(last_reply, &QNetworkReply::finished, this, &HttpRequest::replyFinished);
        connect(last_reply, &QNetworkReply::destroyed, this, &HttpRequest::replyDestroyed);
#ifndef QT_NO_SSL
        connect(last_reply, &QNetworkReply::sslErrors, this, [this](const QList<QSslError> &errors) {
            last_reply->ignoreSslErrors(errors);
        });
#endif
    } else {
        last_reply->disconnect(this);
        last_reply->abort();
        last_reply->deleteLater();
    }
}

void HttpRequest::receiveData()
{
    Q_ASSERT(last_reply);

    reply_data.append(last_reply->readAll());
}

void HttpRequest::replyFinished()
{
    Q_ASSERT(last_reply);

    last_reply->deleteLater();
    if (last_reply->error() != QNetworkReply::NoError) {
        QVariant code = last_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        int status = code.isValid() ? code.toInt() : last_reply->error();
        if (status / 100 != 5) {
            setStatus(Error);
            emit recvError(QString("[%1] %2").arg(status).arg(last_reply->errorString()));
            return;
        }
    } else if (last_reply->hasRawHeader(QByteArrayLiteral("Authorization")) &&
               last_reply->hasRawHeader(QByteArrayLiteral("Refresh-Token"))) {
        emit recvTokens(QString::fromLatin1(last_reply->rawHeader(QByteArrayLiteral("Authorization"))),
                        QString::fromLatin1(last_reply->rawHeader(QByteArrayLiteral("Refresh-Token"))));
    }

    if (reply_data.size()) {
        const QJsonDocument jdoc = QJsonDocument::fromJson(reply_data);
        if (jdoc.isObject()) {
            emit recvObject(last_reply->url(), jdoc.object());
        } else if (jdoc.isArray()) {
            emit recvArray(last_reply->url(), jdoc.array());
        }
    }

    setStatus(Ready);
}

void HttpRequest::replyDestroyed(QObject *reply)
{
    Q_UNUSED(reply);

    if (!request_queue.isEmpty() && !sendData(request_queue.dequeue()))
        request_queue.clear();
}

void HttpRequest::cancel()
{
    request_queue.clear();

    setListenReply(false);
    setStatus(Idle);
}
