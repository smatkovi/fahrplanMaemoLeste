/*******************************************************************************

    This file is a part of Fahrplan for maemo 2009-2010

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#ifdef Q_WS_MAEMO_5
#include <QtMaemo5>
    #include <QMaemo5InformationBox>
    #include <qgtkstyle.h>
    #include <qgeopositioninfo.h>
    #include <qgeopositioninfosource.h>
    #include "calendardialog.h"
#endif
#include <QtCore/QtGlobal>
#include <QtCore/QtDebug>

#include <QtCore/QCoreApplication>
#include <QtCore/QBuffer>
#include <QtCore/QStringList>
#include <QCloseEvent>
#include "parser_abstract.h"
#include "parser_mobilebahnde.h"
#include "parser_mvvde.h"
#include "parser_sbbch.h"
#include "parser_131500comau.h"
#include "parser_hafasxml.h"
#include "parser_xmloebbat.h"
#include "parser_xmlrejseplanendk.h"
#include "parser_xmlsbbch.h"
#include "parser_translink.h"
#include "selectstationdialog.h"
#include "configdialog.h"
#include "qvaluebutton.h"

#define APP_VER "0.0.37"
#define APP_FIRSTLAUNCH false

#ifdef Q_WS_MAEMO_5
    //Use the QtMobility namespace
    QTM_USE_NAMESPACE
#endif

class ResultItemUserData : public QObjectUserData
{
public:
    ResultItem item;
};

class ModeButtonUserData : public QObjectUserData
{
public:
    int mode;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.nokia.fahrplan")

public:
    explicit MainWindow(QWidget *parent=0);
    void initBackend(bool resetStations = false);

public slots:
    void positionUpdated(QGeoPositionInfo geoPositionInfo);
    Q_SCRIPTABLE int top_application();
    void orientationChanged();
    void updateProgress();

private slots:
    void searchButtonClicked(bool checked = false);
    void aboutButtonClicked();
    void donateButtonClicked();
    void alarmButtonClicked();
    void alarmDefaultButtonClicked();
    void settingsButtonClicked();
    void earlierButtonClicked();
    void laterButtonClicked();
    void stationDepartureClicked();
    void stationArrivalClicked();
    void swapClicked();
    void modeClicked();
    void gpsButtonClicked();
    void windowDestroyed(QObject* obj = 0);
    void resultItemClicked();

private:
    QAction *gpsAct;
    QPushButton        *searchButton;
    QValueButton       *stationDeparture;
    QValueButton       *stationArrival;
    QPushButton *stationSwap;
    QMaemo5ValueButton *dateSelector;
    QMaemo5ValueButton *timeSelector;
    QValueButton *modeButton;
    QValueButton *traintypesButton;
    ResultInfo         lastSearchResult;
    DetailResultInfo   lastSearchDetails;
    QPointer<QMainWindow> resultWindow;
    QPointer<QMainWindow> detailsWindow;
    QGeoPositionInfo myPositionInfo;
    QPointer<QGeoPositionInfoSource> locationDataSource;
    QSettings          *settings;
    QDial *progressbar;
    QDial *progressbarResult;
    QScrollArea *scrollAreaResult;
    QTimer *timer;
    bool timeout;
    bool locationDataSourceIsUpdating;


    parserAbstract *backendParser;

    bool detailsLoading;
    void closeEvent(QCloseEvent *event);
    void showResultWindow();
    void showDetailsWindow();
    bool eventFilter(QObject* pObject, QEvent* pEvent);
};

#endif // MAINWINDOW_H
