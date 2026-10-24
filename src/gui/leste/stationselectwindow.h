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

#ifndef FAHRPLAN_LESTE_STATIONSELECTWINDOW_H
#define FAHRPLAN_LESTE_STATIONSELECTWINDOW_H

#include <QMainWindow>

#include "fahrplan.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class GpsLocation;
class StationsListModel;

// Picks a station: favorites first, a search by name, or the stations near
// the current position.
class StationSelectWindow : public QMainWindow
{
    Q_OBJECT

public:
    StationSelectWindow(Fahrplan *fahrplan, Fahrplan::StationType type, QWidget *parent);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void search();
    void searchNearby();
    void onTextChanged(const QString &text);
    void onStationsResult();
    void onParserError(const QString &message);
    void onItemClicked(QListWidgetItem *item);
    void onContextMenu(const QPoint &pos);
    void onPositionFound(qreal latitude, qreal longitude);
    void onPositionError(const QString &message);
    void showFavorites();
    void clearStation();

private:
    void fill(StationsListModel *model, bool favorites);
    void setSearching(bool searching, const QString &message = QString());

    Fahrplan *m_fahrplan;
    Fahrplan::StationType m_type;
    bool m_showingFavorites;
    bool m_searching;
    QLineEdit *m_edit;
    QPushButton *m_searchButton;
    QLabel *m_hint;
    QListWidget *m_list;
    GpsLocation *m_gps;
    QAction *m_nearbyAction;
};

#endif // FAHRPLAN_LESTE_STATIONSELECTWINDOW_H
