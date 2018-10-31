#include "mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    editor = new QTextEdit(this);
    editor->setHtml("<ul><li>tet</li></ul>");

    qDebug() << QBasicHtmlExporter( editor->document() ).toHtml();

    this->resize(800,600);
    editor->resize(800,600);
    this->show();
}

MainWindow::~MainWindow()
{
    delete editor;
}
