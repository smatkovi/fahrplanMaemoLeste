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

#ifndef SELECTSTATIONDIALOG_H
#define SELECTSTATIONDIALOG_H

#include <QDialog>
#include <QtGui>
#ifdef Q_WS_MAEMO_5
#include <QtMaemo5>
    #include <qgeopositioninfo.h>
    #include <qgeopositioninfosource.h>
#endif
#include <QLineEdit>
#include "backupgpsopenstreetmap.h"
#include "fahrplanutils.h"
#include "parser_abstract.h"
#include "qvaluebutton.h"

#ifdef Q_WS_MAEMO_5
//Use the QtMobility namespace
QTM_USE_NAMESPACE
#endif

class selectStationDialog : public QDialog
{
    Q_OBJECT

public:
    #ifdef Q_WS_MAEMO_5
    explicit selectStationDialog(QValueButton *sender, QGeoPositionInfo *position, parserAbstract *mainBackendParser, QSettings *mainSettings);
    #else
    explicit selectStationDialog(QValueButton *sender, parserAbstract *mainBackendParser, QSettings *mainSettings);
    #endif

signals:

public slots:
    void searchButtonClicked(bool checked);
    void gpsButtonClicked(bool checked);
    void stationSelected(QString value);
    void orientationChanged();
    void updateProgress();

private:
    QComboBox   *searchboxCombo;
    QLineEdit   *searchboxText;
    QPushButton *gpsButton;
    QPushButton *searchButton;
    QValueButton *mainSender;
    QDial *progressbar;
    QTimer *timer;
    int progress;
    #ifdef Q_WS_MAEMO_5
    QGeoPositionInfo *myPositionInfo;
    #endif
    QSettings          *settings;
    QStandardItemModel *stationsModel;

    void showSelector(QStringList items);
    void closeEvent(QCloseEvent *event);

    parserAbstract *backendParser;
};

#endif // SELECTSTATIONDIALOG_H
