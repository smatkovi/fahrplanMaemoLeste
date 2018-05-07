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

#include "parser_sbbch.h"

parserSbbCh::parserSbbCh(QObject *parent)
{
    Q_UNUSED(parent);
    http = new QHttp(this);

    connect(http, SIGNAL(requestFinished(int,bool)), this, SLOT(httpRequestFinished(int,bool)));
    connect(http, SIGNAL(dataReadProgress(int,int)), this, SLOT(httpDataReadProgress(int,int)));
}

bool parserSbbCh::supportsGps()
{
    return false;
}


QStringList parserSbbCh::getStationsByName(QString stationName)
{
    QByteArray postData = "queryPageDisplayed=yes&start=%BB%A0Search+connection&REQ0JourneyStopsS0A=1&REQ0JourneyStopsS0G=";
    postData.append(stationName);

    qDebug() << "SBBch: getStationsByName";

    QUrl url("http://fahrplan.sbb.ch/bin/query.exe/en");

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

    QRegExp regexp = QRegExp("<select name=\"REQ0JourneyStopsS0K\" accesskey=\"f\"  tabindex=\"10\">(.*)</select>");
    regexp.setMinimal(true);

    regexp.indexIn(filebuffer->buffer());

    QString element = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body>\n" + regexp.cap(0) + "\n</body>\n</html>\n";

    QBuffer readBuffer;
    readBuffer.setData(element.toAscii());
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    //Query for more than one result
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/select[@name='REQ0JourneyStopsS0K']/option/string()");

    QStringList result;
    if (!query.evaluateTo(&result))
    {
        qDebug() << "parserSbbCh::getStationsByName - Query 1 Failed";
    }

    if (result.count() == 0)
    {
        regexp = QRegExp("<input type=\"hidden\" name=\"REQ0JourneyStopsS0K\" value=\"(.*)\">(.*)<span class=\"bold\">(.*)</span>");
        regexp.setMinimal(true);

        regexp.indexIn(filebuffer->buffer());

        if (regexp.cap(3).trimmed() != "")
        {
            QString element = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body>\n<span>" + regexp.cap(3) + "</span>\n</body>\n</html>\n";

            readBuffer.close();

            readBuffer.setData(element.toAscii());
            readBuffer.open(QIODevice::ReadOnly);
            query.bindVariable("path", &readBuffer);

            query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/span/string()");

            if (!query.evaluateTo(&result))
            {
                qDebug() << "parserSbbCh::getStationsByName - Query 2 Failed";
            }
        }
    }

    delete filebuffer;

    //qDebug() <<result;

    return result;
}

QStringList parserSbbCh::getStationsByGPS(qreal latitude, qreal longitude)
{
    Q_UNUSED(latitude);
    Q_UNUSED(longitude);
    QStringList result;
    return result;
}

ResultInfo parserSbbCh::getJourneyData(QString destinationStation, QString arrivalStation, QString viaStation, QDate date, QTime time, int mode, int trainrestrictions)
{
    Q_UNUSED(viaStation);
    Q_UNUSED(trainrestrictions);

    QUrl url("http://fahrplan.sbb.ch/bin/query.exe/en");

    //Postdata for normal Search
    QString postData = "REQ0JourneyStopsS0A=1&REQ0JourneyStopsS0G=" +
                          destinationStation +
                          "&REQ0JourneyStopsS0ID=&REQ0JourneyStopsZ0A=1&REQ0JourneyStopsZ0G=" +
                          arrivalStation +
                          "&REQ0JourneyStopsZ0ID=&REQ0JourneyDate=" +
                          date.toString("dd.MM.yyyy") +
                          "&REQ0JourneyTime=" +
                          time.toString("hh:mm") +
                          "&REQ0HafasSearchForw=" +
                          QString::number(mode) +
                          "&existOptimizePrice=1&REQ0HafasOptimize1=0%3A1&existProductNahverkehr=yes&REQ0Tariff_TravellerAge.1=35&start=Suchen&REQ0Tariff_Class=2&REQ0Tariff_TravellerReductionClass.1=0";

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

    return parseJourneyData(filebuffer->buffer());
}

ResultInfo parserSbbCh::getJourneyData(QString queryUrl)
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

DetailResultInfo parserSbbCh::getJourneyDetailsData(QString queryUrl)
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

    QRegExp regexp = QRegExp("</form>(.*)Details - Connection(.*)<table class=\"hac_greybox\">(.*)<table class=\"buttons\"");
    regexp.setMinimal(true);

    regexp.indexIn(filebuffer->buffer());

    QString element = "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body>\n<table>" + regexp.cap(3) + "</td></tr></table>\n</body>\n</html>\n";

    //Clean HTML or fix other not needed but parse error causing stuff
    element.replace(" nowrap ", " nowrap=\"true\" ");
    element.replace(" nowrap>", " nowrap=\"true\">");
    element.replace("style=\"margin-top:2px;\">", "style=\"margin-top:2px;\" />");
    QRegExp ahrefReg = QRegExp("href=\"(.*)\"");
    ahrefReg.setMinimal(true);
    element.replace(ahrefReg, "");
    QRegExp imgReg = QRegExp("alt=\"([^\"]*)\">");
    imgReg.setMinimal(true);
    element.replace(imgReg, "alt=\"\\1\" />");
    element.replace("&nbsp;", " ");

    QRegExp detailIdReg = QRegExp("id=\"linkDtlClose(.*)\"");
    detailIdReg.setMinimal(true);
    detailIdReg.indexIn(element);
    QString detailId = detailIdReg.cap(1);

//    qDebug() <<element;

    QBuffer readBuffer;
    readBuffer.setData(element.toAscii());
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    //Query for more than one result
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td/table/tr/td[@headers='time-" + detailId + "']/string()");
    QStringList timelinkResults;
    if (!query.evaluateTo(&timelinkResults))
    {
        qDebug() << "parserSbbCh::getJourneyDetailsData - Query 1 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td/table/tr/td[@headers='products-" + detailId + "']/string()");

    QStringList trainsResults;
    if (!query.evaluateTo(&trainsResults))
    {
        qDebug() << "parserSbbCh::getJourneyDetailsData - Query 2 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td/table/tr/td[@class!='stop-station-icon noPrint'][@class!='stop-station-icon noPrint last'][@headers='stops-" + detailId + "']/a/string()");

    QStringList fromToResults;
    if (!query.evaluateTo(&fromToResults))
    {
        qDebug() << "parserSbbCh::getJourneyDetailsData - Query 3 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td/table/tr/td[@style='padding-top:4px;']/child::text()/string()");

    QStringList durationResults;
    if (!query.evaluateTo(&durationResults))
    {
        qDebug() << "parserSbbCh::getJourneyDetailsData - Query 4 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td/table/tr/td[@style='padding-top:4px;']/table/string()");

    QStringList infoResults;
    if (!query.evaluateTo(&infoResults))
    {
        qDebug() << "parserSbbCh::getJourneyDetailsData - Query 5 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td/table/tr/td[@headers='platform-" + detailId + "']/string()");

    QStringList platformResults;
    if (!query.evaluateTo(&platformResults))
    {
        qDebug() << "parserSbbCh::getJourneyDetailsData - Query 6 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td/table/tr/td[@headers='date-" + detailId + "']/string()");

    QStringList dateResults;
    if (!query.evaluateTo(&dateResults))
    {
        qDebug() << "parserSbbCh::parseJourneyData - Query 7 Failed";
    }

    /*
    qDebug() <<infoResults;
    qDebug() <<fromToResults;
    qDebug() <<timelinkResults;
    qDebug() <<platformResults;
    qDebug() <<trainsResults;
    qDebug() <<durationResults;
    qDebug() <<dateResults;
    */

    DetailResultInfo result;

    QString lastDate = "";

    for(int i = 0; i < trainsResults.count(); i++)
    {
        DetailResultItem item;

        item.fromStation = fromToResults[i * 2].trimmed();
        item.fromInfo    = timelinkResults[(i * 4) + 1].trimmed();
        if (platformResults[(i*2)].trimmed() != "")
        {
            item.fromInfo.append(" Pl." + platformResults[(i*2)].trimmed());
        }

        item.toStation   = fromToResults[(i * 2) + 1].trimmed();
        item.toInfo      = timelinkResults[(i * 4) + 3].trimmed();
        if (platformResults[(i*2) + 1].trimmed() != "")
        {
            item.toInfo.append(" Pl." + platformResults[(i*2) + 1].trimmed());
        }
        item.train       = trainsResults[i].trimmed();

        if (dateResults[i * 2].trimmed() != "")
        {
            QRegExp tmpRegexp = QRegExp("(.*), (\\d\\d.\\d\\d.\\d\\d)");
            tmpRegexp.setMinimal(true);
            tmpRegexp.indexIn(dateResults[i * 2].trimmed());
            lastDate = tmpRegexp.cap(2);
        }

        if (lastDate != "")
        {
            QDate fromDate = QDate::fromString(lastDate, "dd.MM.yy");
            QTime fromTime = QTime::fromString(timelinkResults[(i * 4) + 1].trimmed(), "hh:mm");

            fromDate = fromDate.addYears(100); //default is 1900 so we add 100years
            item.fromTime.setDate(fromDate);
            item.fromTime.setTime(fromTime);
        }

        if (dateResults[(i * 2) + 1].trimmed() != "")
        {
            QRegExp tmpRegexp = QRegExp("(.*), (\\d\\d.\\d\\d.\\d\\d)");
            tmpRegexp.setMinimal(true);
            tmpRegexp.indexIn(dateResults[(i * 2) + 1].trimmed());
            lastDate = tmpRegexp.cap(2);
        }

        if (lastDate != "")
        {
            QDate toDate = QDate::fromString(lastDate, "dd.MM.yy");
            QTime toTime = QTime::fromString(timelinkResults[(i * 4) + 3].trimmed(), "hh:mm");

            toDate = toDate.addYears(100); //default is 1900 so we add 100years
            item.toTime.setDate(toDate);
            item.toTime.setTime(toTime);
        }

        result.items.append(item);
    }

    QRegExp durationRex = QRegExp("(.*)(\\d+:\\d\\d)(.*)");
    durationRex.setMinimal(true);
    durationRex.indexIn(durationResults[0].replace("\n", "").trimmed());

    result.duration = durationRex.cap(2);
    result.info     = infoResults.join("<br>").trimmed();

    return result;
}

ResultInfo parserSbbCh::parseJourneyData(QByteArray data)
{
    QRegExp regexp = QRegExp("<form name=\"formular\" action=\"(.*)\" method=\"post\" style=\"display:inline;\">(.*)<table cellspacing=\"0\" class=\"hafas-content hafas-tp-result-overview\">(.*)</table>(.*)<table class=\"hafas-content\"");
    regexp.setMinimal(true);

    regexp.indexIn(data);

    QString element = "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body>\n<table>" + regexp.cap(3) + "</table>\n</body>\n</html>\n";

    QString baseUrl = regexp.cap(1).trimmed().replace("#focus","");

    //Clean HTML or fix other not needed but parse error causing stuff
    element.replace("checked value=\"yes\"", "checked=\"true\" value=\"yes\" /");
    element.replace(" value=\"yes\">", " value=\"yes\" />");
    element.replace(" nowrap ", " nowrap=\"true\" ");
    element.replace(" nowrap>", " nowrap=\"true\">");
    QRegExp advRegex = QRegExp("href=\"(.*)\"");
    advRegex.setMinimal(true);
    element.replace(advRegex, "");
    advRegex = QRegExp("<script(.*)</script>");
    advRegex.setMinimal(true);
    element.replace(advRegex, "");
    advRegex = QRegExp("<table class=\"hafas-tp-result-overview\">(.*)</table>");
    advRegex.setMinimal(true);
    element.replace(advRegex, "");
    element.replace("&nbsp;", " ");

    //qDebug() <<element;

    QBuffer readBuffer;
    readBuffer.setData(element.toAscii());
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    //Query for more than one result
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td/span/@id/string()");

    QStringList detailIdResult;
    if (!query.evaluateTo(&detailIdResult))
    {
        qDebug() << "parserSbbCh::parseJourneyData - Query 1 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td[@class='result timeRight']/string()");

    QStringList timelinkResults;
    if (!query.evaluateTo(&timelinkResults))
    {
        qDebug() << "parserSbbCh::parseJourneyData - Query 2 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td[@headers='duration']/string()");

    QStringList durationsResults;
    if (!query.evaluateTo(&durationsResults))
    {
        qDebug() << "parserSbbCh::parseJourneyData - Query 3 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td[@headers='changes']/string()");

    QStringList changesResults;
    if (!query.evaluateTo(&changesResults))
    {
        qDebug() << "parserSbbCh::parseJourneyData - Query 4 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td[@headers='products']/string()");

    QStringList trainsResults;
    if (!query.evaluateTo(&trainsResults))
    {
        qDebug() << "parserSbbCh::parseJourneyData - Query 5 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td[@headers='location'][@class!='result maplink']/string()");

    QStringList fromToResults;
    if (!query.evaluateTo(&fromToResults))
    {
        qDebug() << "parserSbbCh::parseJourneyData - Query 6 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tr/td[@headers='date']/string()");

    QStringList dateResults;
    if (!query.evaluateTo(&dateResults))
    {
        qDebug() << "parserSbbCh::parseJourneyData - Query 7 Failed";
    }

/*
    qDebug() <<detailIdResult;
    qDebug() <<fromToResults;
    qDebug() <<timelinkResults;
    qDebug() <<durationsResults;
    qDebug() <<changesResults;
    qDebug() <<trainsResults;
    qDebug() <<dateResults;
*/

    ResultInfo result;

    if (fromToResults.count() < 2)
    {
        return result;
    }

    result.fromStation = fromToResults[0];
    result.toStation   = fromToResults[1];
    result.timeInfo    = dateResults[0];
    result.earlierUrl  = baseUrl + "&REQ0HafasScrollDir=2";
    result.laterUrl    = baseUrl + "&REQ0HafasScrollDir=1";

    int i;
    for(i=0; i < detailIdResult.count(); i++)
    {
        QString detailId = detailIdResult[i];

        QRegExp detailIdReg = QRegExp("connectionNumber(.*)");
        detailIdReg.setMinimal(true);
        detailId.replace(detailIdReg, "");

        ResultItem item;
        item.changes    = changesResults[i];
        item.duration   = durationsResults[i];
        //item.state      = stateResults[i];
        item.trainType  = trainsResults[i];
        item.fromTime   = timelinkResults[(i * 2)];
        item.toTime     = timelinkResults[(i * 2) + 1];
        item.detailsUrl = baseUrl + "&HWAI=CONNECTION$" + detailId + "!id=" + detailId + "!HwaiConId=" + detailId + "!HwaiDetailStatus=details";
        result.items.append(item);
    }
    return result;
}
