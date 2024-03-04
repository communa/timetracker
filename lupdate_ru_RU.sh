#!/bin/bash
QT_PATH=/usr/local/Qt/bin
APP_NAME=time-tracker
TS_PATH=translations/$APP_NAME_ru_RU.ts
$QT_PATH/lupdate $APP_NAME.pro -ts $TS_PATH
#$QT_PATH/lrelease $TS_PATH
