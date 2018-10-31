#include "qbasichtml.h"
#include <QDebug>
#include <QTextDocument>
#include <QTextList>
#include <QtMath>
#include "private/qtextdocumentfragment_p.h"

static QTextFormat formatDifference(const QTextFormat &from, const QTextFormat &to)
{
    QTextFormat diff = to;
    const QMap<int, QVariant> props = to.properties();
    for (QMap<int, QVariant>::ConstIterator it = props.begin(), end = props.end();
         it != end; ++it)
        if (it.value() == from.property(it.key()))
            diff.clearProperty(it.key());
    return diff;
}

QBasicHtmlExporter::QBasicHtmlExporter(QTextDocument *_doc)
{
    doc = _doc;
    const QFont defaultFont = doc->defaultFont();
    defaultCharFormat.setFont(defaultFont);
    // don't export those for the default font since we cannot turn them off with CSS
    defaultCharFormat.clearProperty(QTextFormat::FontUnderline);
    defaultCharFormat.clearProperty(QTextFormat::FontOverline);
    defaultCharFormat.clearProperty(QTextFormat::FontStrikeOut);
    defaultCharFormat.clearProperty(QTextFormat::TextUnderlineStyle);
}

QString QBasicHtmlExporter::toHtml()
{
    html = QLatin1String("");
    emitFrame(doc->rootFrame()->begin());

    return html;
}

void QBasicHtmlExporter::emitFrame(const QTextFrame::Iterator &frameIt)
{
    if (!frameIt.atEnd()) {
        QTextFrame::Iterator next = frameIt;
        ++next;
        if (next.atEnd()
            && frameIt.currentFrame() == nullptr
            && frameIt.parentFrame() != doc->rootFrame()
            && frameIt.currentBlock().begin().atEnd())
            return;
    }
    for (QTextFrame::Iterator it = frameIt;
         !it.atEnd(); ++it) {
        if (QTextFrame *f = it.currentFrame()) {
            if (QTextTable *table = qobject_cast<QTextTable *>(f)) {
                emitTable(table);
            } else {
                emitTextFrame(f);
            }
        } else if (it.currentBlock().isValid()) {
            emitBlock(it.currentBlock());
        }
    }

}

void QBasicHtmlExporter::emitTextFrame(const QTextFrame *f)
{
    FrameType frameType = f->parentFrame() ? TextFrame : RootFrame;
    html += QLatin1String("\n<table");
    QTextFrameFormat format = f->frameFormat();
    emitFrameStyle(format, frameType);
    html += QLatin1Char('>');
    html += QLatin1String("\n<tr>\n<td\">");
    emitFrame(f->begin());
    html += QLatin1String("</td></tr></table>");
}

void QBasicHtmlExporter::emitBlock(const QTextBlock &block)
{
    if (block.begin().atEnd()) {
        // ### HACK, remove once QTextFrame::Iterator is fixed
        int p = block.position();
        if (p > 0)
            --p;
        QTextDocumentPrivate::FragmentIterator frag = doc->docHandle()->find(p);
        QChar ch = doc->docHandle()->buffer().at(frag->stringPosition);
        if (ch == QTextBeginningOfFrame
            || ch == QTextEndOfFrame)
            return;
    }
    html += QLatin1Char('\n');
    // save and later restore, in case we 'change' the default format by
    // emitting block char format information
    QTextCharFormat oldDefaultCharFormat = defaultCharFormat;
    QTextList *list = block.textList();
    bool numbered_list = false;
    if (list) {
        if (list->itemNumber(block) == 0) { // first item? emit <ul> or appropriate
            const QTextListFormat format = list->format();
            const int style = format.style();
            switch (style) {
                case QTextListFormat::ListDecimal: numbered_list = true; break;
                case QTextListFormat::ListLowerAlpha: numbered_list = true; break;
                case QTextListFormat::ListUpperAlpha: numbered_list = true; break;
                case QTextListFormat::ListLowerRoman: numbered_list = true; break;
                case QTextListFormat::ListUpperRoman: numbered_list = true; break;
            }

            html += QString("<%1>").arg(numbered_list ? "ul" : "li");
        }
        html += QLatin1String("<li>");
        const QTextCharFormat blockFmt = formatDifference(defaultCharFormat, block.charFormat()).toCharFormat();
        if (!blockFmt.properties().isEmpty()) {
            emitCharFormatStyle(blockFmt);
            defaultCharFormat.merge(block.charFormat());
        }
    }

    const QTextBlockFormat blockFormat = block.blockFormat();
    if (blockFormat.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth)) {
        html += QLatin1String("<hr />");
        return;
    }

    const bool pre = blockFormat.nonBreakableLines();
    if (pre) {
        html += QLatin1String("<pre>");
    } else if (!list) {
        // TODO!!!! We need to determine what the hell to do here
        // since Qt removed the 'headinglevel' function from the API
        // html += QLatin1String("<p");
    }

    emitBlockAttributes(block);

    if (block.begin().atEnd())
        html += QLatin1String("<br />");

    QTextBlock::Iterator it = block.begin();

    // IDK WHAT THIS IS
    if (fragmentMarkers && !it.atEnd() && block == doc->begin())
        html += QLatin1String("<!--StartFragment-->");
    for (; !it.atEnd(); ++it)
        emitFragment(it.fragment());
    if (fragmentMarkers && block.position() + block.length() == doc->docHandle()->length())
        html += QLatin1String("<!--EndFragment-->");

    if (pre)
        html += QLatin1String("</pre>");
    else if (list)
        html += QLatin1String("</li>");
    else {
        // MUST FIGURE OUT HOW TO CLOSE REST OF THIS OFF
        // html += "</p>";
    }
    if (list) {
        if (list->itemNumber(block) == list->count() - 1) { // last item? close list
            if ( numbered_list )
                html += QLatin1String("</ol>");
            else
                html += QLatin1String("</ul>");
        }
    }
    defaultCharFormat = oldDefaultCharFormat;
}

bool QBasicHtmlExporter::emitCharFormatStyle(const QTextCharFormat &format)
{
    bool attributesEmitted = false;

     if (format.hasProperty(QTextFormat::FontPointSize)
         && qFloor(format.fontPointSize()) != qFloor(defaultCharFormat.fontPointSize())) {
         html += QLatin1String(" font-size:");
         html += QString::number(format.fontPointSize());
         html += QLatin1String("pt;");
         attributesEmitted = true;
     } else if (format.hasProperty(QTextFormat::FontSizeAdjustment)) {
         static const char sizeNameData[] =
             "small" "\0"
             "medium" "\0"
             "xx-large" ;
         static const quint8 sizeNameOffsets[] = {
             0,                                         // "small"
             sizeof("small"),                           // "medium"
             sizeof("small") + sizeof("medium") + 3,    // "large"    )
             sizeof("small") + sizeof("medium") + 1,    // "x-large"  )> compressed into "xx-large"
             sizeof("small") + sizeof("medium"),        // "xx-large" )
         };
         const char *name = 0;
         const int idx = format.intProperty(QTextFormat::FontSizeAdjustment) + 1;
         if (idx >= 0 && idx <= 4) {
             name = sizeNameData + sizeNameOffsets[idx];
         }
         if (name) {
             html += QLatin1String(" font-size:");
             html += QLatin1String(name);
             html += QLatin1Char(';');
             attributesEmitted = true;
         }
     } else if (format.hasProperty(QTextFormat::FontPixelSize)) {
         html += QLatin1String(" font-size:");
         html += QString::number(format.intProperty(QTextFormat::FontPixelSize));
         html += QLatin1String("px;");
         attributesEmitted = true;
     }

     if (format.hasProperty(QTextFormat::FontWeight)
         && format.fontWeight() != defaultCharFormat.fontWeight()) {
         html += QLatin1String(" font-weight:");
         html += QString::number(format.fontWeight() * 8);
         html += QLatin1Char(';');
         attributesEmitted = true;
     }

     if (format.hasProperty(QTextFormat::FontItalic)
         && format.fontItalic() != defaultCharFormat.fontItalic()) {
         html += QLatin1String(" font-style:");
         html += (format.fontItalic() ? QLatin1String("italic") : QLatin1String("normal"));
         html += QLatin1Char(';');
         attributesEmitted = true;
     }

     QLatin1String decorationTag(" text-decoration:");
     html += decorationTag;
     bool hasDecoration = false;
     bool atLeastOneDecorationSet = false;

     if ((format.hasProperty(QTextFormat::FontUnderline) || format.hasProperty(QTextFormat::TextUnderlineStyle))
         && format.fontUnderline() != defaultCharFormat.fontUnderline()) {
         hasDecoration = true;
         if (format.fontUnderline()) {
             html += QLatin1String(" underline");
             atLeastOneDecorationSet = true;
         }
     }

     if (format.hasProperty(QTextFormat::FontOverline)
         && format.fontOverline() != defaultCharFormat.fontOverline()) {
         hasDecoration = true;
         if (format.fontOverline()) {
             html += QLatin1String(" overline");
             atLeastOneDecorationSet = true;
         }
     }

     if (format.hasProperty(QTextFormat::FontStrikeOut)
         && format.fontStrikeOut() != defaultCharFormat.fontStrikeOut()) {
         hasDecoration = true;
         if (format.fontStrikeOut()) {
             html += QLatin1String(" line-through");
             atLeastOneDecorationSet = true;
         }
     }

     if (hasDecoration) {
         if (!atLeastOneDecorationSet)
             html += QLatin1String("none");
         html += QLatin1Char(';');
         attributesEmitted = true;
     } else {
         html.chop(decorationTag.size());
     }

     if (format.foreground() != defaultCharFormat.foreground()
         && format.foreground().style() != Qt::NoBrush) {
         html += QLatin1String(" color:");
         html += colorValue(format.foreground().color());
         html += QLatin1Char(';');
         attributesEmitted = true;
     }

     if (format.background() != defaultCharFormat.background()
         && format.background().style() == Qt::SolidPattern) {
         html += QLatin1String(" background-color:");
         html += colorValue(format.background().color());
         html += QLatin1Char(';');
         attributesEmitted = true;
     }

     if (format.verticalAlignment() != defaultCharFormat.verticalAlignment()
         && format.verticalAlignment() != QTextCharFormat::AlignNormal)
     {
         html += QLatin1String(" vertical-align:");

         QTextCharFormat::VerticalAlignment valign = format.verticalAlignment();
         if (valign == QTextCharFormat::AlignSubScript)
             html += QLatin1String("sub");
         else if (valign == QTextCharFormat::AlignSuperScript)
             html += QLatin1String("super");
         else if (valign == QTextCharFormat::AlignMiddle)
             html += QLatin1String("middle");
         else if (valign == QTextCharFormat::AlignTop)
             html += QLatin1String("top");
         else if (valign == QTextCharFormat::AlignBottom)
             html += QLatin1String("bottom");

         html += QLatin1Char(';');
         attributesEmitted = true;
     }

     if (format.fontCapitalization() != QFont::MixedCase) {
         const QFont::Capitalization caps = format.fontCapitalization();
         if (caps == QFont::AllUppercase)
             html += QLatin1String(" text-transform:uppercase;");
         else if (caps == QFont::AllLowercase)
             html += QLatin1String(" text-transform:lowercase;");
         else if (caps == QFont::SmallCaps)
             html += QLatin1String(" font-variant:small-caps;");
         attributesEmitted = true;
     }

     if (format.fontWordSpacing() != 0.0) {
         html += QLatin1String(" word-spacing:");
         html += QString::number(format.fontWordSpacing());
         html += QLatin1String("px;");
         attributesEmitted = true;
     }

     return attributesEmitted;
}
