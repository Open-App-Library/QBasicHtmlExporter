#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QTextEdit>
#include <qbasichtmlexporter.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTextEdit *e = new QTextEdit();
    e->setHtml("<h1>Hello world</h1>");

    QString html = QBasicHtmlExporter(e->document()).toHtml();

    qDebug() << html;
}
