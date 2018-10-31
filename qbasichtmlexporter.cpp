#include "qbasichtmlexporter.h"
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
    html.remove(QRegExp("^[\r\n]+"));
    return html;
}

QBasicHtmlExporter::Heading QBasicHtmlExporter::headingType(QString name)
{
    if ( name == "xx-large" ) return h1;
    if ( name == "x-large" )  return h2;
    if ( name == "large" )    return h3;
    if ( name == "medium" )   return h4;
    if ( name == "small" )    return h5;
    return paragraph;
}

QString QBasicHtmlExporter::headingStr(QBasicHtmlExporter::Heading heading)
{
    switch (heading) {
    case h1:
        return "h1";
    case h2:
        return "h2";
    case h3:
        return "h3";
    case h4:
        return "h4";
    case h5:
        return "h5";
    default:
        return "p";
    }
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
                //emitTable(table);
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
    html += QLatin1String("\n<table>");
    QTextFrameFormat format = f->frameFormat();
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

    // emitBlockAttributes(block); // TAKEN OUT NOT NEEDED

    if (block.begin().atEnd())
        html += QLatin1String("<br />");

    QTextBlock::Iterator it = block.begin();

    // IDK WHAT THIS IS
//    if (fragmentMarkers && !it.atEnd() && block == doc->begin())
//        html += QLatin1String("<!--StartFragment-->");
    for (; !it.atEnd(); ++it)
        emitFragment(it.fragment());
//    if (fragmentMarkers && block.position() + block.length() == doc->docHandle()->length())
//        html += QLatin1String("<!--EndFragment-->");

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

void QBasicHtmlExporter::emitFragment(const QTextFragment &fragment)
{
    const QTextCharFormat format = fragment.charFormat();
    bool closeAnchor = false;
    if (format.isAnchor()) {
        const QString name = format.anchorName();
        if (!name.isEmpty()) {
            html += QLatin1String("<a name=\"");
            html += name.toHtmlEscaped();
            html += QLatin1String("\"></a>");
        }
        const QString href = format.anchorHref();
        if (!href.isEmpty()) {
            html += QLatin1String("<a href=\"");
            html += href.toHtmlEscaped();
            html += QLatin1String("\">");
            closeAnchor = true;
        }
    }
    QString txt = fragment.text();
    const bool isObject = txt.contains(QChar::ObjectReplacementCharacter);
    const bool isImage = isObject && format.isImageFormat();
//    QLatin1String styleTag("<span style=\"");
//    html += styleTag;
    QStringList closing_tags = emitCharFormatStyle(format);
    bool attributesEmitted = false;
    if (!isImage)
        attributesEmitted = closing_tags.length();
//    if (attributesEmitted)
//        html += QLatin1String("\">");
//    else
//        html.chop(styleTag.size());
    if (isObject) {
        for (int i = 0; isImage && i < txt.length(); ++i) {
            QTextImageFormat imgFmt = format.toImageFormat();
            html += QLatin1String("<img");
            if (imgFmt.hasProperty(QTextFormat::ImageName))
                emitAttribute("src", imgFmt.name());
            html += QLatin1String(" />");
        }
    } else {
        Q_ASSERT(!txt.contains(QChar::ObjectReplacementCharacter));
        txt = txt.toHtmlEscaped();
        // split for [\n{LineSeparator}]
        QString forcedLineBreakRegExp = QString::fromLatin1("[\\na]");
        forcedLineBreakRegExp[3] = QChar::LineSeparator;
        // space in BR on purpose for compatibility with old-fashioned browsers
        html += txt.replace(QRegExp(forcedLineBreakRegExp), QLatin1String("<br />"));
    }
    for (int i = 0; i < closing_tags.length(); i++)
        html += closing_tags[i];
    if (closeAnchor)
        html += QLatin1String("</a>");
}

void QBasicHtmlExporter::emitAttribute(const char *attribute, const QString &value)
{
    html += QLatin1Char(' ');
    html += QLatin1String(attribute);
    html += QLatin1String("=\"");
    html += value.toHtmlEscaped();
    html += QLatin1Char('"');
}

QStringList QBasicHtmlExporter::emitCharFormatStyle(const QTextCharFormat &format)
{
    QStringList closing_tags;
    bool isHeading = false;
    Heading cur_heading = paragraph;

     if (format.hasProperty(QTextFormat::FontSizeAdjustment)) {
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
         const char *name = nullptr;
         const int idx = format.intProperty(QTextFormat::FontSizeAdjustment) + 1;
         if (idx >= 0 && idx <= 4) {
             name = sizeNameData + sizeNameOffsets[idx];
         }
         if (name) {
             cur_heading = headingType(name);
             if (cur_heading) isHeading = true;
         }
     }

     // Opening and closing tags for paragraph or heading
     html += QString("<%1>").arg( headingStr(cur_heading) );
     closing_tags << QString("</%1>").arg( headingStr(cur_heading) );

     if (format.hasProperty(QTextFormat::FontWeight && !isHeading)
         && format.fontWeight() != defaultCharFormat.fontWeight()) {
         html += QLatin1String("<strong>");
         closing_tags << "</strong>";
     }

     if (format.hasProperty(QTextFormat::FontItalic)
         && format.fontItalic() != defaultCharFormat.fontItalic()) {
         html += QLatin1String("<it>");
         closing_tags << "</it>";
     }

     // UNDERLINE CODE
//     QLatin1String decorationTag(" text-decoration:");
//     html += decorationTag;
//     bool hasDecoration = false;
//     bool atLeastOneDecorationSet = false;

//     if ((format.hasProperty(QTextFormat::FontUnderline) || format.hasProperty(QTextFormat::TextUnderlineStyle))
//         && format.fontUnderline() != defaultCharFormat.fontUnderline()) {
//         hasDecoration = true;
//         if (format.fontUnderline()) {
//             html += QLatin1String(" underline");
//             atLeastOneDecorationSet = true;
//         }
//     }

     if (format.hasProperty(QTextFormat::FontStrikeOut)
         && format.fontStrikeOut() != defaultCharFormat.fontStrikeOut()) {
         if (format.fontStrikeOut()) {
             html += QLatin1String("<del>");
             closing_tags << "</del>";
         }
     }

     return closing_tags;
}
