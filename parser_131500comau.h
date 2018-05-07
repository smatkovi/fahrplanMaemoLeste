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

#ifndef PARSER_131500COMAU_H
#define PARSER_131500COMAU_H

#include <QObject>
#include <QtNetwork>
#include <QtXmlPatterns/QXmlQuery>
#include <QXmlResultItems>
#include "fahrplanutils.h"

#include "parser_abstract.h"

class parser131500ComAu : public parserAbstract
{
    Q_OBJECT
public:
    explicit parser131500ComAu(QObject *parent = 0);
    QStringList getStationsByName(QString stationName);
    QStringList getTrainRestrictions();
    QStringList getStationsByGPS(qreal latitude, qreal longitude);
    ResultInfo getJourneyData(QString destinationStation, QString arrivalStation, QString viaStation, QDate date, QTime time, int mode, int trainrestrictions);
    ResultInfo getJourneyData(QString queryUrl);
    DetailResultInfo getJourneyDetailsData(QString queryUrl);
    bool supportsGps();

  signals:

  public slots:

  private:
      QHttp *http;
      QBuffer *filebuffer;
      ResultInfo parseJourneyData(QByteArray data);
};

#endif // PARSER_131500COMAU_H
