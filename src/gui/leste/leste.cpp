/****************************************************************************
**
**  This file is a part of Fahrplan.
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License along
**  with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "leste.h"

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QPainter>
#include <QScroller>
#include <QScrollerProperties>
#include <QTextDocument>
#include <QTimer>
#include <QWidget>

#include <QMaemo5InformationBox>
#include <QMaemo5Style>

namespace Leste
{

void makeStackedWindow(QWidget *window)
{
    window->setProperty("X-Maemo-StackedWindow", 1);
    // 2 = follow the device orientation (landscape and portrait)
    window->setProperty("X-Maemo-Orientation", 2);
}

void useApplicationMenu(QMainWindow *window)
{
    QMenuBar *menuBar = window->menuBar();
    menuBar->setFixedHeight(0);
    menuBar->hide();
}

void setBusy(QWidget *window, bool busy)
{
    if (window)
        window->setProperty("X-Maemo-Progress", busy ? 1 : 0);
}

void enableKineticScrolling(QAbstractScrollArea *area)
{
    if (QAbstractItemView *view = qobject_cast<QAbstractItemView *>(area)) {
        view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    }
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScroller::grabGesture(area->viewport(), QScroller::LeftMouseButtonGesture);

    QScroller *scroller = QScroller::scroller(area->viewport());
    QScrollerProperties props = scroller->scrollerProperties();
    props.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
                          QScrollerProperties::OvershootAlwaysOff);
    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy,
                          QScrollerProperties::OvershootWhenScrollable);
    scroller->setScrollerProperties(props);
}

void showBanner(QWidget *parent, const QString &text)
{
    QMaemo5InformationBox::information(parent, text);
}

static QString colorName(const char *logicalName, const QColor &fallback)
{
    QColor c = QMaemo5Style::standardColor(QString::fromLatin1(logicalName));
    if (!c.isValid())
        c = fallback;
    return c.name();
}

QString secondaryTextColor()
{
    return colorName("SecondaryTextColor", QColor("#999999"));
}

QString activeTextColor()
{
    return colorName("ActiveTextColor", QColor("#3399ff"));
}

QString esc(const QString &text)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return text.toHtmlEscaped();
#else
    return Qt::escape(text);
#endif
}

int rowHeight(const QWidget *widget)
{
    // A Hildon "finger" row is 70 px for an 18 px font on the N900; scale
    // with the font so that it fits the high resolution screens as well.
    const QFontMetrics fm(widget ? widget->font() : QApplication::font());
    return qMax(70, fm.height() * 5 / 2);
}

void installWindowGrabber(const QString &fileName)
{
    QTimer *timer = new QTimer(qApp);
    QObject::connect(timer, &QTimer::timeout, [fileName]() {
        QWidget *top = QApplication::activeModalWidget();
        if (!top)
            top = QApplication::activePopupWidget();
        if (!top) {
            // the stacked window highest up the stack
            int best = -1;
            foreach (QWidget *w, QApplication::topLevelWidgets()) {
                if (!w->isVisible() || !w->isWindow())
                    continue;
                const QVariant pos = w->property("X-Maemo-StackedWindowPosition");
                const int p = pos.isValid() ? pos.toInt() : 0;
                if (p >= best) {
                    best = p;
                    top = w;
                }
            }
        }
        if (top)
            top->grab().save(fileName);
    });
    timer->start(2000);
}

} // namespace Leste

RichItemDelegate::RichItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QString RichItemDelegate::html(const QModelIndex &index) const
{
    return index.data(Qt::DisplayRole).toString();
}

void RichItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    // Background (pressed / selected highlight) from the style, without text.
    const QString text = opt.text;
    opt.text = QString();
    const QWidget *widget = opt.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    const int margin = opt.fontMetrics.height() / 2;
    QTextDocument doc;
    doc.setDefaultFont(opt.font);
    doc.setDocumentMargin(0);
    doc.setTextWidth(qMax(10, opt.rect.width() - 2 * margin));
    doc.setHtml(text);

    painter->save();
    const int y = opt.rect.top() + qMax(0, (opt.rect.height() - int(doc.size().height())) / 2);
    painter->translate(opt.rect.left() + margin, y);
    QAbstractTextDocumentLayout::PaintContext ctx;
    const bool selected = opt.state & QStyle::State_Selected;
    ctx.palette = opt.palette;
    ctx.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Normal,
                         selected ? QPalette::HighlightedText : QPalette::Text));
    doc.documentLayout()->draw(painter, ctx);
    painter->restore();
}

QSize RichItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const int margin = opt.fontMetrics.height() / 2;
    int width = opt.rect.width();
    if (width <= 0 && opt.widget)
        width = opt.widget->width();
    if (width <= 0)
        width = 400;

    QTextDocument doc;
    doc.setDefaultFont(opt.font);
    doc.setDocumentMargin(0);
    doc.setTextWidth(qMax(10, width - 2 * margin));
    doc.setHtml(html(index));

    const int height = qMax(Leste::rowHeight(opt.widget), int(doc.size().height()) + 2 * margin);
    return QSize(width, height);
}
