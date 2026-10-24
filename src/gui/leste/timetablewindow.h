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

#ifndef FAHRPLAN_LESTE_TIMETABLEWINDOW_H
#define FAHRPLAN_LESTE_TIMETABLEWINDOW_H

#include <QMainWindow>

class Fahrplan;
class QLabel;
class QListWidget;

// The departures (or arrivals) of one station.
class TimetableWindow : public QMainWindow
{
    Q_OBJECT

public:
    TimetableWindow(Fahrplan *fahrplan, QWidget *parent);

    void start();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void onTimetableResult();
    void onParserError(const QString &message);
    void refresh();

private:
    void setSearching(bool searching);

    Fahrplan *m_fahrplan;
    bool m_searching;
    QLabel *m_header;
    QLabel *m_status;
    QListWidget *m_list;
};

#endif // FAHRPLAN_LESTE_TIMETABLEWINDOW_H
