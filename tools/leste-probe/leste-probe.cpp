/****************************************************************************
**
**  This file is a part of Fahrplan.
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
****************************************************************************/

// Drives one backend exactly like the user interface does -- through the
// Fahrplan object, whose parser runs in its own thread -- and prints what
// comes back. On the device it shows whether a backend works there (TLS,
// the service, the parsing) without having to tap through the GUI:
//
//   leste-probe --list
//   leste-probe <backend> "<from>" "<to>"         journey + details of the first
//   leste-probe --timetable <backend> "<station>"
//
// <backend> is the number from --list or a part of its name ("Austria").
// The settings (favourites, last stations) of the application are not touched:
// the probe uses its own settings file.

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QSettings>
#include <QStringList>
#include <QTextStream>
#include <QTimer>

#include <cstdio>

#include "fahrplan.h"
#include "fahrplan_parser_thread.h"
#include "models/backends.h"
#include "models/stationsearchresults.h"
#include "models/timetable.h"

static QTextStream out(stdout);

class Probe : public QObject
{
    Q_OBJECT

public:
    Probe(Fahrplan *fahrplan, const QStringList &stations, bool timetable)
        : m_fahrplan(fahrplan), m_stations(stations), m_timetable(timetable), m_step(0)
    {
        connect(m_fahrplan, SIGNAL(parserStationsResult()), SLOT(onStations()));
        connect(m_fahrplan, SIGNAL(parserJourneyResult(JourneyResultList*)), SLOT(onJourney(JourneyResultList*)));
        connect(m_fahrplan, SIGNAL(parserJourneyDetailsResult(JourneyDetailResultList*)), SLOT(onDetails(JourneyDetailResultList*)));
        connect(m_fahrplan, SIGNAL(parserTimeTableResult()), SLOT(onTimetable()));
        connect(m_fahrplan, SIGNAL(parserErrorOccured(QString)), SLOT(onError(QString)));
        m_clock.start();
    }

public slots:
    void start()
    {
        nextStation();
    }

private slots:
    void onStations()
    {
        StationSearchResults *results = m_fahrplan->stationSearchResults();
        out << "  " << results->rowCount() << " stations (" << m_clock.restart() << " ms)" << endl;
        for (int i = 0; i < qMin(5, results->rowCount()); ++i)
            out << "    " << results->data(results->index(i, 0), StationsListModel::Name).toString() << endl;
        if (results->rowCount() == 0) {
            out << "FAIL: no station found" << endl;
            QCoreApplication::exit(2);
            return;
        }
        Fahrplan::StationType type;
        if (m_timetable)
            type = Fahrplan::CurrentStation;
        else
            type = (m_step == 1) ? Fahrplan::DepartureStation : Fahrplan::ArrivalStation;
        results->selectStation(type, 0);
        nextStation();
    }

    void onJourney(JourneyResultList *result)
    {
        const int count = result ? int(result->itemcount()) : 0;
        out << "Journeys " << (result ? result->departureStation() : QString()) << " -> "
            << (result ? result->arrivalStation() : QString()) << ": " << count
            << " (" << m_clock.restart() << " ms)" << endl;
        for (int i = 0; i < count; ++i) {
            JourneyResultItem *item = result->getItem(i);
            out << "  " << item->departureTime() << " - " << item->arrivalTime()
                << "  " << item->duration() << "  transfers " << item->transfers()
                << "  " << item->trainType();
            if (!item->miscInfo().isEmpty())
                out << "  [" << item->miscInfo() << "]";
            out << endl;
        }
        if (count == 0) {
            out << "FAIL: no journey" << endl;
            QCoreApplication::exit(3);
            return;
        }
        out << "Details of the first journey:" << endl;
        m_fahrplan->parser()->getJourneyDetails(result->getItem(0)->id());
    }

    void onDetails(JourneyDetailResultList *result)
    {
        const int count = result ? int(result->itemcount()) : 0;
        out << "  " << count << " legs (" << m_clock.restart() << " ms), duration " << (result ? result->duration() : QString()) << endl;
        for (int i = 0; i < count; ++i) {
            JourneyDetailResultItem *item = result->getItem(i);
            out << "  " << item->departureDateTime().toString("hh:mm") << " " << item->departureStation()
                << " [" << item->departureInfo() << "]  --" << item->train() << "-->  "
                << item->arrivalDateTime().toString("hh:mm") << " " << item->arrivalStation()
                << " [" << item->arrivalInfo() << "]" << endl;
        }
        out << (count > 0 ? "OK" : "FAIL: no details") << endl;
        QCoreApplication::exit(count > 0 ? 0 : 4);
    }

    void onTimetable()
    {
        Timetable *timetable = m_fahrplan->timetable();
        out << "Departures: " << timetable->rowCount() << " (" << m_clock.restart() << " ms)" << endl;
        for (int i = 0; i < timetable->rowCount(); ++i) {
            const QModelIndex index = timetable->index(i, 0);
            out << "  " << timetable->data(index, Timetable::Time).toTime().toString("hh:mm")
                << "  " << timetable->data(index, Timetable::TrainType).toString()
                << "  -> " << timetable->data(index, Timetable::DestinationStation).toString()
                << "  " << timetable->data(index, Timetable::Platform).toString() << endl;
        }
        out << (timetable->rowCount() > 0 ? "OK" : "FAIL: empty timetable") << endl;
        QCoreApplication::exit(timetable->rowCount() > 0 ? 0 : 5);
    }

    void onError(const QString &message)
    {
        out << "FAIL: error: " << message << endl;
        QCoreApplication::exit(1);
    }

private:
    void nextStation()
    {
        if (m_step < m_stations.count()) {
            out << "Station search \"" << m_stations.at(m_step) << "\"" << endl;
            m_fahrplan->findStationsByName(m_stations.at(m_step));
            ++m_step;
            return;
        }
        m_fahrplan->setMode(Fahrplan::NowMode);
        if (m_timetable) {
            out << "Timetable of " << m_fahrplan->currentStationName() << endl;
            m_fahrplan->getTimeTable();
        } else {
            out << "Journey search " << m_fahrplan->departureStationName() << " -> "
                << m_fahrplan->arrivalStationName() << endl;
            m_fahrplan->searchJourney();
        }
    }

    Fahrplan *m_fahrplan;
    QStringList m_stations;
    bool m_timetable;
    int m_step;
    QTime m_clock;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    // Keep the favourites and stations of the application out of this.
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, QDir::tempPath() + "/leste-probe-settings");

    // as in main.cpp: the parser thread hands these over queued
    qRegisterMetaType<Station>();
    qRegisterMetaType<StationsList>();
    qRegisterMetaType<TimetableEntry>();
    qRegisterMetaType<TimetableEntriesList>();
    qRegisterMetaType<Fahrplan::StationType>();
    qRegisterMetaType<Fahrplan::Mode>();

    QStringList args = app.arguments().mid(1);
    const bool timetable = args.contains("--timetable");
    args.removeAll("--timetable");

    Fahrplan fahrplan;
    Backends *backends = fahrplan.backends();

    if (args.isEmpty() || args.first() == "--list") {
        // What each backend can do: via station, time table, direction, GPS
        out << "id  via tt  dir gps  name" << endl;
        for (int i = 0; i < backends->count(); ++i) {
            const int id = backends->getParserIdForItemIndex(i);
            fahrplan.setParser(id);
            FahrplanParserThread *parser = fahrplan.parser();
            out << QString::number(id).leftJustified(4)
                << (parser->supportsVia() ? "x   " : "-   ")
                << (parser->supportsTimeTable() ? "x   " : "-   ")
                << (parser->supportsTimeTableDirection() ? "x   " : "-   ")
                << (parser->supportsGps() ? "x    " : "-    ")
                << backends->data(backends->index(i, 0), Backends::Name).toString() << endl;
        }
        return 0;
    }
    if (args.count() < (timetable ? 2 : 3)) {
        out << "usage: leste-probe <backend> <from> <to> | --timetable <backend> <station> | --list" << endl;
        return 64;
    }

    bool isNumber = false;
    int parserId = args.first().toInt(&isNumber);
    if (!isNumber) {
        parserId = -1;
        for (int i = 0; i < backends->count(); ++i) {
            if (backends->data(backends->index(i, 0), Backends::Name).toString().contains(args.first(), Qt::CaseInsensitive)) {
                parserId = backends->getParserIdForItemIndex(i);
                break;
            }
        }
    }
    if (parserId < 0) {
        out << "unknown backend " << args.first() << endl;
        return 64;
    }

    fahrplan.setParser(parserId);
    out << "Backend: " << fahrplan.parserName() << " (" << parserId << ")" << endl;

    Probe probe(&fahrplan, args.mid(1), timetable);
    QTimer::singleShot(0, &probe, SLOT(start()));
    QTimer::singleShot(120000, &app, SLOT(quit()));
    const int result = app.exec();
    out.flush();
    return result;
}

#include "leste-probe.moc"
