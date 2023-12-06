#include "auth.h"
#include "api.h"
#include <QPixmap>
#include <QString>
#include <QDebug>

QPixmap Auth::buildQrCode()
{
    Api api;
    QPixmap pm;

    QByteArray dataNonce = api.getNonce();
    QByteArray dataQr = api.authQrCode(dataNonce);

    qDebug() << "OK: "<< dataNonce;

    pm.loadFromData(dataQr);

    return pm;
}
