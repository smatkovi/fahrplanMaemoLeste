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

#include "stationselectwindow.h"

#include "gpslocation.h"
#include "leste.h"

#include "fahrplan_parser_thread.h"
#include "models/favorites.h"
#include "models/stationsearchresults.h"

#include <QAction>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QVBoxLayout>

#define T(context, text) QCoreApplication::translate(context, text)

StationSelectWindow::StationSelectWindow(Fahrplan *fahrplan, Fahrplan::StationType type, QWidget *parent)
    : QMainWindow(parent)
    , m_fahrplan(fahrplan)
    , m_type(type)
    , m_showingFavorites(true)
    , m_searching(false)
    , m_gps(new GpsLocation(this))
{
    Leste::makeStackedWindow(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(T("StationSelectPage", "Select station"));

    QWidget *content = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(content);

    QHBoxLayout *searchRow = new QHBoxLayout;
    m_edit = new QLineEdit(content);
    m_edit->setPlaceholderText(T("StationSelectPage", "Search for Station..."));
    m_edit->setMinimumHeight(Leste::rowHeight(this));
    m_searchButton = new QPushButton(T("StationSelectPage", "Search"), content);
    m_searchButton->setMinimumHeight(Leste::rowHeight(this));
    searchRow->addWidget(m_edit, 1);
    searchRow->addWidget(m_searchButton);
    vbox->addLayout(searchRow);

    m_hint = new QLabel(content);
    m_hint->setWordWrap(true);
    m_hint->setAlignment(Qt::AlignCenter);
    vbox->addWidget(m_hint);

    m_list = new QListWidget(content);
    m_list->setItemDelegate(new RichItemDelegate(m_list));
    m_list->setResizeMode(QListView::Adjust);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    Leste::enableKineticScrolling(m_list);
    vbox->addWidget(m_list, 1);
    setCentralWidget(content);

    connect(m_edit, SIGNAL(returnPressed()), SLOT(search()));
    connect(m_edit, SIGNAL(textChanged(QString)), SLOT(onTextChanged(QString)));
    connect(m_searchButton, SIGNAL(clicked()), SLOT(search()));
    connect(m_list, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(onItemClicked(QListWidgetItem*)));
    connect(m_list, SIGNAL(customContextMenuRequested(QPoint)), SLOT(onContextMenu(QPoint)));
    connect(m_fahrplan, SIGNAL(parserStationsResult()), SLOT(onStationsResult()));
    connect(m_fahrplan, SIGNAL(parserErrorOccured(QString)), SLOT(onParserError(QString)));
    connect(m_gps, SIGNAL(positionFound(qreal,qreal)), SLOT(onPositionFound(qreal,qreal)));
    connect(m_gps, SIGNAL(positionError(QString)), SLOT(onPositionError(QString)));

    // Hildon application menu
    QAction *favorites = menuBar()->addAction(T("StationSelectPage", "Favorites"));
    connect(favorites, SIGNAL(triggered()), SLOT(showFavorites()));
    m_nearbyAction = menuBar()->addAction(tr("Stations nearby (GPS)"));
    m_nearbyAction->setVisible(m_fahrplan->parser()->supportsGps());
    connect(m_nearbyAction, SIGNAL(triggered()), SLOT(searchNearby()));
    QAction *clear = menuBar()->addAction(T("MainPage", "Clear station"));
    connect(clear, SIGNAL(triggered()), SLOT(clearStation()));
    Leste::useApplicationMenu(this);

    showFavorites();
    m_edit->setFocus();
}

void StationSelectWindow::closeEvent(QCloseEvent *event)
{
    if (m_searching)
        m_fahrplan->parser()->cancelRequest();
    m_gps->cancel();
    QMainWindow::closeEvent(event);
}

void StationSelectWindow::setSearching(bool searching, const QString &message)
{
    m_searching = searching;
    Leste::setBusy(this, searching);
    m_searchButton->setEnabled(!searching);
    m_hint->setText(message);
    m_hint->setVisible(!message.isEmpty());
}

void StationSelectWindow::fill(StationsListModel *model, bool favorites)
{
    m_showingFavorites = favorites;
    m_list->clear();
    const QString grey = Leste::secondaryTextColor();
    const QString star = Leste::activeTextColor();
    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        QString html = Leste::esc(model->data(index, StationsListModel::Name).toString());
        if (!favorites && model->data(index, StationsListModel::IsFavorite).toBool())
            html = QString::fromLatin1("<font color=\"%1\">&#9733;</font> ").arg(star) + html;
        const QString misc = model->data(index, StationsListModel::MiscInfo).toString();
        if (!misc.isEmpty())
            html += QString::fromLatin1("<br><small><font color=\"%1\">%2</font></small>").arg(grey, Leste::esc(misc));
        QListWidgetItem *item = new QListWidgetItem(html, m_list);
        item->setData(Qt::UserRole, row);
    }
}

void StationSelectWindow::showFavorites()
{
    if (m_searching)
        m_fahrplan->parser()->cancelRequest();
    setSearching(false);
    Favorites *favorites = m_fahrplan->favorites();
    fill(favorites, true);
    if (favorites->rowCount() == 0)
        setSearching(false, T("StationSelectPage", "Click and hold in the search results to add or remove a station as a favorite"));
}

void StationSelectWindow::onTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty() && !m_showingFavorites)
        showFavorites();
}

void StationSelectWindow::search()
{
    const QString text = m_edit->text().trimmed();
    if (text.isEmpty()) {
        showFavorites();
        return;
    }
    m_list->clear();
    setSearching(true, T("StationSelectPage", "Searching for stations..."));
    m_fahrplan->findStationsByName(text);
}

void StationSelectWindow::searchNearby()
{
    m_list->clear();
    m_showingFavorites = false;
    setSearching(true, T("StationSelectPage", "Waiting for GPS lock..."));
    m_gps->requestPosition();
}

void StationSelectWindow::onPositionFound(qreal latitude, qreal longitude)
{
    setSearching(true, T("StationSelectPage", "Searching for stations..."));
    m_fahrplan->findStationsByCoordinates(longitude, latitude);
}

void StationSelectWindow::onPositionError(const QString &message)
{
    setSearching(false);
    Leste::showBanner(this, message);
    showFavorites();
}

void StationSelectWindow::onStationsResult()
{
    if (!m_searching)
        return;
    setSearching(false);
    StationSearchResults *results = m_fahrplan->stationSearchResults();
    fill(results, false);
    if (results->rowCount() == 0)
        setSearching(false, T("MainPage", "No results found"));
}

void StationSelectWindow::onParserError(const QString &message)
{
    if (!m_searching)
        return;
    setSearching(false, message);
    Leste::showBanner(this, message);
}

void StationSelectWindow::onItemClicked(QListWidgetItem *item)
{
    const int row = item->data(Qt::UserRole).toInt();
    if (m_showingFavorites)
        m_fahrplan->favorites()->selectStation(m_type, row);
    else
        m_fahrplan->stationSearchResults()->selectStation(m_type, row);
    close();
}

void StationSelectWindow::onContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_list->itemAt(pos);
    if (!item)
        return;
    const int row = item->data(Qt::UserRole).toInt();

    StationsListModel *model = m_showingFavorites
            ? static_cast<StationsListModel *>(m_fahrplan->favorites())
            : static_cast<StationsListModel *>(m_fahrplan->stationSearchResults());
    const QModelIndex index = model->index(row, 0);
    const bool isFavorite = model->data(index, StationsListModel::IsFavorite).toBool();
    const QString name = model->data(index, StationsListModel::Name).toString();

    QMenu menu(this);
    QAction *toggle = menu.addAction(isFavorite ? T("StationDelegate", "Remove from favorites")
                                                : T("StationDelegate", "Add to favorites"));
    if (menu.exec(m_list->viewport()->mapToGlobal(pos)) != toggle)
        return;

    if (m_showingFavorites) {
        m_fahrplan->favorites()->removeFromFavorites(row);
        Leste::showBanner(this, T("StationDelegate", "Removing '%1' from favorites").arg(name));
        showFavorites();
    } else {
        StationSearchResults *results = m_fahrplan->stationSearchResults();
        if (isFavorite) {
            results->removeFromFavorites(row);
            Leste::showBanner(this, T("StationDelegate", "Removing '%1' from favorites").arg(name));
        } else {
            results->addToFavorites(row);
            Leste::showBanner(this, T("StationDelegate", "Adding '%1' to favorites").arg(name));
        }
        fill(results, false);
    }
}

void StationSelectWindow::clearStation()
{
    m_fahrplan->resetStation(m_type);
    close();
}
