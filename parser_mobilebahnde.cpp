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

#include "parser_mobilebahnde.h"

parserMobileBahnDe::parserMobileBahnDe(QObject *parent)
{
     Q_UNUSED(parent);
     http = new QHttp(this);

     connect(http, SIGNAL(requestFinished(int,bool)), this, SLOT(httpRequestFinished(int,bool)));
     connect(http, SIGNAL(dataReadProgress(int,int)), this, SLOT(httpDataReadProgress(int,int)));
}

bool parserMobileBahnDe::supportsGps()
{
    return true;
}

ResultInfo parserMobileBahnDe::parseJourneyData(QByteArray data)
{
    QBuffer readBuffer;

    QString element = data;
    qDebug() << "parserMobileBahnDe::parseJourneyData(): pre html" << endl << element;

    //Remove misformed html
    QRegExp ahrefReg = QRegExp("class=\"arrowlink\" href=\"(.*)\"");
    ahrefReg.setMinimal(true);
    element.replace(ahrefReg, "");

    element.replace("<img src=\"/v/760/img/achtung_rot_16x16.gif\" />", "<img src=\"/v/760/img/achtung_rot_16x16.gif\" />" + tr("Warning"));
    #if 1
        ahrefReg = QRegExp("<script.*</script>");
        ahrefReg.setMinimal(true);
        element.replace(ahrefReg, "");

        /*ahrefReg = QRegExp("<ul class=\"neben \">");
        ahrefReg.setMinimal(true);
        element.replace(ahrefReg, "<ul><ul class=\"neben \">");*/
    #endif

        qDebug() << "parserMobileBahnDe::parseJourneyData(): post html" << endl << element;
    readBuffer.setData(element.toAscii());
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    //Query for Header infos
    //query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/div[@class='haupt']/span[@class='bold']/string()");
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/div/div[@class='stdpadding editBtnCon paddingleft ']/span/string()");

    QStringList headerResults;
    if (!query.evaluateTo(&headerResults))
    {
        qDebug() << "parserMobileBahnDe::parseJourneyData - Query 1 Failed";
    }
    qDebug() << "headerResults" << endl << headerResults;

    //Query for Time infos
    //query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/table/tr/td[@class='overview timelink']/descendant::text()/string()");
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/table/tr/td[@class='overview timelink']/descendant::text()/string()");

    QStringList timelinkResults;
    if (!query.evaluateTo(&timelinkResults))
    {
        qDebug() << "parserMobileBahnDe::parseJourneyData - Query 2 Failed";
    }
    qDebug() << "timelinkResults" << endl << timelinkResults;

    //Query for state infos
    //query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/table/tr/td[@class='overview tprt']/string()");
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/table/tr/td[@class='overview tprt']/string()");

    QStringList stateResults;
    if (!query.evaluateTo(&stateResults))
    {
        qDebug() << "parserMobileBahnDe::parseJourneyData - Query 3 Failed";
    }
    qDebug() << "stateResults" << endl << stateResults;

    //Query for changes infos
    //query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/table/tr/td[@class='overview']/descendant::text()/string()");
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/table/tr/td[@class='overview']/descendant::text()/string()");

    QStringList changesResults;
    if (!query.evaluateTo(&changesResults))
    {
        qDebug() << "parserMobileBahnDe::parseJourneyData - Query 4 Failed";
    }
    qDebug() << "changeResults" << endl << changesResults;

    //Query for train infos
    //query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/table/tr/td[@class='overview iphonepfeil']/descendant::text()[1]/string()");
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/table/tr/td[@class='overview iphonepfeil']/descendant::text()[1]/string()");

    QStringList trainResults;
    if (!query.evaluateTo(&trainResults))
    {
        qDebug() << "parserMobileBahnDe::parseJourneyData - Query 5 Failed";
    }
    qDebug() << "trainResults" << endl << trainResults;

    //Query for error infos
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/form/div[@class='fline errormsg']/string()");

    QStringList errorsResults;
    if (!query.evaluateTo(&errorsResults))
    {
        qDebug() << "parserMobileBahnDe::parseJourneyData - Query 6 Failed";
    }
    qDebug() << "errorsResults" << endl << errorsResults;

    //Query for later url
    //query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/ul[@class='neben']/li/a[@class='noBG']/@href/string()");
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/ul/ul[@class='neben']/li/a[@class='noBG']/@href/string()");

    QStringList laterUrl;
    if (!query.evaluateTo(&laterUrl))
    {
        qDebug() << "parserMobileBahnDe::parseJourneyData - Query 7 Failed";
    }
    qDebug() << "laterUrl" << endl << laterUrl;

    //Query for Details Link infos
    //query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/table/tr/td[@class='overview timelink']/a/@href/string()");
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/table/tr/td[@class='overview timelink']/a/@href/string()");

    QStringList detailslinkResults;
    if (!query.evaluateTo(&detailslinkResults))
    {
        qDebug() << "parserMobileBahnDe::parseJourneyData - Query 8 Failed";
    }
    qDebug() << "detailslinkResults" << endl << detailslinkResults;

    ResultInfo result;

    if (errorsResults.count() > 0)
    {
        result.errorMsg = errorsResults[0];
        return result;
    }

    if (headerResults.count() < 3)
    {
        return result;
    }

    if (laterUrl.count() == 1)
    {
        result.laterUrl   = laterUrl[0];
        result.earlierUrl = laterUrl[0];
        result.earlierUrl.replace("e=1", "e=2");
    }

    result.fromStation = headerResults[0];
    result.toStation   = headerResults[1];
    result.timeInfo    = headerResults[2];

    int i;
    for(i=0; i < stateResults.count(); i++)
    {
        ResultItem item;
        item.changes    = changesResults[i * 2];
        item.duration   = changesResults[(i * 2) + 1];
        item.state      = stateResults[i];
        item.trainType  = trainResults[i];
        item.fromTime   = timelinkResults[i * 2];
        item.toTime     = timelinkResults[(i * 2) + 1];
        item.detailsUrl = detailslinkResults[i];
        result.items.append(item);
    }

    delete filebuffer;

    return result;
}

QStringList parserMobileBahnDe::getTrainRestrictions()
{
    QStringList result;
    result.append(tr("All"));
    result.append(tr("All without ICE"));
    result.append(tr("Only local transport"));
    result.append(tr("Local transport without S-Bahn"));
    return result;
}

DetailResultInfo parserMobileBahnDe::getJourneyDetailsData(QString queryUrl)
{
    QUrl url(queryUrl);

    DetailResultInfo result;

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

    QString xhtml = filebuffer->buffer();

    //check for closed arrow (Verbundtarif???)
    int linkpos=xhtml.indexOf("id=\"dtlOpen_link\"");
    int linkpos_start, linkpos_stop;
    QString link;
    linkpos_stop = xhtml.indexOf(">",linkpos);
    link=xhtml.mid(linkpos,linkpos_stop-linkpos);
    qDebug() << link;

    if (link.contains("class=\"flaparrow\"")) // if arrow closed reload with open arrow
    {
        linkpos_start= link.indexOf("href=")+6;
        linkpos_stop= link.indexOf("\"",linkpos_start);
        link=link.mid(linkpos_start, linkpos_stop-linkpos_start);
        link=link.remove(QRegExp("amp;"));
        qDebug() << link;
        url.setUrl(link);
        http->setHost(url.host(), QHttp::ConnectionModeHttp, url.port() == -1 ? 0 : url.port());

        //filebuffer = new QBuffer();

        if (!filebuffer->open(QIODevice::WriteOnly))
        {
            qDebug() << "Can't open Buffer";
        }

        currentRequestId = http->get(url.path() + "?" + url.encodedQuery(), filebuffer);

        loop.exec();

        filebuffer->close();

        //qDebug() <<filebuffer->buffer();

        xhtml = filebuffer->buffer();
    }

    QDateTime routeStart;
    QDateTime routeEnd;

    {
        //QRegExp regexp = QRegExp("<div class=\"querysummary2 (.*)\".*>.*([0-3][0-9]\\.[0-1][0-9]\\.[0-9][0-9]).*([0-9][0-9]:[0-9][0-9]).*-.*([0-3][0-9]\\.[0-1][0-9]\\.[0-9][0-9]).*([0-9][0-9]:[0-9][0-9]).*</div>");
        QRegExp regexp = QRegExp("<span class=\"querysummary2\" id=\"(.*)\".*>.*([0-3][0-9]\\.[0-1][0-9]\\.[0-9][0-9]).*([0-9][0-9]:[0-9][0-9]).*-.*([0-3][0-9]\\.[0-1][0-9]\\.[0-9][0-9]).*([0-9][0-9]:[0-9][0-9]).*</span>");
        regexp.setMinimal(true);
        regexp.indexIn(xhtml);
        qDebug() << "******" << regexp.cap(0);

        if (regexp.cap(0)!="")
        {
            qDebug() << "******" << regexp.cap(0);
            QDate date=QDate::fromString(regexp.cap(2).trimmed(),"dd.MM.yy");
            date = date.addYears(100);
            qDebug() << "date: " << regexp.cap(2) << date;
            QTime time=QTime::fromString(regexp.cap(3).trimmed(),"hh:mm");
            routeStart=QDateTime(date,time);
            qDebug() << "routestart: " << regexp.cap(3) << routeStart;
            date=QDate::fromString(regexp.cap(4).trimmed(),"dd.MM.yy");
            date = date.addYears(100);
            qDebug() << "date: " << regexp.cap(4) << date;
            time=QTime::fromString(regexp.cap(5).trimmed(),"hh:mm");
            routeEnd=QDateTime(date,time);
            qDebug() << "routeend: " << regexp.cap(5) << routeEnd;
        }
        else {
            //regexp.setPattern("<div class=\"querysummary2 (.*)\".*>.*([0-3][0-9]\\.[0-1][0-9]\\.[0-9][0-9]).*([0-9][0-9]:[0-9][0-9]).*\\-.*([0-9][0-9]:[0-9][0-9]).*</div>");
            regexp.setPattern("<span class=\"querysummary2\" id=\"(.*)\".*>.*([0-3][0-9]\\.[0-1][0-9]\\.[0-9][0-9]).*([0-9][0-9]:[0-9][0-9]).*\\-.*([0-9][0-9]:[0-9][0-9]).*</span>");
            regexp.indexIn(xhtml);
            if (regexp.cap(0)!="")
            {
                qDebug() << "******" << regexp.cap(0);
                QDate date=QDate::fromString(regexp.cap(2).trimmed(),"dd.MM.yy");
                date = date.addYears(100);
                QTime time=QTime::fromString(regexp.cap(3).trimmed(),"hh:mm");
                routeStart=QDateTime(date,time);
                time=QTime::fromString(regexp.cap(4).trimmed(),"hh:mm");
                routeEnd=QDateTime(date,time);
            }
            else qDebug() << "time and date not found";
        }
    }

    //We use Regex to get each element details page
    QRegExp regexp = QRegExp("<div class=\"rline haupt (.*)\".*>(.*)</div>");

    regexp.setMinimal(true);

    int idx = 0;
    int num = 0;
    DetailResultItem fromStationItem;
    DetailResultItem toStationItem;
    QTime lastTime=routeStart.time();
    QDate currentDate=routeStart.date();

    //qDebug() << "data_file: " << xhtml;

    while (idx >= 0)
    {
        qDebug() << "idx:" << idx;
        idx = regexp.indexIn(xhtml, idx);
        qDebug() << "DB Mobile:" << idx;
        if (idx >= 0)
        {
            QString divType=regexp.cap(1);
            enum { ROUTE_NONE, ROUTE_START, ROUTE_CHANGE, ROUTE_END, ROUTE_MOT } divMode = ROUTE_NONE;
            if (divType.contains("routeStart")) divMode=ROUTE_START;
            if (divType.contains("routeChange")) divMode=ROUTE_CHANGE;
            if (divType.contains("routeEnd")) divMode=ROUTE_END;
            if (divType.contains("mot")) divMode=ROUTE_MOT;

            num++;
            DetailResultItem stationInfo;
            QBuffer readBuffer;
            QString element = "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body>\n" + regexp.cap(0) + "\n</body>\n</html>\n";

            //Remove misformed html
            QRegExp ahrefReg = QRegExp("class=\"arrowlink\" href=\"(.*)\"");
            ahrefReg.setMinimal(true);
            element.replace(ahrefReg, "");
            element.replace("&nbsp;", " ");

            //remove divs because of missing closing div
            /*ahrefReg = QRegExp("</a>");
            ahrefReg.setMinimal(true);
            element.replace(ahrefReg, "</a></div>");*/
            ahrefReg = QRegExp("<div class=\"interSection\">");
            ahrefReg.setMinimal(true);
            element.replace(ahrefReg, "");
            ahrefReg = QRegExp("<div class=\"motSection\">");
            ahrefReg.setMinimal(true);
            element.replace(ahrefReg, "");
            ahrefReg = QRegExp("compulsory reservation");
            ahrefReg.setMinimal(true);
            element.replace(ahrefReg, "</div>");

            qDebug() << "element: " << element;

            readBuffer.setData(element.toAscii());
            readBuffer.open(QIODevice::ReadOnly);

            QXmlQuery query;
            query.bindVariable("path", &readBuffer);
            query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/span[@class='bold']/string()");

            QStringList stationResults;
            if (!query.evaluateTo(&stationResults))
            {
                qDebug() << "parserMobileBahnDe::getJourneyDetailsData - Query 1 Failed";
            }
            qDebug() << "stationResults" << endl << stationResults;

            query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/a/span/string()");

            QStringList trainnrResults;
            if (!query.evaluateTo(&trainnrResults))
            {
                qDebug() << "parserMobileBahnDe::getJourneyDetailsData - Query 2 Failed";
            }
            qDebug() << "trainnrResults" << endl << trainnrResults;

            if (divMode==ROUTE_MOT)
            {
                query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/a/child::text()/string()");
            }
            else
            {
                query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/child::text()/string()");
            }
            QStringList txtResults;
            if (!query.evaluateTo(&txtResults))
            {
                qDebug() << "parserMobileBahnDe::getJourneyDetailsData - Query 3 Failed";
            }



            qDebug() <<num;
            qDebug() <<element;
            qDebug() <<stationResults;
            qDebug() <<trainnrResults;
            qDebug() <<txtResults;




            if (stationResults.count() > 0)
            {
                qDebug() << "Station:" << divMode << "(" << divType << ") " << stationResults[0].trimmed();
                if (divMode==ROUTE_START)
                {
                    fromStationItem.fromStation = stationResults[0].trimmed();
                }
                if (divMode==ROUTE_END || divMode == ROUTE_CHANGE)
                {
                    fromStationItem.toStation   = stationResults[0].trimmed();
                    toStationItem.fromStation = fromStationItem.toStation;
                }
            }
            if (divMode==ROUTE_MOT)
            {
                if (trainnrResults.count() > 0)
                {
                    fromStationItem.train = trainnrResults[0].trimmed();
                }
                else {
                    fromStationItem.train = txtResults.join(" ").trimmed();
                    qDebug() << "no train found" << fromStationItem.train;
                }

            }
            if (stationResults.count() > 0)
            {
                QString tmp = txtResults.join(" ");
                tmp.replace("  ", " ");
                tmp.replace("\n", " ");

                //Check and Parse departure and arrival Times
                bool depFound=false;
                bool arrFound=false;
                bool minFound=false;
                QRegExp tmpRegexp = QRegExp("(arr|dep) ([0-9:]+) +[PG]l\\. ([0-9]+ ?[A-H]?\\-?[A-H]?)");
                tmpRegexp.setMinimal(false);
                int depArrIndex=0;
                while (depArrIndex>=0) {
                    depArrIndex=tmpRegexp.indexIn(tmp,depArrIndex);
                    if (depArrIndex>=0)
                    {
                        QString info = tmpRegexp.cap(2).trimmed() + " on " + tr("Platform ") + tmpRegexp.cap(3).trimmed();

                        QRegExp tmpRegexp2 = QRegExp("(\\d\\d:\\d\\d)");
                        tmpRegexp2.setMinimal(true);
                        tmpRegexp2.indexIn(tmpRegexp.cap(2).trimmed());

                        QTime time = QTime::fromString(tmpRegexp2.cap(1).trimmed(), "hh:mm");
                        if (time<lastTime) {
                            currentDate=currentDate.addDays(1);
                            lastTime=time;
                        }

                        QDateTime dateTime;
                        dateTime.setDate(currentDate);
                        dateTime.setTime(time);
                        if (tmpRegexp.cap(1)=="arr") {
                            fromStationItem.toInfo = info;
                            fromStationItem.toTime = dateTime;
                            arrFound=true;
                        }
                        else {
                            if (divMode==ROUTE_START)
                            {
                                fromStationItem.fromInfo = info;
                                fromStationItem.fromTime = dateTime;
                            }
                            else
                            {
                                toStationItem.fromInfo = info;
                                toStationItem.fromTime = dateTime;
                            }
                            depFound=true;

                        }
                        ++depArrIndex;
                    }

                }

                if (!depFound || !arrFound) {
                    tmpRegexp.setPattern("(arr|dep) ([0-9:]+)");
                    if (depFound)
                        tmpRegexp.setPattern("(arr) ([0-9:]+)");
                    if (arrFound)
                        tmpRegexp.setPattern("(dep) ([0-9:]+)");

                    tmpRegexp.setMinimal(false);
                    int depArrIndex=0;
                    while (depArrIndex>=0) {
                        depArrIndex=tmpRegexp.indexIn(tmp,depArrIndex);
                        if (depArrIndex>=0)
                        {
                            QString info = tmpRegexp.cap(2).trimmed();

                            QRegExp tmpRegexp2 = QRegExp("(\\d\\d:\\d\\d)");
                            tmpRegexp2.setMinimal(true);
                            tmpRegexp2.indexIn(tmpRegexp.cap(2).trimmed());

                            QTime time = QTime::fromString(tmpRegexp2.cap(1).trimmed(), "hh:mm");
                            if (time<lastTime) {
                                currentDate=currentDate.addDays(1);
                                lastTime=time;
                            }

                            QDateTime dateTime;
                            dateTime.setDate(currentDate);
                            dateTime.setTime(time);
                            if (tmpRegexp.cap(1)=="arr") {
                                fromStationItem.toInfo = info;
                                fromStationItem.toTime = dateTime;
                                arrFound=true;
                            }
                            else {
                                if (divMode==ROUTE_START)
                                {
                                    fromStationItem.fromInfo = info;
                                    fromStationItem.fromTime = dateTime;
                                }
                                else
                                {
                                    toStationItem.fromInfo = info;
                                    toStationItem.fromTime = dateTime;
                                }
                                depFound=true;

                            }
                            ++depArrIndex;
                        }

                    }

                }

                if (!depFound) {
                    tmpRegexp.setPattern("([0-9:]+) (min\\.)");
                    tmpRegexp.setMinimal(false);
                    tmpRegexp.indexIn(tmp);
                    if (tmpRegexp.cap(0) != "")
                    {
                        QString info = tmpRegexp.cap(1).trimmed() + " min.";

                        if (divMode==ROUTE_START)
                        {
                            fromStationItem.fromInfo = info;
                        }
                        else
                        {
                            toStationItem.fromInfo = info;
                        }
                        minFound=true;
                    }
                }


                if (!depFound && !arrFound && !minFound)
                {
                    tmp = tmp.trimmed();
                    fromStationItem.info.append(tmp);
                }

                if (divMode!=ROUTE_START) {
                    result.items.append(fromStationItem);
                    fromStationItem = toStationItem;
                    toStationItem = DetailResultItem();
                }
            }
            else
            {
                for (int i = 0; i < txtResults.count(); i++)
                {
                    QString tmp = txtResults[i];
                    tmp.replace("  ", " ");
                    tmp.replace("\n", " ");

                    //Check and Parse departure and arrival Times
                    QRegExp tmpRegexp = QRegExp("(Duration|Hint)(.*)$");
                    tmpRegexp.setMinimal(true);
                    tmpRegexp.indexIn(tmp);
                    if (tmpRegexp.cap(0) != "")
                    {
                        if (tmpRegexp.cap(1) == "Duration")
                        {
                            result.duration = tmpRegexp.cap(2).trimmed();
                        }
                        if (tmpRegexp.cap(1) == "Hint")
                        {
                            result.info = tmpRegexp.cap(2).trimmed();
                        }
                    }
                }
            }
            ++idx;
        }
    }

    //regexp = QRegExp("<div class=\"red bold\">(.*)</div>");
    regexp = QRegExp("<span class=\"red bold\">(.*)</span>");
    regexp.setMinimal(true);
    regexp.indexIn(xhtml);

    if (regexp.cap(1).trimmed() != "") {
        QBuffer readBuffer;
        QString element = "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n<body>\n" + regexp.cap(1) + "\n</body>\n</html>\n";

        //Remove misformed html
        QRegExp ahrefReg = QRegExp("<a(.*)>");
        ahrefReg.setMinimal(true);
        element.replace(ahrefReg, "");
        element.replace("&nbsp;", " ");
        ahrefReg = QRegExp("</a>");
        ahrefReg.setMinimal(true);
        element.replace(ahrefReg, "");
        element.replace("&nbsp;", " ");

        readBuffer.setData(element.toAscii());
        readBuffer.open(QIODevice::ReadOnly);

        QXmlQuery query;
        query.bindVariable("path", &readBuffer);
        query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/string()");

        QStringList errormsgsResults;
        if (!query.evaluateTo(&errormsgsResults))
        {
            qDebug() << "parserMobileBahnDe::getJourneyDetailsData - Query 4 Failed";
        }

        if (result.info != "") {
            result.info += "<br>";
        }
        result.info += errormsgsResults.join("").trimmed();
    }

    int secs=routeStart.secsTo(routeEnd);
    int hours=secs/3600;
    int minutes=(secs-hours*3600)/60;
    result.duration=QString("(%1h, %2m)").arg(hours).arg(minutes);
    return result;
}


ResultInfo parserMobileBahnDe::getJourneyData(QString queryUrl)
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

ResultInfo parserMobileBahnDe::getJourneyData(QString destinationStation, QString arrivalStation, QString viaStation, QDate date, QTime time, int mode, int trainrestrictions)
{
    Q_UNUSED(viaStation);

    QString trainrestr = "1:1111111111000000";
    if (trainrestrictions == 0) {
        trainrestr = "1:1111111111000000"; //ALL
    } else if (trainrestrictions == 1) {
        trainrestr = "2:0111111111000000"; //All without ICE
    } else if (trainrestrictions == 2) {
        trainrestr = "4:00011111110000000"; //Only local transport
    } else if (trainrestrictions == 3) {
        trainrestr = "4:0001011111000000"; //Only local transport without S-Bahn
    }

    QUrl url("http://mobile.bahn.de/bin/mobil/query.exe/eox?rt=1&use_realtime_filter=1&searchMode=NORMAL");

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
                          "&REQ0JourneyProduct_prod_list=" +
                          trainrestr +
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

    QString element = filebuffer->buffer();

    //Remove misformed html
    QRegExp ahrefReg = QRegExp("class=\"arrowlink\" href=\"(.*)\"");
    ahrefReg.setMinimal(true);
    element.replace(ahrefReg, "");

    #if 1
        ahrefReg = QRegExp("<script.*</script>");
        ahrefReg.setMinimal(true);
        element.replace(ahrefReg, "");

        /*ahrefReg = QRegExp("<ul class=\"neben \">");
        ahrefReg.setMinimal(true);
        element.replace(ahrefReg, "<ul><ul class=\"neben \">");*/
    #endif


    //Check if we need do a search by id, sometimes the search for the exact name
    //returns a selectbox again, so we check for it and if there are selectboxes
    //we search by the ids we get as response

    QBuffer readBuffer;
    readBuffer.setData(element.toAscii());
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    //Query for more than one result
    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/form/div/select[@name='REQ0JourneyStopsS0K']/option/string()");

    QStringList departureResult;
    if (!query.evaluateTo(&departureResult))
    {
        qDebug() << "parserMobileBahnDe::getJourneyData - Query 1 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/form/div/select[@name='REQ0JourneyStopsZ0K']/option/string()");

    QStringList arrivalResult;
    if (!query.evaluateTo(&arrivalResult))
    {
        qDebug() << "parserMobileBahnDe::getJourneyData - Query 2 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/form/div/select[@name='REQ0JourneyStopsS0K']/option/@value/string()");

    QStringList departureResultIds;
    if (!query.evaluateTo(&departureResultIds))
    {
        qDebug() << "parserMobileBahnDe::getJourneyData - Query 3 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/form/div/select[@name='REQ0JourneyStopsZ0K']/option/@value/string()");

    QStringList arrivalResultIds;
    if (!query.evaluateTo(&arrivalResultIds))
    {
        qDebug() << "parserMobileBahnDe::getJourneyData - Query 4 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/form/@action/string()");

    QStringList formUrl;
    if (!query.evaluateTo(&formUrl))
    {
        qDebug() << "parserMobileBahnDe::getJourneyData - Query 5 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/form/div/input[@name='REQ0JourneyStopsS0K']/@value/string()");

    QStringList destinationStationIds;
    if (!query.evaluateTo(&destinationStationIds))
    {
        qDebug() << "parserMobileBahnDe::getJourneyData - Query 6 Failed";
    }

    query.setQuery("declare default element namespace \"http://www.w3.org/1999/xhtml\"; declare variable $path external; doc($path)/html/body/div/div/div/form/div/input[@name='REQ0JourneyStopsZ0K']/@value/string()");

    QStringList arrivalStationIds;
    if (!query.evaluateTo(&arrivalStationIds))
    {
        qDebug() << "parserMobileBahnDe::getJourneyData - Query 7 Failed";
    }

    if (departureResult.count() > 0 || arrivalResult.count() > 0)
    {
        QString destinationStationId = "";
        QString arrivalStationId = "";

        if (departureResult.count() > 0)
        {
            for (int i = 0; i < departureResult.count(); i++)
            {
                if ((QString)departureResult[i].trimmed() == destinationStation.trimmed())
                {
                    destinationStationId = departureResultIds[i];
                }
            }
        } else
        {
            destinationStationId = destinationStationIds[0];
        }

        if (arrivalResult.count() > 0)
        {
            for (int i = 0; i < arrivalResult.count(); i++)
            {
                if ((QString)arrivalResult[i].trimmed() == arrivalStation.trimmed())
                {
                    arrivalStationId = arrivalResultIds[i];
                }
            }
        } else
        {
            arrivalStationId = arrivalStationIds[0];
        }

        QUrl newUrl(formUrl[0]);

        postData = "REQ0JourneyStopsS0A=1&REQ0JourneyStopsS0K=" +
                                  destinationStationId +
                                  "&REQ0JourneyStopsS0ID=&REQ0JourneyStopsZ0A=1&REQ0JourneyStopsZ0K=" +
                                  arrivalStationId +
                                  "&REQ0JourneyStopsZ0ID=&REQ0JourneyDate=" +
                                  date.toString("dd.MM.yyyy") +
                                  "&REQ0JourneyTime=" +
                                  time.toString("hh:mm") +
                                  "&REQ0HafasSearchForw=" +
                                  QString::number(mode) +
                                  "&REQ0JourneyProduct_prod_list=" +
                                  trainrestr +
                                  "&existOptimizePrice=1&REQ0HafasOptimize1=0%3A1&existProductNahverkehr=yes&REQ0Tariff_TravellerAge.1=35&start=Suchen&REQ0Tariff_Class=2&REQ0Tariff_TravellerReductionClass.1=0";
        filebuffer = new QBuffer();

        if (!filebuffer->open(QIODevice::WriteOnly))
        {
            qDebug() << "Can't open Buffer";
        }

        header.setRequest("POST", newUrl.path() + "?" + newUrl.encodedQuery());
        header.setValue("Host", newUrl.host());

        currentRequestId = http->request(header, postData.toAscii(), filebuffer);

        loop.exec();

        filebuffer->close();
    }

    return parseJourneyData(filebuffer->buffer());
}

QStringList parserMobileBahnDe::getStationsByGPS(qreal latitude, qreal longitude)
{
    //We must format the lat and longitude to have the ??.?????? format.
    QString zeros      = "0";
    QString sLongitude = QString::number(longitude).append(zeros.repeated(6));
    QString sLatitude  = QString::number(latitude).append(zeros.repeated(6));

    QRegExp regexp = QRegExp("(\\d*)\\.(\\d\\d\\d\\d\\d\\d)(\\d*)");
    regexp.setMinimal(true);

    regexp.indexIn(sLongitude);
    sLongitude = regexp.cap(1) + regexp.cap(2);

    regexp.indexIn(sLatitude);
    sLatitude = regexp.cap(1) + regexp.cap(2);

    QString fullUrl = "http://mobile.bahn.de/bin/mobil/query.exe/eol?look_x=" +
                      sLongitude +
                      "&look_y=" +
                      sLatitude +
                      "&performLocating=2&tpl=stopsnear&L=vs_java&look_maxdist=5000&look_maxno=40";

    //qDebug()<<"GPS REQUEST URL:"<<fullUrl;

    //qDebug()<<"GPS RAW VALUES: LAT"<<latitude<<" LONG:"<<longitude;

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

    QBuffer readBuffer;
    readBuffer.setData(filebuffer->buffer());
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    //Query for Gps infos
    query.setQuery("doc($path)/ResC/MLcRes/MLc[@t='ST']/@n/string()");

    QStringList result;
    if (!query.evaluateTo(&result))
    {
        qDebug() << "parserMobileBahnDe::getStationsByGPS - Query Failed";
    }

    delete filebuffer;

    result.removeDuplicates();

    return result;
}


QStringList parserMobileBahnDe::getStationsByName(QString stationName)
{
    stationName.replace("\"", "");
    QByteArray postData = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><ReqC ver=\"1.1\" prod=\"String\" lang=\"EN\"><MLcReq><MLc n=\"";
    postData.append(stationName);
    postData.append("\" t=\"ST\" /></MLcReq></ReqC>");

    qDebug() << "parserMobileBahnDe::getStationsByName";

    QUrl url("http://mobile.bahn.de/bin/mobil/query.exe/en");

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

    QBuffer readBuffer;
    readBuffer.setData(filebuffer->buffer());
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    //Query for station infos
    query.setQuery("doc($path)/ResC/MLcRes/MLc[@t='ST']/@n/string()");

    QStringList result;
    if (!query.evaluateTo(&result))
    {
        qDebug() << "parserMobileBahnDe::getStationsByName - Query Failed";
    }

    delete filebuffer;

    return result;
}
