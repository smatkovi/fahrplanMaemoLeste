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

#include "parser_mvvde.h"

parserMvvDe::parserMvvDe(QObject *parent)
{
     Q_UNUSED(parent);
     http = new QHttp(this);

     connect(http, SIGNAL(requestFinished(int,bool)), this, SLOT(httpRequestFinished(int,bool)));
     connect(http, SIGNAL(dataReadProgress(int,int)), this, SLOT(httpDataReadProgress(int,int)));
}

bool parserMvvDe::supportsGps()
{
    return false;
}

QStringList parserMvvDe::getTrainRestrictions()
{
    QStringList result;
    /*
     * Have to check, does not work for me
    result.append(tr("All"));
    result.append(tr("All without ICE"));
    result.append(tr("Only local transport and authority lines"));
    result.append(tr("Only authority lines without extra charge"));
    */
    return result;
}

QStringList parserMvvDe::getStationsByName(QString stationName)
{
    QByteArray postData = "sessionID=0&requestID=0&language=en&command=&ptOptionsActive=0&place_origin=&placeState_origin=empty&type_origin=stop&nameState_origin=empty&place_destination=&placeState_destination=empty&type_destination=stop&name_destination=&nameState_destination=empty&place_via=&placeState_via=empty&type_via=stop&name_via=&name_origin=";
    postData.append(stationName);

    qDebug() << "parserMVVDe::getStationsByName";

    QUrl url("http://efa.mvv-muenchen.de/bcl/XSLT_TRIP_REQUEST2?language=en");

    http->setHost(url.host(), QHttp::ConnectionModeHttp, url.port() == -1 ? 0 : url.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    QHttpRequestHeader header;
    header.setRequest("POST", url.path() + "?" + url.encodedQuery());
    header.setValue("Host", url.host());

    currentRequestId = http->request(header, postData, filebuffer);

    loop.exec();

    filebuffer->close();

    QRegExp regexp = QRegExp("<select name=\"name_origin\" acceskey=\"s\">(.*)</select>");
    regexp.setMinimal(true);

    regexp.indexIn(filebuffer->buffer());

    QString element = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body>\n" + regexp.cap(0) + "\n</body>\n</html>\n";

    QBuffer readBuffer;
    readBuffer.setData(element.toAscii());
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    //Query for more than one result
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/select[@name='name_origin']/div/option/string()");

    QStringList result;
    if (!query.evaluateTo(&result))
    {
        qDebug() << "parserMVVDe::getStationsByName - Query 1 Failed";
    }

    //One Entry only
    if (result.count() == 0)
    {
        QRegExp regexp = QRegExp("<input type=\"hidden\" name=\"nameState_origin\" value=\"identified\">(.*)</div>");
        regexp.setMinimal(true);
        regexp.indexIn(filebuffer->buffer());
        if (regexp.cap(1) != "")
        {
            result.append(regexp.cap(1));
        }
    }

    delete filebuffer;

    return result;
}

QStringList parserMvvDe::getStationsByGPS(qreal latitude, qreal longitude)
{
    //No GPS available on MVV.de
    Q_UNUSED(latitude);
    Q_UNUSED(longitude);
    QStringList result;
    return result;
}

ResultInfo parserMvvDe::getJourneyData(QString destinationStation, QString arrivalStation, QString viaStation, QDate date, QTime time, int mode, int trainrestrictions)
{
    Q_UNUSED(viaStation);

    if (destinationStation == "München")
    {
        destinationStation = "M";
    }
    if (arrivalStation == "München")
    {
        arrivalStation = "M";
    }

    QString trainrestr = "400";
    if (trainrestrictions == 0) {
        trainrestr = "400"; //ALL
    } else if (trainrestrictions == 1) {
        trainrestr = "401"; //All without ICE
    } else if (trainrestrictions == 2) {
        trainrestr = "403"; //Only local transport and authority
    } else if (trainrestrictions == 3) {
        trainrestr = "402"; //Only authority without extra fare
    }

    QUrl url("http://efa.mvv-muenchen.de/mobile/XSLT_TRIP_REQUEST2");

    QString modeString = "dep";
    if (mode == 0)
    {
        modeString = "arr";
    }

    //Postdata for normal Search
    QString postData = "sessionID=0&requestID=&language=en&usage=xslt_trip&command=&ptOptionsActive=0&itdDateDay=" +
                       date.toString("dd") +
                       "&itdDateMonth=" +
                       date.toString("MM") +
                       "&itdDateYear=" +
                       date.toString("yyyy") +
                       "&place_origin=&placeState_origin=empty&type_origin=stop&name_origin=" +
                       destinationStation +
                       "&nameState_origin=empty&place_destination=&placeState_destination=empty&type_destination=stop&name_destination=" +
                       arrivalStation +
                       "&nameState_destination=empty&place_via=&placeState_via=empty&type_via=stop&name_via=&nameState_via=empty&itdTripDateTimeDepArr=" +
                       modeString +
                       "&itdTimeHour=" +
                       time.toString("hh") +
                       "&itdTimeMinute=" +
                       time.toString("mm") +
                       "&lineRestriction=" +
                       trainrestr;

    http->setHost(url.host(), QHttp::ConnectionModeHttp, url.port() == -1 ? 0 : url.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    QHttpRequestHeader header;
    header.setRequest("POST", url.path() + "?" + url.encodedQuery());
    header.setValue("Host", url.host());

    currentRequestId = http->request(header, postData.toAscii(), filebuffer);

    loop.exec();

    filebuffer->close();

    //qDebug() <<filebuffer->buffer();
    return parseJourneyData(filebuffer->buffer());
}

ResultInfo parserMvvDe::getJourneyData(QString queryUrl)
{
    QUrl url(queryUrl);

    http->setHost(url.host(), QHttp::ConnectionModeHttp, url.port() == -1 ? 0 : url.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    currentRequestId = http->get(url.path() + "?" + url.encodedQuery(), filebuffer);

    loop.exec();

    filebuffer->close();

    //qDebug() <<filebuffer->buffer();

    return parseJourneyData(filebuffer->buffer());
}

ResultInfo parserMvvDe::parseJourneyData(QByteArray data)
{
    ResultInfo result;

    //Header
    QRegExp regexp = QRegExp("<td>.*<b>From:.*</b>(.*)<br />.*<b>To:.*</b>(.*)<br />.*<b>Date:.*</b>(.*)</td>");
    regexp.setMinimal(true);

    regexp.indexIn(data);

    result.fromStation = regexp.cap(1);
    result.toStation   = regexp.cap(2);
    result.timeInfo    = regexp.cap(3);
    result.timeInfo.replace("\n", " ").replace("  ", " ");

    regexp = QRegExp("sessionID=(.*)&");
    regexp.setMinimal(true);
    regexp.indexIn(data);

    result.earlierUrl = "http://efa.mvv-muenchen.de/mobile/XSLT_TRIP_REQUEST2?language=en&requestID=1&command=tripPrev&sessionID=" + regexp.cap(1);
    result.laterUrl = "http://efa.mvv-muenchen.de/mobile/XSLT_TRIP_REQUEST2?language=en&requestID=1&command=tripNext&sessionID=" + regexp.cap(1);

    //Items
    regexp = QRegExp("<div style=\"background-color:#.*;\">(.*)</div>");
    regexp.setMinimal(true);
    int idx = 0;
    int num = 0;
    while (idx >= 0)
    {
        idx = regexp.indexIn(data, idx);
        if (idx >= 0)
        {
            num++;

            QBuffer readBuffer;
            QString element = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body>\n" + regexp.cap(1) + "\n</body>\n</html>\n";

            //qDebug() <<element;

            readBuffer.setData(element.toAscii());
            readBuffer.open(QIODevice::ReadOnly);

            QXmlQuery query;
            query.bindVariable("path", &readBuffer);
            query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/a/string()");

            QStringList timesResults;
            if (!query.evaluateTo(&timesResults))
            {
                qDebug() << "parserMvvDe::getJourneyData - Query 1 Failed";
            }

            query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/child::text()/string()");

            QStringList durationResults;
            if (!query.evaluateTo(&durationResults))
            {
                qDebug() << "parserMvvDe::getJourneyData - Query 2 Failed";
            }

            //Query for train infos
            query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/img/@alt/string()");

            QStringList trainResults;
            if (!query.evaluateTo(&trainResults))
            {
                qDebug() << "parserMvvDe::getJourneyData - Query 3 Failed";
            }

            query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/a[1]/@href/string()");

            QStringList detaillinkResults;
            if (!query.evaluateTo(&detaillinkResults))
            {
                qDebug() << "parserMvvDe::getJourneyData - Query 4 Failed";
            }

            if (timesResults.count() > 0)
            {
                ResultItem item;
                QRegExp innerRegexp = QRegExp("(\\d\\d:\\d\\d).*-.*(\\d\\d:\\d\\d)");
                innerRegexp.setMinimal(true);
                innerRegexp.indexIn(timesResults.join(""));

                item.fromTime  = innerRegexp.cap(1);
                item.toTime    = innerRegexp.cap(2);

                trainResults.removeDuplicates();
                QString trains = trainResults.join(", ");
                //Replace strange alt tags with better and shorter variants
                trains.replace("Footpath", tr("Walk"));
                trains.replace("DLR", tr("S-Bahn"));
                trains.replace("Tube", tr("U-Bahn"));
                trains.replace("Tramlink", tr("Tram"));
                trains.replace(", Seat", "");

                item.trainType = trains;

                QString tmp = durationResults.join("");
                tmp = tmp.trimmed();
                tmp.replace("\n", " ");
                tmp.replace("  ", " ");
                tmp.append(" ");
                innerRegexp = QRegExp("Duration:(.+)Changes:(.+) ");
                innerRegexp.setMinimal(true);
                innerRegexp.indexIn(tmp);

                item.duration = innerRegexp.cap(1).trimmed();
                item.changes  = innerRegexp.cap(2).trimmed();
                item.detailsUrl = "http://efa.mvv-muenchen.de/mobile/" + detaillinkResults[0];

                result.items.append(item);
            }

            ++idx;
        }
    }
    return result;
}

DetailResultInfo parserMvvDe::getJourneyDetailsData(QString queryUrl)
{
    QUrl url(queryUrl);

    http->setHost(url.host(), QHttp::ConnectionModeHttp, url.port() == -1 ? 0 : url.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    currentRequestId = http->get(url.path() + "?" + url.encodedQuery(), filebuffer);

    loop.exec();

    filebuffer->close();

    //qDebug() <<filebuffer->buffer();

    DetailResultInfo result;
    QString xhtml = filebuffer->buffer();

    QRegExp regexp = QRegExp("<td>.*<b>From:.*</b>(.*)<br />.*<b>To:.*</b>(.*)<br />.*<b>Date:.*</b>(.*)</td>");
    regexp.setMinimal(true);
    regexp.indexIn(xhtml);

    QString dateString = regexp.cap(3).replace("\n", "").trimmed();

    regexp = QRegExp("(.*)(\\d\\d)\\.(.*)(\\d\\d\\d\\d)(.*)");
    regexp.setMinimal(true);
    regexp.indexIn(dateString);

    QDate theDate = QDate::fromString(regexp.cap(2) + "." + regexp.cap(3).trimmed() + regexp.cap(4), "dd.MMM.yyyy");

    //We use Regex to get each element details page
    regexp = QRegExp("<table width=\"100%\" cellspacing=\"0\" cellpadding=\"2\" border=\"0\">(.*)</table>");
    regexp.setMinimal(true);

    int idx = 0;
    int num = 0;
    while (idx >= 0)
    {
        idx = regexp.indexIn(xhtml, idx);
        if (idx >= 0)
        {
            num++;

            QBuffer readBuffer;
            QString element = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body>\n" + regexp.cap(0) + "\n</body>\n</html>\n";

            readBuffer.setData(element.toAscii());
            readBuffer.open(QIODevice::ReadOnly);

            QXmlQuery query;
            query.bindVariable("path", &readBuffer);
            query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td[@colspan='4']/string()");

            QStringList namesResult;
            if (!query.evaluateTo(&namesResult))
            {
                qDebug() << "parserMobileBahnDe::getJourneyDetailsData - Query 1 Failed";
            }

            query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td[3]/string()");

            QStringList descResult;
            if (!query.evaluateTo(&descResult))
            {
                qDebug() << "parserMobileBahnDe::getJourneyDetailsData - Query 2 Failed";
            }

            for(int i = 0; i < descResult.count(); i++)
            {
                DetailResultItem item;

                item.train = descResult[i].trimmed().replace("\t", " ").replace("\n", " ").replace("  ", " ");

                QRegExp innerRegexp = QRegExp("(arr|dep)\\. (\\d\\d:\\d\\d)?(.*)$");
                innerRegexp.setMinimal(true);

                QString tmp = namesResult[(i * 2)];
                tmp = tmp.trimmed();
                innerRegexp.indexIn(tmp);

                item.fromInfo    = innerRegexp.cap(2).trimmed();
                item.fromStation = innerRegexp.cap(3).trimmed().replace("\t", " ");

                QTime fromTime = QTime::fromString(innerRegexp.cap(2).trimmed(), "HH:mm");

                //if fromTime is empty use the toTime form prev entry
                if ((innerRegexp.cap(2).trimmed() == "") && (result.items.count() > 0))
                {
                    DetailResultItem prev = result.items.last();
                    fromTime              = prev.toTime.time();
                }

                item.fromTime.setDate(theDate);
                item.fromTime.setTime(fromTime);

                //Also check if day change to the previous entry
                if (result.items.count() > 0)
                {
                    DetailResultItem prev = result.items.last();
                    if (item.fromTime.toTime_t() < prev.toTime.toTime_t())
                    {
                        item.fromTime = item.fromTime.addDays(1);
                        theDate       = theDate.addDays(1);
                    }
                }

                tmp = namesResult[(i * 2) + 1];
                tmp = tmp.trimmed();
                innerRegexp.indexIn(tmp);

                item.toInfo    = innerRegexp.cap(2).trimmed();
                item.toStation = innerRegexp.cap(3).trimmed().replace("\t", " ");

                QTime toTime   = QTime::fromString(innerRegexp.cap(2).trimmed(), "HH:mm");

                //if toTime is empty use the toTime form prev entry
                if ((innerRegexp.cap(2).trimmed() == "") && (result.items.count() > 0))
                {
                    DetailResultItem prev = result.items.last();
                    toTime                = prev.toTime.time();
                }

                item.toTime.setDate(theDate);
                item.toTime.setTime(toTime);

                //its possible that we have a day change so we append one day to totime if needed
                if (item.toTime.toTime_t() < item.fromTime.toTime_t())
                {
                    item.toTime = item.toTime.addDays(1);
                }
                result.items.append(item);
            }

            ++idx;
        }
    }
    return result;
}
