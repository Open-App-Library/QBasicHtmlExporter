#include "mainwindow.h"
#include <QDebug>

// TODO: Proper unit testing

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    editor = new QTextEdit(this);
    editor->setHtml("<h1>Test!</h1>");

    qDebug() << QBasicHtmlExporter( editor->document() ).toHtml();

    this->resize(800,600);
    editor->resize(800,600);
    this->show();
}

MainWindow::~MainWindow()
{
    delete editor;
}
