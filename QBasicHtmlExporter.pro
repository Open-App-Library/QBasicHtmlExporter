#-------------------------------------------------
#
# Project created by QtCreator 2018-10-31T12:08:06
#
#-------------------------------------------------

QT       += core gui

TARGET = QBasicHtmlExporter
TEMPLATE = lib
CONFIG += staticlib

DEFINES += QT_DEPRECATED_WARNINGS

# Include sources and headers
include( $$PWD/QBasicHtmlExporterInclude.pro )

unix {
    target.path = /usr/lib
    INSTALLS += target
}
