#include "api.h"
#include "http.h"


// https://github.com/communa/backend/blob/dev/src/controller/AuthController.ts
// https://github.com/communa/backend/blob/dev/src/test/controller/AuthController.test.ts
void Api::authStatus()
{
}

void Api::authLogin()
{
}

QByteArray Api::authQrCode()
{
    QString url = "https://api.qrcode-monkey.com/tmp/04f3f143fc06c835acebdeb98f318146.svg?1701790393204";
    Http http;

    QByteArray data = http.request(url);

    return data;
}

void Api::authJwtRefresh()
{
}

// https://github.com/communa/backend/blob/dev/src/controller/ActivityController.ts
// https://github.com/communa/backend/blob/dev/src/test/controller/ActivityController.test.ts
void Api::activitySearch()
{
}

// https://github.com/communa/backend/blob/dev/src/controller/TimeController.ts
// https://github.com/communa/backend/blob/dev/src/test/controller/TimeController.test.ts
void Api::timeCreate()
{
}
