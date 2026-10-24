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

#ifndef FAHRPLAN_LESTE_FAHRPLANWINDOW_H
#define FAHRPLAN_LESTE_FAHRPLANWINDOW_H

#include <QMainWindow>
#include <QPointer>

#include "fahrplan.h"

class QAction;
class QGridLayout;
class QPushButton;
class QStandardItemModel;
class QMaemo5ValueButton;
class QMaemo5ListPickSelector;
class QMaemo5DatePickSelector;
class QMaemo5TimePickSelector;

// The main window, laid out like the Fahrplan of Maemo 5: picker buttons for
// the stations, date, time and options, and a search button. Results and
// details open as stacked windows on top of it.
class FahrplanWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FahrplanWindow(QWidget *parent = 0);

protected:
    void resizeEvent(QResizeEvent *event);

private slots:
    void onParserChanged(const QString &name, int index);
    void onParserError(const QString &message);
    void updateStationButtons();
    void updateDateTimeButtons();
    void onBackendSelected();
    void onDateSelected();
    void onTimeSelected();
    void onModeSelected();
    void onTrainsSelected();
    void selectStation();
    void swapDepartureArrival();
    void clearStation();
    void swapStation();
    void search();
    void setJourneyMode();
    void setTimetableMode();
    void showAbout();

private:
    QMaemo5ValueButton *createStationButton(const QString &title, Fahrplan::StationType type);
    void updateVisibility();
    void relayout();
    bool isSelected(const QString &stationName) const;

    Fahrplan *m_fahrplan;
    bool m_timetableMode;
    bool m_portrait;
    bool m_updating;

    QMaemo5ValueButton *m_backendButton;
    QMaemo5ValueButton *m_departureButton;
    QMaemo5ValueButton *m_viaButton;
    QMaemo5ValueButton *m_arrivalButton;
    QMaemo5ValueButton *m_currentButton;
    QMaemo5ValueButton *m_directionButton;
    QMaemo5ValueButton *m_dateButton;
    QMaemo5ValueButton *m_timeButton;
    QMaemo5ValueButton *m_modeButton;
    QMaemo5ValueButton *m_trainsButton;
    QPushButton *m_swapButton;
    QPushButton *m_searchButton;

    QMaemo5ListPickSelector *m_backendPicker;
    QMaemo5ListPickSelector *m_modePicker;
    QMaemo5ListPickSelector *m_trainsPicker;
    QMaemo5DatePickSelector *m_datePicker;
    QMaemo5TimePickSelector *m_timePicker;
    QStandardItemModel *m_backendModel;
    QStandardItemModel *m_modeModel;
    QStandardItemModel *m_trainsModel;

    QGridLayout *m_stationsGrid;
    QGridLayout *m_optionsGrid;
    QAction *m_journeyAction;
    QAction *m_timetableAction;
    QPointer<QWidget> m_child;
};

#endif // FAHRPLAN_LESTE_FAHRPLANWINDOW_H
