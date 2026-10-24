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

#include "fahrplanwindow.h"

#include "leste.h"
#include "stationselectwindow.h"
#include "journeyresultswindow.h"
#include "timetablewindow.h"

#include "fahrplan_parser_thread.h"
#include "models/backends.h"
#include "models/trainrestrictions.h"

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QGridLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include <QMaemo5DatePickSelector>
#include <QMaemo5ListPickSelector>
#include <QMaemo5TimePickSelector>
#include <QMaemo5ValueButton>

// The strings of the Harmattan pages are already translated into many
// languages; use the same contexts so that those translations apply here.
#define T(context, text) QCoreApplication::translate(context, text)

static QStandardItem *centeredItem(const QString &text)
{
    QStandardItem *item = new QStandardItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    return item;
}

FahrplanWindow::FahrplanWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_fahrplan(new Fahrplan(this))
    , m_timetableMode(false)
    , m_portrait(false)
    , m_updating(false)
{
    Leste::makeStackedWindow(this);
    setWindowTitle(QLatin1String("Fahrplan"));

    // Backend
    m_backendModel = new QStandardItemModel(this);
    Backends *backends = m_fahrplan->backends();
    for (int i = 0; i < backends->count(); ++i)
        m_backendModel->appendRow(centeredItem(backends->data(backends->index(i, 0), Backends::Name).toString()));
    m_backendPicker = new QMaemo5ListPickSelector(this);
    m_backendPicker->setModel(m_backendModel);
    m_backendButton = new QMaemo5ValueButton(T("SettingsPage", "Backend"), this);
    m_backendButton->setValueLayout(QMaemo5ValueButton::ValueUnderText);
    m_backendButton->setPickSelector(m_backendPicker);
    connect(m_backendPicker, SIGNAL(selected(QString)), SLOT(onBackendSelected()));

    // Stations
    m_departureButton = createStationButton(T("MainPage", "Departure Station"), Fahrplan::DepartureStation);
    m_viaButton = createStationButton(T("MainPage", "Via Station"), Fahrplan::ViaStation);
    m_arrivalButton = createStationButton(T("MainPage", "Arrival Station"), Fahrplan::ArrivalStation);
    m_currentButton = createStationButton(T("MainPage", "Station"), Fahrplan::CurrentStation);
    m_directionButton = createStationButton(T("MainPage", "Direction"), Fahrplan::DirectionStation);

    m_swapButton = new QPushButton(QString(QChar(0x21C5)), this);   // ⇅
    QFont swapFont = m_swapButton->font();
    swapFont.setPointSizeF(swapFont.pointSizeF() * 1.6);
    m_swapButton->setFont(swapFont);
    m_swapButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    connect(m_swapButton, SIGNAL(clicked()), SLOT(swapDepartureArrival()));

    // Date and time
    m_datePicker = new QMaemo5DatePickSelector(this);
    m_dateButton = new QMaemo5ValueButton(T("MainPage", "Date"), this);
    m_dateButton->setValueLayout(QMaemo5ValueButton::ValueUnderText);
    m_dateButton->setPickSelector(m_datePicker);
    connect(m_datePicker, SIGNAL(selected(QString)), SLOT(onDateSelected()));

    m_timePicker = new QMaemo5TimePickSelector(this);
    m_timeButton = new QMaemo5ValueButton(T("MainPage", "Time"), this);
    m_timeButton->setValueLayout(QMaemo5ValueButton::ValueUnderText);
    m_timeButton->setPickSelector(m_timePicker);
    connect(m_timePicker, SIGNAL(selected(QString)), SLOT(onTimeSelected()));

    // Departure / arrival / now -- in the order of Fahrplan::Mode
    m_modeModel = new QStandardItemModel(this);
    m_modeModel->appendRow(centeredItem(T("MainPage", "Departure")));
    m_modeModel->appendRow(centeredItem(T("MainPage", "Arrival")));
    m_modeModel->appendRow(centeredItem(T("MainPage", "Now")));
    m_modePicker = new QMaemo5ListPickSelector(this);
    m_modePicker->setModel(m_modeModel);
    m_modeButton = new QMaemo5ValueButton(T("MainPage", "Mode"), this);
    m_modeButton->setValueLayout(QMaemo5ValueButton::ValueUnderText);
    m_modeButton->setPickSelector(m_modePicker);
    connect(m_modePicker, SIGNAL(selected(QString)), SLOT(onModeSelected()));

    // Train restrictions of the backend
    m_trainsModel = new QStandardItemModel(this);
    m_trainsPicker = new QMaemo5ListPickSelector(this);
    m_trainsPicker->setModel(m_trainsModel);
    m_trainsButton = new QMaemo5ValueButton(T("MainPage", "Trains"), this);
    m_trainsButton->setValueLayout(QMaemo5ValueButton::ValueUnderText);
    m_trainsButton->setPickSelector(m_trainsPicker);
    connect(m_trainsPicker, SIGNAL(selected(QString)), SLOT(onTrainsSelected()));

    m_searchButton = new QPushButton(T("MainPage", "Start search"), this);
    m_searchButton->setMinimumHeight(Leste::rowHeight(this));
    connect(m_searchButton, SIGNAL(clicked()), SLOT(search()));

    // Layout
    QWidget *content = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(content);
    m_stationsGrid = new QGridLayout;
    m_optionsGrid = new QGridLayout;
    m_stationsGrid->setContentsMargins(0, 0, 0, 0);
    m_optionsGrid->setContentsMargins(0, 0, 0, 0);
    m_stationsGrid->addWidget(m_departureButton, 0, 0);
    m_stationsGrid->addWidget(m_viaButton, 1, 0);
    m_stationsGrid->addWidget(m_arrivalButton, 2, 0);
    m_stationsGrid->addWidget(m_currentButton, 3, 0);
    m_stationsGrid->addWidget(m_directionButton, 4, 0);
    m_stationsGrid->addWidget(m_swapButton, 0, 1, 3, 1);
    m_stationsGrid->setColumnStretch(0, 1);
    vbox->addWidget(m_backendButton);
    vbox->addLayout(m_stationsGrid);
    vbox->addLayout(m_optionsGrid);
    vbox->addWidget(m_searchButton);
    vbox->addStretch(1);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(content);
    Leste::enableKineticScrolling(scrollArea);
    setCentralWidget(scrollArea);

    // Hildon application menu: the two search modes as filters, then actions
    QActionGroup *modes = new QActionGroup(this);
    modes->setExclusive(true);
    m_journeyAction = new QAction(T("main", "Journey"), modes);
    m_journeyAction->setCheckable(true);
    m_journeyAction->setChecked(true);
    connect(m_journeyAction, SIGNAL(triggered()), SLOT(setJourneyMode()));
    m_timetableAction = new QAction(T("main", "Time table"), modes);
    m_timetableAction->setCheckable(true);
    connect(m_timetableAction, SIGNAL(triggered()), SLOT(setTimetableMode()));
    menuBar()->addActions(modes->actions());
    QAction *about = menuBar()->addAction(T("AboutPage", "About"));
    connect(about, SIGNAL(triggered()), SLOT(showAbout()));
    Leste::useApplicationMenu(this);

    // Backend signals
    connect(m_fahrplan, SIGNAL(parserChanged(QString,int)), SLOT(onParserChanged(QString,int)));
    connect(m_fahrplan, SIGNAL(parserErrorOccured(QString)), SLOT(onParserError(QString)));
    connect(m_fahrplan, SIGNAL(departureStationChanged()), SLOT(updateStationButtons()));
    connect(m_fahrplan, SIGNAL(viaStationChanged()), SLOT(updateStationButtons()));
    connect(m_fahrplan, SIGNAL(arrivalStationChanged()), SLOT(updateStationButtons()));
    connect(m_fahrplan, SIGNAL(currentStationChanged()), SLOT(updateStationButtons()));
    connect(m_fahrplan, SIGNAL(directionStationChanged()), SLOT(updateStationButtons()));
    connect(m_fahrplan, SIGNAL(dateTimeChanged()), SLOT(updateDateTimeButtons()));
    connect(m_fahrplan, SIGNAL(modeChanged()), SLOT(updateDateTimeButtons()));

    m_timetableMode = m_fahrplan->getSettingsValue("leste/timetableMode", "false") == QLatin1String("true");
    m_portrait = height() > width();
    relayout();

    // Creates the parser of the stored backend; that emits parserChanged().
    m_fahrplan->parser();
    if (m_fahrplan->mode() == Fahrplan::NowMode)
        m_fahrplan->setDateTime(QDateTime::currentDateTime());
    updateDateTimeButtons();
}

QMaemo5ValueButton *FahrplanWindow::createStationButton(const QString &title, Fahrplan::StationType type)
{
    QMaemo5ValueButton *button = new QMaemo5ValueButton(title, this);
    button->setValueLayout(QMaemo5ValueButton::ValueUnderText);
    button->setProperty("stationType", int(type));
    connect(button, SIGNAL(clicked()), SLOT(selectStation()));

    // Tap and hold: the same choices as the Harmattan context menu
    button->setContextMenuPolicy(Qt::ActionsContextMenu);
    QAction *clear = new QAction(T("MainPage", "Clear station"), button);
    clear->setProperty("stationType", int(type));
    connect(clear, SIGNAL(triggered()), SLOT(clearStation()));
    button->addAction(clear);
    if (type == Fahrplan::DepartureStation || type == Fahrplan::ViaStation || type == Fahrplan::ArrivalStation) {
        struct { Fahrplan::StationType other; const char *text; } swaps[] = {
            { Fahrplan::DepartureStation, QT_TRANSLATE_NOOP("MainPage", "Switch with Departure station") },
            { Fahrplan::ArrivalStation, QT_TRANSLATE_NOOP("MainPage", "Switch with Arrival station") },
            { Fahrplan::ViaStation, QT_TRANSLATE_NOOP("MainPage", "Switch with Via station") },
        };
        for (unsigned i = 0; i < sizeof(swaps) / sizeof(swaps[0]); ++i) {
            if (swaps[i].other == type)
                continue;
            QAction *swap = new QAction(T("MainPage", swaps[i].text), button);
            swap->setProperty("stationType", int(type));
            swap->setProperty("otherStationType", int(swaps[i].other));
            connect(swap, SIGNAL(triggered()), SLOT(swapStation()));
            button->addAction(swap);
        }
    }
    return button;
}

void FahrplanWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    const bool portrait = event->size().height() > event->size().width();
    if (portrait != m_portrait) {
        m_portrait = portrait;
        relayout();
    }
}

void FahrplanWindow::relayout()
{
    QWidget *options[] = { m_dateButton, m_timeButton, m_modeButton, m_trainsButton };
    for (unsigned i = 0; i < 4; ++i)
        m_optionsGrid->removeWidget(options[i]);

    if (m_portrait) {
        for (int i = 0; i < 4; ++i)
            m_optionsGrid->addWidget(options[i], i, 0);
        m_optionsGrid->setColumnStretch(1, 0);
    } else {
        m_optionsGrid->addWidget(m_dateButton, 0, 0);
        m_optionsGrid->addWidget(m_timeButton, 0, 1);
        m_optionsGrid->addWidget(m_modeButton, 1, 0);
        m_optionsGrid->addWidget(m_trainsButton, 1, 1);
        m_optionsGrid->setColumnStretch(0, 1);
        m_optionsGrid->setColumnStretch(1, 1);
    }
}

void FahrplanWindow::updateVisibility()
{
    FahrplanParserThread *parser = m_fahrplan->parser();
    const bool timetable = parser->supportsTimeTable();
    if (!timetable)
        m_timetableMode = false;
    m_timetableAction->setEnabled(timetable);
    m_journeyAction->setChecked(!m_timetableMode);
    m_timetableAction->setChecked(m_timetableMode);

    m_departureButton->setVisible(!m_timetableMode);
    m_arrivalButton->setVisible(!m_timetableMode);
    m_viaButton->setVisible(!m_timetableMode && parser->supportsVia());
    m_swapButton->setVisible(!m_timetableMode);
    m_currentButton->setVisible(m_timetableMode);
    m_directionButton->setVisible(m_timetableMode && parser->supportsTimeTableDirection());
    m_trainsButton->setVisible(m_trainsModel->rowCount() > 0);

    if (m_timetableMode) {
        m_searchButton->setText(m_fahrplan->mode() == Fahrplan::ArrivalMode
                                ? T("MainPage", "Show arrivals")
                                : T("MainPage", "Show departures"));
    } else {
        m_searchButton->setText(T("MainPage", "Start search"));
    }
}

void FahrplanWindow::onParserChanged(const QString &name, int index)
{
    m_updating = true;
    m_backendButton->setValueText(name);
    m_backendPicker->setCurrentIndex(m_fahrplan->backends()->getItemIndexForParserId(index));
    m_backendButton->setValueText(name);

    m_trainsModel->clear();
    Trainrestrictions *restrictions = m_fahrplan->trainrestrictions();
    for (int i = 0; i < restrictions->count(); ++i)
        m_trainsModel->appendRow(centeredItem(restrictions->get(i)));
    if (m_trainsModel->rowCount() > 0) {
        int current = 0;
        const QString currentName = m_fahrplan->trainrestrictionName();
        for (int i = 0; i < restrictions->count(); ++i) {
            if (restrictions->get(i) == currentName) {
                current = i;
                break;
            }
        }
        m_trainsPicker->setCurrentIndex(current);
        m_trainsButton->setValueText(currentName);
    }
    m_updating = false;

    updateStationButtons();
    updateVisibility();
}

void FahrplanWindow::onParserError(const QString &message)
{
    // The stacked windows report the errors of their own requests.
    if (!m_child)
        Leste::showBanner(this, message);
}

bool FahrplanWindow::isSelected(const QString &stationName) const
{
    return stationName != T("Fahrplan", "please select");
}

void FahrplanWindow::updateStationButtons()
{
    m_departureButton->setValueText(m_fahrplan->departureStationName());
    m_viaButton->setValueText(m_fahrplan->viaStationName());
    m_arrivalButton->setValueText(m_fahrplan->arrivalStationName());
    m_currentButton->setValueText(m_fahrplan->currentStationName());
    m_directionButton->setValueText(m_fahrplan->directionStationName());
}

void FahrplanWindow::updateDateTimeButtons()
{
    m_updating = true;
    const QDateTime dateTime = m_fahrplan->dateTime();
    m_datePicker->setCurrentDate(dateTime.date());
    m_timePicker->setCurrentTime(dateTime.time());
    m_dateButton->setValueText(m_datePicker->currentValueText());
    m_timeButton->setValueText(m_timePicker->currentValueText());

    const int mode = int(m_fahrplan->mode());
    m_modePicker->setCurrentIndex(mode);
    m_modeButton->setValueText(m_modeModel->item(mode)->text());
    m_updating = false;
    updateVisibility();
}

void FahrplanWindow::onBackendSelected()
{
    if (m_updating)
        return;
    const int item = m_backendPicker->currentIndex();
    if (item < 0)
        return;
    const int parserId = m_fahrplan->backends()->getParserIdForItemIndex(item);
    m_fahrplan->setParser(parserId);
}

void FahrplanWindow::onDateSelected()
{
    if (m_updating)
        return;
    QDateTime dateTime = m_fahrplan->dateTime();
    dateTime.setDate(m_datePicker->currentDate());
    if (m_fahrplan->mode() == Fahrplan::NowMode)
        m_fahrplan->setMode(Fahrplan::DepartureMode);
    m_fahrplan->setDateTime(dateTime);
    updateDateTimeButtons();
}

void FahrplanWindow::onTimeSelected()
{
    if (m_updating)
        return;
    QDateTime dateTime = m_fahrplan->dateTime();
    dateTime.setTime(m_timePicker->currentTime());
    if (m_fahrplan->mode() == Fahrplan::NowMode)
        m_fahrplan->setMode(Fahrplan::DepartureMode);
    m_fahrplan->setDateTime(dateTime);
    updateDateTimeButtons();
}

void FahrplanWindow::onModeSelected()
{
    if (m_updating)
        return;
    const int mode = m_modePicker->currentIndex();
    if (mode < 0)
        return;
    m_fahrplan->setMode(Fahrplan::Mode(mode));
    if (mode == Fahrplan::NowMode)
        m_fahrplan->setDateTime(QDateTime::currentDateTime());
    updateDateTimeButtons();
}

void FahrplanWindow::onTrainsSelected()
{
    if (m_updating)
        return;
    const int index = m_trainsPicker->currentIndex();
    if (index >= 0)
        m_fahrplan->setTrainrestriction(index);
}

void FahrplanWindow::selectStation()
{
    const int type = sender()->property("stationType").toInt();
    StationSelectWindow *window = new StationSelectWindow(m_fahrplan, Fahrplan::StationType(type), this);
    m_child = window;
    window->show();
}

void FahrplanWindow::swapDepartureArrival()
{
    m_fahrplan->swapStations(Fahrplan::DepartureStation, Fahrplan::ArrivalStation);
}

void FahrplanWindow::clearStation()
{
    m_fahrplan->resetStation(Fahrplan::StationType(sender()->property("stationType").toInt()));
}

void FahrplanWindow::swapStation()
{
    m_fahrplan->swapStations(Fahrplan::StationType(sender()->property("stationType").toInt()),
                             Fahrplan::StationType(sender()->property("otherStationType").toInt()));
}

void FahrplanWindow::search()
{
    if (m_fahrplan->mode() == Fahrplan::NowMode)
        m_fahrplan->setDateTime(QDateTime::currentDateTime());

    if (m_timetableMode) {
        if (!isSelected(m_fahrplan->currentStationName())) {
            Leste::showBanner(this, T("MainPage", "Please select a Station"));
            return;
        }
        TimetableWindow *window = new TimetableWindow(m_fahrplan, this);
        m_child = window;
        window->show();
        window->start();
        return;
    }

    if (!isSelected(m_fahrplan->departureStationName()) || !isSelected(m_fahrplan->arrivalStationName())) {
        Leste::showBanner(this, T("MainPage", "Please select a departure and arrival station."));
        return;
    }
    JourneyResultsWindow *window = new JourneyResultsWindow(m_fahrplan, this);
    m_child = window;
    window->show();
    window->start();
}

void FahrplanWindow::setJourneyMode()
{
    m_timetableMode = false;
    m_fahrplan->storeSettingsValue("leste/timetableMode", "false");
    updateVisibility();
}

void FahrplanWindow::setTimetableMode()
{
    m_timetableMode = true;
    m_fahrplan->storeSettingsValue("leste/timetableMode", "true");
    updateVisibility();
}

void FahrplanWindow::showAbout()
{
    const QString version = m_fahrplan->getVersion();
    QString text;
    text += QLatin1String("<p><b>Fahrplan</b> ") + T("AboutPage", "Version %1").arg(Leste::esc(version)) + QLatin1String("</p>");
    text += QLatin1String("<p>A journey planner and timetable for public transport.<br>"
                          "(C) 2008-2018 smurfy (maemo@smurfy.de) and the Fahrplan contributors.</p>");
    text += QLatin1String("<p>Maintainers: Mark Washeim (poetaster), Philipp Andreas (smurfy), "
                          "Michael Stevens (stevensmi), Oleksii Serdiuk (leppa).<br>"
                          "MeeGo Harmattan and Maemo Leste ports: smatkovi.</p>");
    text += QLatin1String("<p>The transitous.org services are provided with the assistance of "
                          "<a href=\"https://transitous.org/sources/\">transitous sources</a>. "
                          "Transitous and Fahrplan use data from the OpenStreetMap project, see "
                          "<a href=\"https://www.openstreetmap.org/copyright\">copyright and attribution</a>. "
                          "Netherlands journey data from <a href=\"https://9292.nl\">9292</a> "
                          "(REISinformatiegroep B.V.).</p>");
    text += QLatin1String("<p>") + T("AboutPage", "Released under the terms of the GNU GPL v2 or higher") + QLatin1String("<br>")
          + T("AboutPage", "Source code available on %1").arg(QLatin1String("<a href=\"https://github.com/smatkovi/fahrplanMaemoLeste\">github.com/smatkovi/fahrplanMaemoLeste</a>"))
          + QLatin1String("</p>");
    QMessageBox::about(this, T("AboutPage", "About Fahrplan"), text);
}
