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

#ifndef FAHRPLAN_LESTE_JOURNEYRESULTSWINDOW_H
#define FAHRPLAN_LESTE_JOURNEYRESULTSWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QStringList>

class Fahrplan;
class JourneyResultList;
class JourneyDetailsWindow;
class QAction;
class QLabel;
class QListWidget;
class QListWidgetItem;

// The connections found for a journey; earlier / later in the app menu.
class JourneyResultsWindow : public QMainWindow
{
    Q_OBJECT

public:
    JourneyResultsWindow(Fahrplan *fahrplan, QWidget *parent);

    // Sends the search with the settings of the main window.
    void start();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void onJourneyResult(JourneyResultList *result);
    void onParserError(const QString &message);
    void onItemClicked(QListWidgetItem *item);
    void searchEarlier();
    void searchLater();

private:
    void setSearching(bool searching);

    Fahrplan *m_fahrplan;
    bool m_searching;
    QLabel *m_header;
    QLabel *m_status;
    QListWidget *m_list;
    QAction *m_earlierAction;
    QAction *m_laterAction;
    QPointer<JourneyDetailsWindow> m_details;
};

#endif // FAHRPLAN_LESTE_JOURNEYRESULTSWINDOW_H
