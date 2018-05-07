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

#include "parser_131500comau.h"

parser131500ComAu::parser131500ComAu(QObject *parent)
{
     Q_UNUSED(parent);
     http = new QHttp(this);

     connect(http, SIGNAL(requestFinished(int,bool)),
             this, SLOT(httpRequestFinished(int,bool)));
}

bool parser131500ComAu::supportsGps()
{
    return false;
}

QStringList parser131500ComAu::getStationsByName(QString stationName)
{
    QString fullUrl = "http://www.131500.com.au/plan-your-trip/trip-planner?session=invalidate&itd_cmd=invalid&itd_includedMeans=checkbox&itd_inclMOT_5=1&itd_inclMOT_7=1&itd_inclMOT_1=1&itd_inclMOT_9=1&itd_anyObjFilter_origin=2&itd_anyObjFilter_destination=0&itd_name_destination=Enter+location&x=37&y=12&itd_itdTripDateTimeDepArr=dep&itd_itdTimeHour=-&itd_itdTimeMinute=-&itd_itdTimeAMPM=pm";
    fullUrl.append("&itd_itdDate=" + QDate::currentDate().toString("yyyyMMdd"));
    fullUrl.append("&itd_name_origin=" + stationName);

    qDebug() << "parser131500ComAu::getStationsByName";

    QUrl url(fullUrl);

    http->setHost(url.host(), QHttp::ConnectionModeHttp, url.port() == -1 ? 0 : url.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    currentRequestId = http->get(url.path() + "?" + url.encodedQuery(), filebuffer);

    loop.exec();

    filebuffer->close();

    QRegExp regexp = QRegExp("<select name=\"(.*)\" id=\"from\" size=\"6\" class=\"multiple\">(.*)</select>");
    regexp.setMinimal(true);

    regexp.indexIn(filebuffer->buffer());

    QString element = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body>\n" + regexp.cap(0) + "\n</body>\n</html>\n";

    QBuffer readBuffer;
    readBuffer.setData(element.toAscii());
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    //Query for more than one result
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/select/option/string()");

    QStringList result;
    if (!query.evaluateTo(&result))
    {
        qDebug() << "parser131500ComAu::getStationsByName - Query 1 Failed";
    }

    //Remove unneeded stuff from the result
    for (int i = 0; i < result.count(); i++) {
        result[i].replace(" (Location)", "");
    }

    delete filebuffer;

    return result;
}

QStringList parser131500ComAu::getTrainRestrictions()
{
    QStringList result;
    result.append(tr("All, except School Buses"));
    result.append(tr("Regular Buses"));
    result.append(tr("Trains"));
    result.append(tr("Ferries"));
    result.append(tr("STA School Bus"));
    return result;
}

QStringList parser131500ComAu::getStationsByGPS(qreal latitude, qreal longitude)
{
    Q_UNUSED(latitude);
    Q_UNUSED(longitude);
    QStringList result;
    return result;
}

ResultInfo parser131500ComAu::getJourneyData(QString destinationStation, QString arrivalStation, QString viaStation, QDate date, QTime time, int mode, int trainrestrictions)
{
    Q_UNUSED(viaStation);
    QString modeString = "dep";
    if (mode == 0) {
        modeString = "arr";
    }

    QString hourStr = "am";
    int hour = time.toString("hh").toInt();
    if (hour > 12) {
        hour = hour - 12;
        hourStr = "pm";
    }

    //Request one. (Station selection and receiving an up to date cookie.)
    QString fullUrl = "http://www.131500.com.au/plan-your-trip/trip-planner?session=invalidate&itd_cmd=invalid&itd_includedMeans=checkbox&itd_inclMOT_7=1&itd_anyObjFilter_origin=2&itd_anyObjFilter_destination=2&x=37&y=12";
    fullUrl.append("&itd_itdDate=" + date.toString("yyyyMMdd"));
    fullUrl.append("&itd_itdTimeHour=" + QString::number(hour));
    fullUrl.append("&itd_itdTimeMinute=" + time.toString("mm"));
    fullUrl.append("&itd_itdTripDateTimeDepArr=" + modeString);
    fullUrl.append("&itd_itdTimeAMPM=" + hourStr);
    fullUrl.append("&itd_name_origin=" + destinationStation);
    fullUrl.append("&itd_name_destination=" + arrivalStation);

    // itd_inclMOT_5 = bus
    // itd_inclMOT_1 = train
    // itd_inclMOT_9 = ferry
    // itd_inclMOT_11 = school bus
    if (trainrestrictions == 0) {
       fullUrl.append("&itd_inclMOT_5=1&itd_inclMOT_1=1&itd_inclMOT_9=1");
    }
    if (trainrestrictions == 1) {
       fullUrl.append("&itd_inclMOT_5=1");
    }
    if (trainrestrictions == 2) {
       fullUrl.append("&itd_inclMOT_1=1");
    }
    if (trainrestrictions == 3) {
       fullUrl.append("&itd_inclMOT_9=1");
    }
    if (trainrestrictions == 4) {
       fullUrl.append("&itd_inclMOT_11=1");
    }

    //qDebug()<<fullUrl;

    QUrl url(fullUrl);

    http->setHost(url.host(), QHttp::ConnectionModeHttp, url.port() == -1 ? 0 : url.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    currentRequestId = http->get(url.path() + "?" + url.encodedQuery(), filebuffer);

    loop.exec();

    QRegExp regexp = QRegExp("<div class=\"midcolumn3\">(.*)</div>(.*)</div>(.*)<div id=\"righttools\">");
    regexp.setMinimal(true);

    regexp.indexIn(filebuffer->buffer());

    QString element = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body>\n" + regexp.cap(0) + "\n</div></body>\n</html>\n";

    QRegExp imgReg = QRegExp("icon_(.*)_s.gif\" />");
    imgReg.setMinimal(true);
    element.replace(imgReg, "icon_" + QString("\\1") + "_s.gif\" />" + QString("\\1"));

    //qDebug()<<element;

    QBuffer readBuffer;
    readBuffer.setData(element.toAscii());
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/table/tbody/tr/td[@id='header2']/string()");

    QStringList departResult;
    if (!query.evaluateTo(&departResult))
    {
        qDebug() << "parser131500ComAu::getJourneyData - Query 1 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/table/tbody/tr/td[@id='header3']/string()");

    QStringList arriveResult;
    if (!query.evaluateTo(&arriveResult))
    {
        qDebug() << "parser131500ComAu::getJourneyData - Query 2 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/table/tbody/tr/td[@id='header4']/string()");

    QStringList timeResult;
    if (!query.evaluateTo(&timeResult))
    {
        qDebug() << "parser131500ComAu::getJourneyData - Query 3 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/table/tbody/tr/td[@id='header5']/string()");

    QStringList trainResult;
    if (!query.evaluateTo(&trainResult))
    {
        qDebug() << "parser131500ComAu::getJourneyData - Query 4 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/string()");

    QStringList headerResult;
    if (!query.evaluateTo(&headerResult))
    {
        qDebug() << "parser131500ComAu::getJourneyData - Query 5 Failed";
    }

    ResultInfo result;

    for (int i = 0; i < headerResult.count(); i++) {
        QRegExp regexp = QRegExp("(From:|To:|When:)(.*)$");
        regexp.setMinimal(true);
        regexp.indexIn(headerResult[i].trimmed());
        if (regexp.cap(1) == "From:") {
            result.fromStation = regexp.cap(2).trimmed();
        }
        if (regexp.cap(1) == "To:") {
            result.toStation = regexp.cap(2).trimmed();
        }
        if (regexp.cap(1) == "When:") {
            result.timeInfo = regexp.cap(2).trimmed();
        }
    }

    //Generate Details search results

    QStringList detailsResult;

    regexp = QRegExp("<table class=\"dataTbl widthcol2and3\" cellspacing=\"0\" style=\"margin:0px ! important;border-right:0px none;\" summary=\"Search Results Details\">(.*)</table>");
    regexp.setMinimal(true);
    int pos = 0;

    while ((pos = regexp.indexIn(filebuffer->buffer(), pos)) != -1) {
        QString element = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body><table>\n" + regexp.cap(1) + "\n</table></body>\n</html>\n";
        element.replace("&nbsp;", " ");
        element.replace("bulletin.gif\">", "bulletin.gif\" />");

        QBuffer readBuffer;
        readBuffer.setData(element.toAscii());
        readBuffer.open(QIODevice::ReadOnly);

        QXmlQuery query;
        query.bindVariable("path", &readBuffer);

        query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/table/tbody/tr/td[@headers='header2']/string()");

        QStringList detailsContentResult;
        if (!query.evaluateTo(&detailsContentResult))
        {
            qDebug() << "parser131500ComAu::getJourneyData - DetailsQuery 1 Failed";
        }

        detailsResult << detailsContentResult.join("<linesep>");

        pos += regexp.matchedLength();
    }

    //Create search result
    for (int i = 0; i < departResult.count(); i++) {

        //Parse transporttypes and changes
        QString tmp = trainResult[i].trimmed();
        tmp.replace("\t", " ");
        tmp.replace("\n", " ");
        tmp.replace("\r", " ");
        QStringList trains = tmp.split(" ", QString::SkipEmptyParts);
        int changes = trains.length() - 1;
        trains.removeDuplicates();

        //Parse travel time
        tmp = timeResult[i].trimmed();
        tmp.replace("mins", "");
        tmp.replace("min", "");
        tmp.replace("hrs ", ":");
        tmp.replace("hr ", ":");
        QStringList durationLst = tmp.split(":", QString::SkipEmptyParts);
        QString durationStr = "";
        if (durationLst.length() == 1) {
            durationStr = "00:" + fahrplanUtils::leadingZeros(durationLst[0].toInt(), 2);
        }
        if (durationLst.length() == 2) {
            durationStr = fahrplanUtils::leadingZeros(durationLst[0].toInt(), 2) + ":" + fahrplanUtils::leadingZeros(durationLst[1].toInt(), 2);
        }

        QString detailsData = detailsResult[i];
        detailsData.prepend("Header: <duration>" + durationStr + "</duration><date>" + result.timeInfo + "</date><linesep>");

        ResultItem item;
        item.changes    = QString::number(changes);
        item.duration   = durationStr;
        item.trainType  = trains.join(", ");
        item.fromTime   = QTime::fromString(departResult[i].trimmed(), "h:map").toString("hh:mm");
        item.toTime     = QTime::fromString(arriveResult[i].trimmed(), "h:map").toString("hh:mm");
        item.detailsUrl = detailsData;
        result.items.append(item);
    }

    filebuffer->close();
    delete filebuffer;
    return result;
}

ResultInfo parser131500ComAu::getJourneyData(QString queryUrl)
{
    Q_UNUSED(queryUrl);
    ResultInfo result;
    return result;
}

//We using a fake url, in fact we don't use an url at all, we use the details
//data directly because the data is already present after visiting the search results.
DetailResultInfo parser131500ComAu::getJourneyDetailsData(QString queryUrl)
{
    QStringList detailResults = queryUrl.split("<linesep>");

    DetailResultInfo result;

    QDate journeydate;

    for (int i = 0; i < detailResults.count(); i++) {
        DetailResultItem item;
        QRegExp regexp = QRegExp("(Take the |Walk to |Header: )(.*)$");
        regexp.setMinimal(true);
        regexp.indexIn(detailResults[i].trimmed());

        if (regexp.cap(1) == "Header: ") {
            //qDebug()<<"HEADER: "<<regexp.cap(2).trimmed();
            QRegExp regexp2 = QRegExp("<duration>(.*)</duration><date>(.*), (\\d\\d) (.*) (\\d\\d\\d\\d)</date>");
            regexp2.setMinimal(true);
            regexp2.indexIn(regexp.cap(2).trimmed());
            result.duration = regexp2.cap(1).trimmed();
            QLocale enLocale = QLocale(QLocale::English, QLocale::UnitedStates);
            int month = 1;
            for (month = 1; month < 10; month++) {
                if (regexp2.cap(4).trimmed() == enLocale.standaloneMonthName(month)) {
                    break;
                }
            }
            journeydate = QDate::fromString(regexp2.cap(3).trimmed() + " " + QString::number(month) + " " + regexp2.cap(5).trimmed(), "dd M yyyy");
        }
        if (regexp.cap(1) == "Take the ") {
            //qDebug()<<"Regular: "<<regexp.cap(2).trimmed();
            QRegExp regexp2 = QRegExp("(.*)Dep: (\\d:\\d\\d|\\d\\d:\\d\\d)(am|pm) (.*)Arr: (\\d:\\d\\d|\\d\\d:\\d\\d)(am|pm) (.*)(\\t+.*)$");
            regexp2.setMinimal(true);
            regexp2.indexIn(regexp.cap(2).trimmed());
            //qDebug()<<"***";
            if (regexp2.matchedLength() == -1) {
                regexp2 = QRegExp("(.*)Dep: (\\d:\\d\\d|\\d\\d:\\d\\d)(am|pm) (.*)Arr: (\\d:\\d\\d|\\d\\d:\\d\\d)(am|pm) (.*)$");
                regexp2.setMinimal(true);
                regexp2.indexIn(regexp.cap(2).trimmed());
            }
            /*
            qDebug()<<"Train:"<<regexp2.cap(1).trimmed();
            qDebug()<<"Time1:"<<regexp2.cap(2).trimmed();
            qDebug()<<"Time1b:"<<regexp2.cap(3).trimmed();
            qDebug()<<"Station1:"<<regexp2.cap(4).trimmed();
            qDebug()<<"Time2:"<<regexp2.cap(5).trimmed();
            qDebug()<<"Time2b:"<<regexp2.cap(6).trimmed();
            qDebug()<<"Station2:"<<regexp2.cap(7).trimmed();
            qDebug()<<"Alt:"<<regexp2.cap(8).trimmed();
            */
            item.fromStation = regexp2.cap(4).trimmed();
            item.toStation   = regexp2.cap(7).trimmed();
            item.info        = regexp2.cap(8).trimmed();
            item.train       = regexp2.cap(1).trimmed();
            QTime fromTime   = QTime::fromString(regexp2.cap(2).trimmed() + regexp2.cap(3).trimmed(), "h:map");
            QTime toTime     = QTime::fromString(regexp2.cap(5).trimmed() + regexp2.cap(6).trimmed(), "h:map");

            item.fromTime.setDate(journeydate);
            item.fromTime.setTime(fromTime);
            item.toTime.setDate(journeydate);
            item.toTime.setTime(toTime);

            if (item.toTime.toTime_t() < item.fromTime.toTime_t()) {
                item.toTime.addDays(1);
                journeydate.addDays(1);
            }

            item.fromInfo = item.fromTime.time().toString("hh:mm");
            item.toInfo = item.toTime.time().toString("hh:mm");

            result.items.append(item);
        }
        if (regexp.cap(1) == "Walk to ") {
            //qDebug()<<"Walking: "<<regexp.cap(2).trimmed();
            QRegExp regexp2 = QRegExp("(.*) - (.+) (.*)$");
            regexp2.setMinimal(true);
            regexp2.indexIn(regexp.cap(2).trimmed());
            /*
            qDebug()<<"***";
            qDebug()<<"Station1:"<<regexp2.cap(1).trimmed();
            qDebug()<<"WalkDist1:"<<regexp2.cap(2).trimmed();
            qDebug()<<"WalkDist2:"<<regexp2.cap(3).trimmed();
            */
            item.fromStation = "";
            if (result.items.count() > 0) {
                item.fromStation = result.items.last().toStation;
                item.fromInfo    = result.items.last().toInfo;
                item.fromTime    = result.items.last().toTime;
                item.toTime      = result.items.last().toTime;
            }
            item.toStation   = regexp2.cap(1).trimmed();
            item.info        = "Walking " + regexp2.cap(2).trimmed() + " " + regexp2.cap(3).trimmed();

            //Don't add WalkTo infos as first item
            if (result.items.count() > 0) {
                result.items.append(item);
            }
        }
    }
    return result;
}
