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

#include "parser_abstract.h"


void parserAbstract::httpDataReadProgress(int done, int total)
{
    qDebug() << "httpDataReadProgress:" << done << "/" << total;
    progress = done;
    progressTotal = total;
}



void parserAbstract::httpRequestFinished(int requestId, bool error)
{
    Q_UNUSED(error);
    if(currentRequestId != requestId) return;
    loop.exit();
}


void parserAbstract::httpRequestTimeout()
{
    loop.exit();
}




//void parserAbstract::NetworkManagerInit(void)
//{
//    qDebug() << "NetworkManager init";


//}





//void parserAbstract::replyFinished(QNetworkReply*)
//{
//    qDebug() << "NetworkManager reply finished";
//    loop.exit();
//}














bool parserAbstract::supportsGps()
{
    return false;
}

QStringList parserAbstract::getStationsByName(QString stationName)
{
    Q_UNUSED(stationName);
    QStringList result;
    return result;
}

QStringList parserAbstract::getTrainRestrictions()
{
    QStringList result;
    return result;
}

QStringList parserAbstract::getStationsByGPS(qreal latitude, qreal longitude)
{
    Q_UNUSED(latitude);
    Q_UNUSED(longitude);
    QStringList result;
    return result;
}

ResultInfo parserAbstract::getJourneyData(QString destinationStation, QString arrivalStation, QString viaStation, QDate date, QTime time, int mode, int trainrestrictions)
{
    Q_UNUSED(destinationStation);
    Q_UNUSED(arrivalStation);
    Q_UNUSED(viaStation);
    Q_UNUSED(date);
    Q_UNUSED(time);
    Q_UNUSED(mode);
    Q_UNUSED(trainrestrictions);
    ResultInfo result;
    return result;
}

ResultInfo parserAbstract::getJourneyData(QString queryUrl)
{
    Q_UNUSED(queryUrl);
    ResultInfo result;
    return result;
}

DetailResultInfo parserAbstract::getJourneyDetailsData(QString queryUrl)
{
    Q_UNUSED(queryUrl);
    DetailResultInfo result;
    return result;
}
