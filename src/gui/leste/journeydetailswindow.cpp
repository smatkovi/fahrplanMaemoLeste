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

#include "journeydetailswindow.h"

#include "leste.h"

#include "fahrplan.h"
#include "fahrplan_parser_thread.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QVBoxLayout>

#define T(context, text) QCoreApplication::translate(context, text)

static QString hhmm(const QDateTime &dateTime)
{
    return dateTime.isValid() ? dateTime.time().toString(QLatin1String("hh:mm")) : QString();
}

JourneyDetailsWindow::JourneyDetailsWindow(Fahrplan *fahrplan, QWidget *parent)
    : QMainWindow(parent)
    , m_fahrplan(fahrplan)
    , m_searching(false)
{
    Leste::makeStackedWindow(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(T("JourneyDetailsResultsPage", "Journey advice"));

    QWidget *content = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(content);
    m_header = new QLabel(content);
    m_header->setWordWrap(true);
    m_header->setAlignment(Qt::AlignCenter);
    m_header->setTextFormat(Qt::RichText);
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

    connect(m_fahrplan, SIGNAL(parserJourneyDetailsResult(JourneyDetailResultList*)),
            SLOT(onDetailsResult(JourneyDetailResultList*)));
    connect(m_fahrplan, SIGNAL(parserErrorOccured(QString)), SLOT(onParserError(QString)));
    // no menu entries yet, but the platform plugin adds a menu bar anyway
    Leste::useApplicationMenu(this);
}

void JourneyDetailsWindow::start(const QString &journeyId)
{
    m_header->setText(QLatin1String("<b>") + T("JourneyResultsPage", "Loading details") + QLatin1String("</b>"));
    setSearching(true);
    m_fahrplan->parser()->getJourneyDetails(journeyId);
}

void JourneyDetailsWindow::closeEvent(QCloseEvent *event)
{
    if (m_searching)
        m_fahrplan->parser()->cancelRequest();
    QMainWindow::closeEvent(event);
}

void JourneyDetailsWindow::setSearching(bool searching)
{
    m_searching = searching;
    Leste::setBusy(this, searching);
    if (searching) {
        m_status->setText(T("JourneyResultsPage", "please wait..."));
        m_status->show();
    } else {
        m_status->hide();
    }
}

void JourneyDetailsWindow::addStation(const QString &arrival, const QString &departure,
                                      const QString &station, const QString &info)
{
    QString times;
    if (!arrival.isEmpty() && !departure.isEmpty() && arrival != departure)
        times = arrival + QLatin1String(" / ") + departure;
    else
        times = arrival.isEmpty() ? departure : arrival;

    QString html = QString::fromLatin1("<b>%1</b>&nbsp;&nbsp; %2").arg(Leste::esc(times), Leste::esc(station));
    if (!info.isEmpty())
        html += QString::fromLatin1("<br><small><font color=\"%1\">%2</font></small>")
                .arg(Leste::secondaryTextColor(), info);
    new QListWidgetItem(html, m_list);
}

void JourneyDetailsWindow::addTrain(const QString &train, const QString &direction, const QString &info)
{
    const QString grey = Leste::secondaryTextColor();
    QString line = QLatin1String("<b>") + Leste::esc(train) + QLatin1String("</b>");
    if (!direction.isEmpty())
        line = T("JourneyDetailsDelegate", "%1 to %2").arg(line, Leste::esc(direction));
    QString html = QString::fromLatin1("<table><tr><td width=\"40\"><font color=\"%1\">&#8595;</font></td><td>%2")
            .arg(Leste::activeTextColor(), line);
    if (!info.isEmpty())
        html += QString::fromLatin1("<br><small><font color=\"%1\">%2</font></small>").arg(grey, info);
    html += QLatin1String("</td></tr></table>");
    new QListWidgetItem(html, m_list);
}

void JourneyDetailsWindow::onDetailsResult(JourneyDetailResultList *result)
{
    if (!m_searching)
        return;
    setSearching(false);
    m_list->clear();

    const int count = result ? int(result->itemcount()) : 0;
    if (count == 0) {
        m_header->setText(T("MainPage", "Error loading details"));
        Leste::showBanner(this, T("MainPage", "Error loading details"));
        return;
    }

    const QString via = result->viaStation();
    QString header = via.isEmpty()
            ? T("MainPage", "<b>%1</b> to <b>%2</b>").arg(Leste::esc(result->departureStation()),
                                                         Leste::esc(result->arrivalStation()))
            : T("MainPage", "<b>%1</b> via <b>%3</b> to <b>%2</b>").arg(Leste::esc(result->departureStation()),
                                                                       Leste::esc(result->arrivalStation()),
                                                                       Leste::esc(via));
    const QDateTime dep = result->departureDateTime();
    const QDateTime arr = result->arrivalDateTime();
    QString when = QLocale().toString(dep.date(), QLocale::ShortFormat) + QLatin1Char(' ') + hhmm(dep)
            + QLatin1String(" &ndash; ");
    if (arr.date() != dep.date())
        when += QLocale().toString(arr.date(), QLocale::ShortFormat) + QLatin1Char(' ');
    when += hhmm(arr);
    if (!result->duration().isEmpty())
        when += QLatin1String("&nbsp;&nbsp; ") + T("JourneyDetailsResultsPage", "Dur.: %1").arg(Leste::esc(result->duration()));
    header += QString::fromLatin1("<br><font color=\"%1\">%2</font>").arg(Leste::secondaryTextColor(), when);
    if (!result->info().isEmpty())
        header += QLatin1String("<br><i>") + result->info() + QLatin1String("</i>");
    m_header->setText(header);

    // The same walk over the legs as the Harmattan details page: a station
    // row, the train leaving it, the next station (arrival / departure), ...
    for (int i = 0; i < count; ++i) {
        JourneyDetailResultItem *item = result->getItem(i);
        JourneyDetailResultItem *next = (i + 1 < count) ? result->getItem(i + 1) : 0;

        if (i == 0) {
            addStation(QString(), hhmm(item->departureDateTime()), item->departureStation(), item->departureInfo());
            addTrain(item->train(), item->direction(), item->info());
        }
        if (next) {
            QString station = item->arrivalStation();
            if (next->departureStation() != item->arrivalStation())
                station += QLatin1String(" / ") + next->departureStation();
            QString info = item->arrivalInfo();
            if (!info.isEmpty() && !next->departureInfo().isEmpty())
                info += QLatin1String(" / ");
            info += next->departureInfo();
            addStation(hhmm(item->arrivalDateTime()), hhmm(next->departureDateTime()), station, info);
            addTrain(next->train(), next->direction(), next->info());
        } else {
            addStation(hhmm(item->arrivalDateTime()), QString(), item->arrivalStation(), item->arrivalInfo());
        }
    }
}

void JourneyDetailsWindow::onParserError(const QString &message)
{
    if (!m_searching)
        return;
    setSearching(false);
    m_header->setText(message);
    Leste::showBanner(this, message);
}
