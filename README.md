QBasicHtmlExporter
---

Provides an alternative `toHtml()` function for `QTextDocument`s that export basic-formatted HTML.

Used in [Vibrato Notes](https://vibrato.app) and the [Escriba text editor](https://gitlab.com/Open-App-Library/escriba).

## Installation

There are various ways to get set up. I will share the easiest way with QMake.

First, you need to clone the repo to your project folder.

Then, you just have to add the following lines to your `.pro` file:

```
include($$PWD/QBasicHtmlExporter/QBasicHtmlExporter.pro)
INCLUDEPATH += $$PWD/QBasicHtmlExporter
```

You're all set!

## Usage

Here is a trivial example to get you up and running.

```c++
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
```
