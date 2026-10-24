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

#include "journeyresultswindow.h"

#include "journeydetailswindow.h"
#include "leste.h"

#include "fahrplan.h"
#include "fahrplan_parser_thread.h"

#include <QAction>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QVBoxLayout>

#define T(context, text) QCoreApplication::translate(context, text)

JourneyResultsWindow::JourneyResultsWindow(Fahrplan *fahrplan, QWidget *parent)
    : QMainWindow(parent)
    , m_fahrplan(fahrplan)
    , m_searching(false)
{
    Leste::makeStackedWindow(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(T("JourneyResultsPage", "Journey alternatives"));

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
    Leste::enableKineticScrolling(m_list);
    vbox->addWidget(m_list, 1);
    setCentralWidget(content);

    m_earlierAction = menuBar()->addAction(T("JourneyResultsPage", "Earlier"));
    m_laterAction = menuBar()->addAction(T("JourneyResultsPage", "Later"));
    connect(m_earlierAction, SIGNAL(triggered()), SLOT(searchEarlier()));
    connect(m_laterAction, SIGNAL(triggered()), SLOT(searchLater()));
    Leste::useApplicationMenu(this);

    connect(m_list, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(onItemClicked(QListWidgetItem*)));
    connect(m_fahrplan, SIGNAL(parserJourneyResult(JourneyResultList*)), SLOT(onJourneyResult(JourneyResultList*)));
    connect(m_fahrplan, SIGNAL(parserErrorOccured(QString)), SLOT(onParserError(QString)));

    const QString via = m_fahrplan->viaStationName();
    const bool hasVia = m_fahrplan->parser()->supportsVia() && via != T("Fahrplan", "please select");
    if (hasVia)
        m_header->setText(T("MainPage", "<b>%1</b> via <b>%3</b> to <b>%2</b>")
                          .arg(Leste::esc(m_fahrplan->departureStationName()),
                               Leste::esc(m_fahrplan->arrivalStationName()),
                               Leste::esc(via)));
    else
        m_header->setText(T("MainPage", "<b>%1</b> to <b>%2</b>")
                          .arg(Leste::esc(m_fahrplan->departureStationName()),
                               Leste::esc(m_fahrplan->arrivalStationName())));
}

void JourneyResultsWindow::start()
{
    setSearching(true);
    m_fahrplan->searchJourney();
}

void JourneyResultsWindow::closeEvent(QCloseEvent *event)
{
    if (m_searching)
        m_fahrplan->parser()->cancelRequest();
    QMainWindow::closeEvent(event);
}

void JourneyResultsWindow::setSearching(bool searching)
{
    m_searching = searching;
    Leste::setBusy(this, searching);
    m_earlierAction->setEnabled(!searching);
    m_laterAction->setEnabled(!searching);
    if (searching) {
        m_status->setText(T("JourneyResultsPage", "Searching..."));
        m_status->show();
    } else {
        m_status->hide();
    }
}

void JourneyResultsWindow::onJourneyResult(JourneyResultList *result)
{
    // A details window on top of this one has its own requests.
    if (!m_searching)
        return;
    setSearching(false);
    m_list->clear();

    if (!result) {
        m_status->setText(T("MainPage", "No results found"));
        m_status->show();
        return;
    }

    const QString via = result->viaStation();
    QString header = via.isEmpty()
            ? T("MainPage", "<b>%1</b> to <b>%2</b>").arg(Leste::esc(result->departureStation()),
                                                         Leste::esc(result->arrivalStation()))
            : T("MainPage", "<b>%1</b> via <b>%3</b> to <b>%2</b>").arg(Leste::esc(result->departureStation()),
                                                                       Leste::esc(result->arrivalStation()),
                                                                       Leste::esc(via));
    if (!result->timeInfo().isEmpty())
        header += QLatin1String("<br>") + Leste::esc(result->timeInfo());
    m_header->setText(header);

    const QString grey = Leste::secondaryTextColor();
    const QString active = Leste::activeTextColor();
    const int count = int(result->itemcount());
    for (int i = 0; i < count; ++i) {
        JourneyResultItem *item = result->getItem(i);
        QString html = QString::fromLatin1("<b>%1 &ndash; %2</b>&nbsp;&nbsp; %3: %4 &nbsp; %5: %6")
                .arg(Leste::esc(item->departureTime()), Leste::esc(item->arrivalTime()),
                     T("JourneyResultsPage", "Dur."), Leste::esc(item->duration()),
                     T("JourneyResultsPage", "Trans."), Leste::esc(item->transfers()));
        if (!item->trainType().isEmpty())
            html += QString::fromLatin1("<br><font color=\"%1\">%2</font>").arg(grey, Leste::esc(item->trainType()));
        if (!item->miscInfo().isEmpty())
            html += QString::fromLatin1("<br><small><font color=\"%1\">%2</font></small>").arg(active, item->miscInfo());
        QListWidgetItem *row = new QListWidgetItem(html, m_list);
        row->setData(Qt::UserRole, item->id());
        row->setData(Qt::UserRole + 1, item->departureTime());
        row->setData(Qt::UserRole + 2, item->arrivalTime());
    }

    if (count == 0) {
        m_status->setText(T("MainPage", "No results found"));
        m_status->show();
    }
}

void JourneyResultsWindow::onParserError(const QString &message)
{
    if (!m_searching)
        return;
    setSearching(false);
    m_status->setText(message);
    m_status->show();
    Leste::showBanner(this, message);
}

void JourneyResultsWindow::onItemClicked(QListWidgetItem *item)
{
    if (m_searching || m_details)
        return;
    m_details = new JourneyDetailsWindow(m_fahrplan, this);
    m_details->show();
    m_details->start(item->data(Qt::UserRole).toString());
}

void JourneyResultsWindow::searchEarlier()
{
    setSearching(true);
    m_fahrplan->parser()->searchJourneyEarlier();
}

void JourneyResultsWindow::searchLater()
{
    setSearching(true);
    m_fahrplan->parser()->searchJourneyLater();
}
