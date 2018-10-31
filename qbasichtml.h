#ifndef QBASICHTML_H
#define QBASICHTML_H
#include <QTextDocument>
#include <QTextFrame>

class QBasicHtmlExporter
{
public:
    QBasicHtmlExporter(QTextDocument *_doc);
    enum ExportMode {
        ExportEntireDocument,
        ExportFragment
    };
    QString toHtml();

private:
    enum StyleMode { EmitStyleTag, OmitStyleTag };
    enum FrameType { TextFrame, TableFrame, RootFrame };

    void emitFrame(const QTextFrame::Iterator &frameIt);
    void emitTextFrame(const QTextFrame *f);
    void emitBlock(const QTextBlock &block);

    bool emitCharFormatStyle(const QTextCharFormat &format);

    QString html;
    QTextCharFormat defaultCharFormat;
    const QTextDocument *doc;
    bool fragmentMarkers;
};

#endif // QBASICHTML_H
