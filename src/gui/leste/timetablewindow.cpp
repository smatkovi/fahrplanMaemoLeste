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

#include "timetablewindow.h"

#include "leste.h"

#include "fahrplan.h"
#include "fahrplan_parser_thread.h"
#include "models/timetable.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QMenuBar>
#include <QVBoxLayout>

#define T(context, text) QCoreApplication::translate(context, text)

TimetableWindow::TimetableWindow(Fahrplan *fahrplan, QWidget *parent)
    : QMainWindow(parent)
    , m_fahrplan(fahrplan)
    , m_searching(false)
{
    Leste::makeStackedWindow(this);
    setAttribute(Qt::WA_DeleteOnClose);
    const bool arrivals = m_fahrplan->mode() == Fahrplan::ArrivalMode;
    setWindowTitle(arrivals ? T("TimetablePage", "Arrivals") : T("TimetablePage", "Departures"));

    QWidget *content = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(content);
    m_header = new QLabel(content);
    m_header->setWordWrap(true);
    m_header->setAlignment(Qt::AlignCenter);
    m_header->setTextFormat(Qt::RichText);
    QString header = QLatin1String("<b>") + Leste::esc(m_fahrplan->currentStationName()) + QLatin1String("</b>");
    const QString direction = m_fahrplan->directionStationName();
    if (m_fahrplan->parser()->supportsTimeTableDirection() && direction != T("Fahrplan", "please select"))
        header += QLatin1String(" &rarr; ") + Leste::esc(direction);
    m_header->setText(header);
    vbox->addWidget(m_header);
    m_status = new QLabel(content);
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setWordWrap(true);
    vbox->addWidget(m_status);
    m_list = new QListWidget(content);
    m_list->setItemDelegate(new RichItemDelegate(m_list));
    m_list->setResizeMode(QListView::Adjust);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    Leste::enableKineticScrolling(m_list);
    vbox->addWidget(m_list, 1);
    setCentralWidget(content);

    QAction *refresh = menuBar()->addAction(tr("Refresh"));
    connect(refresh, SIGNAL(triggered()), SLOT(refresh()));
    Leste::useApplicationMenu(this);

    connect(m_fahrplan, SIGNAL(parserTimeTableResult()), SLOT(onTimetableResult()));
    connect(m_fahrplan, SIGNAL(parserErrorOccured(QString)), SLOT(onParserError(QString)));
}

void TimetableWindow::start()
{
    setSearching(true);
    m_fahrplan->getTimeTable();
}

void TimetableWindow::refresh()
{
    if (m_searching)
        return;
    m_list->clear();
    start();
}

void TimetableWindow::closeEvent(QCloseEvent *event)
{
    if (m_searching)
        m_fahrplan->parser()->cancelRequest();
    QMainWindow::closeEvent(event);
}

void TimetableWindow::setSearching(bool searching)
{
    m_searching = searching;
    Leste::setBusy(this, searching);
    if (searching) {
        m_status->setText(T("TimetablePage", "Searching..."));
        m_status->show();
    } else {
        m_status->hide();
    }
}

void TimetableWindow::onTimetableResult()
{
    if (!m_searching)
        return;
    setSearching(false);
    m_list->clear();

    const bool arrivals = m_fahrplan->mode() == Fahrplan::ArrivalMode;
    const QString grey = Leste::secondaryTextColor();
    const QString active = Leste::activeTextColor();
    Timetable *timetable = m_fahrplan->timetable();
    const QString timeFormat = QLocale().timeFormat(QLocale::ShortFormat);
    for (int row = 0; row < timetable->rowCount(); ++row) {
        const QModelIndex index = timetable->index(row, 0);
        const QString time = timetable->data(index, Timetable::Time).toTime().toString(timeFormat);
        const QString destination = timetable->data(index, Timetable::DestinationStation).toString();
        const QString train = timetable->data(index, Timetable::TrainType).toString();
        const QString platform = timetable->data(index, Timetable::Platform).toString();
        const QString station = timetable->data(index, Timetable::CurrentStation).toString();
        const QString misc = timetable->data(index, Timetable::MiscInfo).toString();

        QString html = QString::fromLatin1("<b>%1</b>&nbsp;&nbsp; <b>%2</b>&nbsp; %3 <b>%4</b>")
                .arg(Leste::esc(time), Leste::esc(train),
                     arrivals ? T("TimetableEntryDelegate", "from") : T("TimetableEntryDelegate", "to"),
                     Leste::esc(destination));
        QString where;
        if (!platform.isEmpty())
            where = T("TimetableEntryDelegate", "Pl. %1").arg(Leste::esc(platform));
        if (!station.isEmpty())
            where += (where.isEmpty() ? QString() : QString::fromLatin1(" / ")) + Leste::esc(station);
        if (!where.isEmpty())
            html += QString::fromLatin1("<br><font color=\"%1\">%2</font>").arg(grey, where);
        if (!misc.isEmpty())
            html += QString::fromLatin1("<br><small><font color=\"%1\">%2</font></small>").arg(active, misc);
        new QListWidgetItem(html, m_list);
    }

    if (timetable->rowCount() == 0) {
        m_status->setText(T("MainPage", "No results found"));
        m_status->show();
    }
}

void TimetableWindow::onParserError(const QString &message)
{
    if (!m_searching)
        return;
    setSearching(false);
    m_status->setText(message);
    m_status->show();
    Leste::showBanner(this, message);
}
