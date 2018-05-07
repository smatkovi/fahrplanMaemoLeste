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

#ifndef PARSER_ABSTRACT_H
#define PARSER_ABSTRACT_H

#include <QObject>
#include <QtNetwork>

//#include <QNetworkAccessManager>

//#include "mainwindow.h"

struct DetailResultItem
{
    QString fromStation;
    QString fromInfo;
    QDateTime fromTime;
    QString toStation;
    QString toInfo;
    QDateTime toTime;
    QString train;
    QString info;
};

struct DetailResultInfo
{
    QString duration;
    QString info;
    QList<DetailResultItem> items;
};

struct ResultItem
{
    QString id;
    QString fromTime;
    QString toTime;
    QString trainType;
    QString duration;
    QString changes;
    QString state;
    QString detailsUrl;
    QDate tripDate;
    DetailResultInfo detailsInfo;
};

struct ResultInfo
{
    QString errorMsg;
    QString fromStation;
    QString toStation;
    QString timeInfo;
    QString earlierUrl;
    QString laterUrl;
    QList<ResultItem> items;
};

class parserAbstract : public QObject
{
    Q_OBJECT
public:
    virtual QStringList getStationsByName(QString stationName);
    virtual QStringList getTrainRestrictions();
    virtual QStringList getStationsByGPS(qreal latitude, qreal longitude);
    virtual ResultInfo getJourneyData(QString destinationStation, QString arrivalStation, QString viaStation, QDate date, QTime time, int mode, int trainrestrictions);
    virtual ResultInfo getJourneyData(QString queryUrl);
    virtual DetailResultInfo getJourneyDetailsData(QString queryUrl);
    virtual void httpRequestTimeout();

//    void NetworkManagerInit(void);

    virtual bool supportsGps();
signals:

public slots:

protected:
    int currentRequestId;
    QEventLoop  loop;

public:
    int progress;
    int progressTotal;


private slots:
    void httpRequestFinished(int requestId, bool error);
    void httpDataReadProgress(int done, int total);

//    void replyFinished(QNetworkReply*);

};





#endif // PARSER_ABSTRACT_H
