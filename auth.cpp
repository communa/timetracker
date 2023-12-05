#include "auth.h"
#include "api.h"
#include <QPixmap>

QPixmap Auth::buildQrCode()
{
    Api api;
    QPixmap pm;

    QByteArray data = api.authQrCode();

    pm.loadFromData(data);

    return pm;
}
