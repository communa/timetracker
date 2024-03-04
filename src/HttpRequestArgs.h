#ifndef HTTPREQUESTARGS_H
#define HTTPREQUESTARGS_H

#include <QSharedDataPointer>
#include <QMetaType>

class HttpRequestArgsData;

class HttpRequestArgs
{
public:
    HttpRequestArgs();
    HttpRequestArgs(int method, const QUrl &url,
                    const QString &token, const QJsonDocument &json);
    HttpRequestArgs(const HttpRequestArgs &);
    HttpRequestArgs &operator=(const HttpRequestArgs &);
    ~HttpRequestArgs();

    bool operator==(const HttpRequestArgs &other) const;
    bool operator!=(const HttpRequestArgs &other) const;

    bool isValid() const;
    int method() const;
    QUrl url() const;
    QString token() const;
    QJsonDocument json() const;

    friend inline QDebug& operator<<(QDebug &dbg, const HttpRequestArgs &from) {
        from.dump(dbg); return dbg; }

private:
    void dump(QDebug &dbg) const;

    QSharedDataPointer<HttpRequestArgsData> d;
};

Q_DECLARE_TYPEINFO(HttpRequestArgs, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(HttpRequestArgs)

#endif // HTTPREQUESTARGS_H
