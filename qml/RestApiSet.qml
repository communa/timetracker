pragma Singleton
import QtQuick 2.15

import CppCustomModules 1.0

Item {
    id: control
    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]

    // ATTENTION!
    // If you want to synchronize all records of the local Database with the Server from scratch, set
    // lastDataSync=0 under [RestApiSet] group in the 'time-tracket.ini' config file and restart the App.

    readonly property string restApiServer:   "https://app.communa.network"
    readonly property string authBaseUrl:     "/api/auth/timeTracker/"
    readonly property string authRefreshUrl:  "/api/auth/refresh"
    readonly property string openNonceUrl:    "/user/timetracker?nonce="
    readonly property string listActivityUrl: "/api/activity/search/freelancer"
    readonly property string activityTimeUrl: "/api/time/activity"
    readonly property string tokenFileName:   "JWT.json"

    readonly property int dataSyncDelay: 1 * 60 * 1000   // 1 minute in milliseconds after App started
    readonly property int dataSyncPeriod: 10 * 60 * 1000 // 10 minutes in milliseconds of dataSyncTimer interval
    readonly property bool busy: HttpRequest.status === HttpRequest.Busy || noncePollTimer.running

    readonly property string loggerFilePath: SystemHelper.appLogPath(Qt.application.name + ".log")
    property bool logging: SystemHelper.loadSettings(myClassName + "/logging", true)

    property string authNonce
    property string accessToken
    property string refreshToken
    property string walletAddress
    property var activityList: []

    signal errorOccurred(string text)
    signal logFileChanged(var list)

    function saveLogFile(text) {
        if (logging) {
            var txt = Qt.formatDateTime(new Date(), Qt.ISODateWithMs) + ' ' + text
            var list = txt.split('\n')
            SystemHelper.saveText(loggerFilePath, list, true)
            if (!text.endsWith("---")) logFileChanged(list)
        }
    }

    function startLogFile() { // cut off the excess at the beginning of the log file
        var trim = true
        var log = SystemHelper.fileTime(loggerFilePath)
        if (log instanceof Date && !isNaN(log)) {
            var now = new Date()
            trim = log.getFullYear() < now.getFullYear() || log.getMonth() < now.getMonth() || log.getDate() < now.getDate()
        }
        if (trim) {
            SystemHelper.renameFile(loggerFilePath, loggerFilePath + "-prev")
            SystemHelper.saveText(loggerFilePath, [], false)
        }
        var runtime = "------------------------ Start ------------------------"
        runtime += "\n\t" + Qt.application.displayName
        runtime += "\n\tQt runtime: " + qtVersion
        if (Url.schemeAt(restApiServer).endsWith('s')) {
            runtime += "\n\t"
            if (!HttpRequest.sslAvailable) {
                runtime += "No SSL support is available"
            } else if (HttpRequest.sslVerRuntime !== HttpRequest.sslVerCompile) {
                runtime += "SSL runtime: " + HttpRequest.sslVerRuntime +
                            " mismatch build on " + HttpRequest.sslVerCompile
            } else {
                runtime += "SSL runtime: " + HttpRequest.sslVerRuntime
            }
        }
        runtime += "\n\tRestApi server: " + restApiServer
        saveLogFile(runtime)
    }

    Component.onCompleted: {
        if (control.logging) startLogFile()
        if (Url.schemeAt(restApiServer).endsWith('s') && !HttpRequest.sslAvailable) {
            Qt.callLater(errorOccurred, qsTr("No SSL support is available"))
        }
    }
    Component.onDestruction: {
        if (control.logging) {
            saveLogFile("------------------------ Quit ------------------------")
        }
    }

    Timer {
        id: noncePollTimer
        interval: 3000
        repeat: false
        onTriggered: {
            if (authNonce && !accessToken) {
                var url = restApiServer + authBaseUrl + authNonce
                if (control.logging) saveLogFile("GET " + url)
                HttpRequest.send(HttpRequest.MethodGet, url)
            }
        }
    }

    Timer {
        id: tokenRefreshTimer
        repeat: false
        onTriggered: {
            if (refreshToken) {
                var url = restApiServer + authRefreshUrl
                if (control.logging) saveLogFile("POST " + url + "\n\trefreshToken: " + refreshToken)
                HttpRequest.sendObject(HttpRequest.MethodPost, url, "", { "refreshToken": refreshToken })
            }
        }
    }

    Timer {
        id: dataSyncTimer
        repeat: false
        onTriggered: {
            var now = new Date()
            var to = new Date(Math.ceil(now / dataSyncPeriod) * dataSyncPeriod)
            interval = (Math.round((to - now) / 1000) + 1) * 1000
            start()
            sqlExecQuery.syncServer()
        }
    }

    SqliteExecQuery {
        id: sqlExecQuery
        timeStep: dataSyncPeriod / 1000
        // The lastDataSync and lastToAt is the time in seconds from UTC.
        property int lastDataSync: SystemHelper.loadSettings(myClassName + "/lastDataSync", Date.now() / 1000)
        property int lastToAt: 0
        property int lastReqId: 0
        function syncServer() {
            if (!accessToken || !walletAddress || lastReqId) return
            var query = "SELECT LocalTime"
            if (timeStep > 0) query += '/' + timeStep + '*' + timeStep + '+' + timeStep
            query += " AS column0, ProjectId, TextNote, COUNT(*) AS minutesActive,"
            query += " TOTAL(KeyPresses) AS KeyPresses, TOTAL(MouseClicks) AS MouseClicks, TOTAL(MouseDistance) AS MouseDistance"
            query += " FROM '" + walletAddress + "'"
            if (lastDataSync > 0) query += " WHERE column0 > " + fromUtcSeconds(lastDataSync)
            query += " GROUP by column0, ProjectId"
            if (control.logging) saveLogFile(query)
            lastReqId = request(query)
        }
        onLastErrorChanged: control.errorOccurred(lastError)
        onResponse: function(reqid, array) {
            if (reqid !== lastReqId) return
            lastReqId = 0
            if (array.length && accessToken) {
                lastToAt = array[array.length - 1].toAt
                var url = restApiServer + activityTimeUrl
                if (control.logging) {
                    saveLogFile("POST " + url + " (" + array.length + " records)" +
                                "\n\taccessToken: " + accessToken + "\n" + JSON.stringify(array, null, 4))
                }
                HttpRequest.sendArray(HttpRequest.MethodPost, url, accessToken, array)
            } else if (control.logging) {
                saveLogFile("No records to send")
            }
        }
        function saveSyncTime() {
            if (lastToAt > 0) {
                lastDataSync = lastToAt
                SystemHelper.saveSettings(myClassName + "/lastDataSync", lastDataSync)
            }
        }
    }

    Connections {
        target: ActivityCounter
        function onAvailableChanged(ok) {
            if (control.logging) saveLogFile("ActivityCounter.available is " + ok)
            if (ok) {
                var jwt = SystemHelper.loadObject(tokenFileName)
                setTokenPair(jwt["accessToken"], jwt["refreshToken"], false)
            } else clearWallet(false)
        }
    }

    Connections {
        target: HttpRequest
        function onRecvTokens(access, refresh) {
            setTokenPair(access, refresh, true)
        }
        function onRecvObject(url, json) {
            var req = url.toString()
            if (control.logging) {
                saveLogFile("RECV Object at " + req + " (" + Object.keys(json).length + " records)" +
                            "\n" + JSON.stringify(json, null, 4))
            }
            if (req.includes(authBaseUrl)) {
                if (!authNonce) {
                    if (!json.hasOwnProperty("nonce")) {
                        if (control.logging) saveLogFile("No nonce token found")
                        control.errorOccurred(qsTr("Malformed server response"))
                        return
                    }
                    var link = restApiServer + openNonceUrl + json["nonce"]
                    if (control.logging) saveLogFile("External browser " + link)
                    if (Qt.openUrlExternally(link)) {
                        authNonce = json["nonce"]
                        noncePollTimer.restart()
                    } else {
                        if (control.logging) saveLogFile("Can't Qt.openUrlExternally() " + link)
                        control.errorOccurred(qsTr("Can't start external browser"))
                    }
                    return
                } else {
                    if (!json.hasOwnProperty("state") || json["state"].toLowerCase() !== "connected") {
                        noncePollTimer.restart()
                        return
                    }
                    if (!json.hasOwnProperty("jwt")) {
                        if (control.logging) saveLogFile("No JWT object found")
                        control.errorOccurred(qsTr("Malformed server response"))
                        return
                    }
                    var jwt = json["jwt"]
                    setTokenPair(jwt["accessToken"], jwt["refreshToken"], true)
                    return
                }
            } else if (req.includes(authRefreshUrl)) {
                if (json.hasOwnProperty("message")) {
                    control.errorOccurred(json["message"])
                }
                return
            }
            if (control.logging) saveLogFile("Got JsonObject response in an unexpected state")
        }
        function onRecvArray(url, json) {
            var req = url.toString()
            if (control.logging) {
                saveLogFile("RECV Array at " + req + " (" + json.length + " records)" +
                            "\n" + JSON.stringify(json, null, 4))
            }
            if (req.includes(listActivityUrl) && json.length === 2) {
                //activityList = json[0].map(a => Object.assign({}, a)) -- this is redundant
                activityList = json[0].slice()
                return
            } else if (req.includes(activityTimeUrl)) {
                sqlExecQuery.saveSyncTime()
                if (json.length) {
                    if (control.logging) saveLogFile("Server report " + json.length + " errors")
                    var obj = json[0]
                    if (obj.hasOwnProperty("message")) {
                        control.errorOccurred(obj["message"])
                    }
                }
                return
            }
            if (control.logging) saveLogFile("Got JsonArray response in an unexpected state")
        }
        function onRecvError(text) {
            if (control.logging) {
                saveLogFile("RECV Error at " + HttpRequest.url + "\n\t" + text)
            }
        }
    }

    function homePage() : string {
        var hp = restApiServer + "/activity"
        if (accessToken && walletAddress)
            hp += "?Authentication=" + accessToken
        return hp
    }

    function setTokenPair(access, refresh, save) {
        if (control.logging) {
            saveLogFile("setTokenPair\n\taccessToken: " + access + "\n\trefreshToken: " + refresh)
        }
        if (access && refresh) {
            if (save && !SystemHelper.saveObject(tokenFileName, { "accessToken": access, "refreshToken": refresh })) {
                if (control.logging) saveLogFile("Can't save JWT tokens to " + tokenFileName)
                control.errorOccurred(qsTr("Can't save JWT tokens"))
                return
            }
            accessToken = access
            refreshToken = refresh
            var list = accessToken.split('.')
            if (list.length === 3) {
                var obj = JSON.parse(Qt.atob(list[1]))
                if (obj.hasOwnProperty("exp") && Number.isInteger(obj["exp"])) {
                    var now = new Date()
                    var exp = new Date(parseInt(obj["exp"]) * 1000)
                    var sec = Math.floor((exp - now) / 1000)
                    /*
                     * Nonsense: the App has been running continuously for more than 30 days.
                     * Therefore, it makes no sense to set the timer for such a long period.
                     */
                    if (sec < 3600 * 24 * 30) {
                        tokenRefreshTimer.interval = Math.max((sec - 60) * 1000, 0)
                        tokenRefreshTimer.restart()
                    }
                    var to = new Date(Math.ceil(now / dataSyncDelay) * dataSyncDelay)
                    dataSyncTimer.interval = (Math.round((to - now) / 1000) + 1) * 1000
                    dataSyncTimer.start()

                    if (!tokenRefreshTimer.running || tokenRefreshTimer.interval > 3000) {
                        requestActivityList()
                    }
                    if (obj.hasOwnProperty("address") && obj["address"] !== walletAddress) {
                        walletAddress = obj["address"]
                        ActivityCounter.tableName = walletAddress
                    }
                    if (control.logging) {
                        saveLogFile("JWT parse\n\twalletAddress: " + walletAddress +
                                    "\n\texpired: " + Qt.formatDateTime(exp, Qt.ISODate) + " (" + sec + " seconds left)" +
                                    "\n\tdataSync: " + Qt.formatDateTime(to, Qt.ISODate))
                    }
                    return
                }
            }
        }
        clearWallet(true)
    }

    function startAuthenticate() {
        clearWallet(false)
        Qt.callLater(function() {
            var url = restApiServer + authBaseUrl + "nonce"
            if (control.logging) saveLogFile("POST " + url)
            HttpRequest.send(HttpRequest.MethodPost, url)
        })
    }

    function requestActivityList() {
        if (accessToken) {
            var url = restApiServer + listActivityUrl
            var obj = { "filter": {}, "page": 0, "sort": {}, "limit": 0 }
            if (control.logging) {
                saveLogFile("POST " + url +
                            "\n\taccessToken: " + accessToken +
                            "\n" + JSON.stringify(obj, null, 4))
            }
            HttpRequest.sendObject(HttpRequest.MethodPost, url, accessToken, obj)
        }
    }

    function clearWallet(remove = true) {
        if (control.logging) {
            saveLogFile("clearWallet, remove is " + remove)
        }
        authNonce = ""
        accessToken = ""
        refreshToken = ""
        walletAddress = ""
        activityList.length = 0
        noncePollTimer.stop()
        tokenRefreshTimer.stop()
        dataSyncTimer.stop()

        if (remove) SystemHelper.removeFile(tokenFileName)
        ActivityCounter.tableName = ""
        HttpRequest.cancel()
    }
}
