#include <QJsonDocument>
#include <QDebug>

#include "HttpRequestArgs.h"

class HttpRequestArgsData : public QSharedData
{
public:
    HttpRequestArgsData()
        : method(-1)
    {}
    HttpRequestArgsData(const HttpRequestArgsData &other)
        : QSharedData(other)
        , method(other.method)
        , url(other.url)
        , token(other.token)
        , json(other.json)
    {}
    ~HttpRequestArgsData() {}

    int method;
    QUrl url;
    QString token;
    QJsonDocument json;
};

HttpRequestArgs::HttpRequestArgs()
    : d(new HttpRequestArgsData)
{
}

HttpRequestArgs::HttpRequestArgs(int method, const QUrl &url,
                                 const QString &token, const QJsonDocument &json)
    : d(new HttpRequestArgsData)
{
    d->method = method;
    d->url = url;
    d->token = token;
    d->json = json;
}

HttpRequestArgs::HttpRequestArgs(const HttpRequestArgs &other)
    : d(other.d)
{
}

HttpRequestArgs &HttpRequestArgs::operator=(const HttpRequestArgs &other)
{
    if (this != &other) d.operator=(other.d);
    return *this;
}

HttpRequestArgs::~HttpRequestArgs()
{
}

bool HttpRequestArgs::operator==(const HttpRequestArgs &other) const
{
    return (d->method == other.d->method &&
            d->url == other.d->url &&
            d->token == other.d->token &&
            d->json == other.d->json);
}

bool HttpRequestArgs::operator!=(const HttpRequestArgs &other) const
{
    return (d->method != other.d->method ||
            d->url != other.d->url ||
            d->token != other.d->token ||
            d->json != other.d->json);
}

bool HttpRequestArgs::isValid() const
{
    return (d->method >= 0 && d->url.isValid());
}

int HttpRequestArgs::method() const
{
    return d->method;
}

QUrl HttpRequestArgs::url() const
{
    return d->url;
}

QString HttpRequestArgs::token() const
{
    return d->token;
}

QJsonDocument HttpRequestArgs::json() const
{
    return d->json;
}

void HttpRequestArgs::dump(QDebug &dbg) const
{
    QDebugStateSaver saver(dbg);
    dbg.noquote();
    dbg << "method" << d->method
        << "url"    << d->url
        << "token"  << d->token
        << "json"   << d->json;
}
