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

#ifndef FAHRPLAN_LESTE_JOURNEYDETAILSWINDOW_H
#define FAHRPLAN_LESTE_JOURNEYDETAILSWINDOW_H

#include <QMainWindow>

class Fahrplan;
class JourneyDetailResultList;
class QLabel;
class QListWidget;

// The legs of one connection: stations with times and platforms, and the
// trains between them.
class JourneyDetailsWindow : public QMainWindow
{
    Q_OBJECT

public:
    JourneyDetailsWindow(Fahrplan *fahrplan, QWidget *parent);

    void start(const QString &journeyId);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void onDetailsResult(JourneyDetailResultList *result);
    void onParserError(const QString &message);

private:
    void setSearching(bool searching);
    void addStation(const QString &arrival, const QString &departure,
                    const QString &station, const QString &info);
    void addTrain(const QString &train, const QString &direction, const QString &info);

    Fahrplan *m_fahrplan;
    bool m_searching;
    QLabel *m_header;
    QLabel *m_status;
    QListWidget *m_list;
};

#endif // FAHRPLAN_LESTE_JOURNEYDETAILSWINDOW_H
