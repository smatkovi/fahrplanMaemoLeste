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

#include "parser_hafasxml.h"

parserHafasXml::parserHafasXml(QObject *parent)
{
     Q_UNUSED(parent);
     http = new QHttp(this);

     QNetworkAccessManager *NetworkManager = new QNetworkAccessManager(this);
     QNetworkRequest request;

     connect(http, SIGNAL(requestFinished(int,bool)), this, SLOT(httpRequestFinished(int,bool)));
     connect(http, SIGNAL(dataReadProgress(int,int)), this, SLOT(httpDataReadProgress(int,int)));

     connect(NetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

//     QNetworkReply *reply = NetworkManager->get(request);
//     connect(NetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
//     connect(reply, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
//      connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(slotError(QNetworkReply::NetworkError)));
//      connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(slotSslErrors(QList<QSslError>)));



     //baseUrl = "http://fahrplan.oebb.at/bin/query.exe"; //OEB (fully operational/no RT) //no xmlhandle, detaildate already present!
     //baseUrl = "http://hafas.bene-system.com/bin/query.exe"; //hafas dev?? system? / no gps
     //baseUrl = "http://reiseauskunft.bahn.de/bin/query.exe"; //bahn.de (journey stuff fails)
     //baseUrl = "http://fahrplan.sbb.ch/bin/query.exe"; //SBB (only returns one journey) / Xmlhandle present
     //baseUrl = "http://www.fahrplaner.de/hafas/query.exe"; //?? No Gps, returns only one result
     //baseUrl = "http://www.rejseplanen.dk/bin/query.exe";//?? No Gps, returns only one result //no xmlhandle, detaildate already present!
     //baseUrl = "http://airs1.septa.org/bin/query.exe";// not working at all

     hafasHeader.accessid = "";
     hafasHeader.prod = "String";
     hafasHeader.ver = "1.1";
}

bool parserHafasXml::supportsGps()
{
    return true;
}

DetailResultInfo parserHafasXml::getJourneyDetailsData(QString queryUrl)
{
    QString id = queryUrl;
    id.truncate(queryUrl.indexOf("="));
    queryUrl.remove(0, queryUrl.indexOf("=") + 1);

    DetailResultInfo results;
    for(int i = 0; i < lastItems.count(); i++) {
        if (lastItems[i].id == id)
        {
            if (lastItems[i].detailsInfo.items.count() <= 0)
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

                QBuffer readBuffer;
                readBuffer.setData(filebuffer->buffer());
                readBuffer.open(QIODevice::ReadOnly);

                lastItems[i].detailsInfo = parseJourneyDataDetails(filebuffer->buffer(), id, lastItems[i].tripDate);
            }

            results = lastItems[i].detailsInfo;
            results.duration = lastItems[i].duration;
            return results;
        }
    }
    return results;
}

ResultInfo parserHafasXml::getJourneyData(QString queryUrl)
{
    QString direction;
    bool append;

    if (queryUrl.startsWith("F")) {
        direction = "F";
        queryUrl.remove(0,1);
        append = true;
    }
    if (queryUrl.startsWith("B")) {
        direction = "B";
        queryUrl.remove(0,1);
        append = false;
    }

    QByteArray apostData = "";
    apostData.append("<?xml version=\"1.0\" encoding=\"UTF-8\" ?><ReqC  accessId=\"" + hafasHeader.accessid + "\" ver=\"" + hafasHeader.ver + "\" prod=\"" + hafasHeader.prod + "\" lang=\"EN\">");
    apostData.append("<ConScrReq scrDir=\"");
    apostData.append(direction);
    apostData.append("\" nrCons=\"5\">");
    apostData.append("<ConResCtxt>");
    apostData.append(queryUrl);
    apostData.append("</ConResCtxt>");
    apostData.append("</ConScrReq>");
    apostData.append("</ReqC>");

    QUrl aurl(baseUrl);

    http->setHost(aurl.host(), QHttp::ConnectionModeHttp, aurl.port() == -1 ? 0 : aurl.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    QHttpRequestHeader aheader;
    aheader.setRequest("POST", aurl.path() + "?" + aurl.encodedQuery());
    aheader.setValue("Host", aurl.host());

    currentRequestId = http->request(aheader, apostData, filebuffer);

    loop.exec();

    filebuffer->close();

    QBuffer readBuffer;
    readBuffer.setData(filebuffer->buffer());
    readBuffer.open(QIODevice::ReadOnly);

    return parseJourneyData(filebuffer->buffer(), append);
}

ResultInfo parserHafasXml::parseJourneyData(QByteArray data, bool append)
{
    QBuffer readBuffer;
    readBuffer.setData(data);
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    query.setQuery("doc($path)/ResC/Err//@text/string()");

    ResultInfo results;

    QStringList errorResult;
    if (!query.evaluateTo(&errorResult))
    {
        results.errorMsg = tr("Internal Error occured!");
        return results;
        qDebug() << "parserHafasXml::ErrorTest - Query Failed";
    }

    if (errorResult.count() > 0 ) {
        results.errorMsg = errorResult.join("");
        return results;
    }

    //Query for station infos
    query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection/@id/string()");

    QStringList resultIds;
    if (!query.evaluateTo(&resultIds))
    {
        qDebug() << "parserHafasXml::getJourneyData 2 - Query Failed";
    }

    if (resultIds.count() <= 0) {
        return results;
    }

    for(int i = 0;i<resultIds.count(); i++) {
        //qDebug()<<"Connection:"<<resultIds[i];

        query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection[@id='" + resultIds[i] + "']/Overview/Date/string()");
        QStringList dateResult;
        if (!query.evaluateTo(&dateResult))
        {
            qDebug() << "parserHafasXml::getJourneyData 3 - Query Failed";
        }

        query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection[@id='" + resultIds[i] + "']/Overview/Transfers/string()");
        QStringList transfersResult;
        if (!query.evaluateTo(&transfersResult))
        {
            qDebug() << "parserHafasXml::getJourneyData 4 - Query Failed";
        }

        query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection[@id='" + resultIds[i] + "']/Overview/Duration/Time/string()");
        QStringList durationResult;
        if (!query.evaluateTo(&durationResult))
        {
            qDebug() << "parserHafasXml::getJourneyData 5 - Query Failed";
        }

        query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection[@id='" + resultIds[i] + "']/Overview/Products/Product/@cat/string()");
        QStringList trainsResult;
        if (!query.evaluateTo(&trainsResult))
        {
            qDebug() << "parserHafasXml::getJourneyData 6 - Query Failed";
        }

        query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection[@id='" + resultIds[i] + "']/Overview/Departure/BasicStop/Station/@name/string()");
        QStringList depResult;
        if (!query.evaluateTo(&depResult))
        {
            qDebug() << "parserHafasXml::getJourneyData 7 - Query Failed";
        }

        query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection[@id='" + resultIds[i] + "']/Overview/Arrival/BasicStop/Station/@name/string()");
        QStringList arrResult;
        if (!query.evaluateTo(&arrResult))
        {
            qDebug() << "parserHafasXml::getJourneyData 8 - Query Failed";
        }

        query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection[@id='" + resultIds[i] + "']/Overview/Departure/BasicStop/Dep/Time/string()");
        QStringList depTimeResult;
        if (!query.evaluateTo(&depTimeResult))
        {
            qDebug() << "parserHafasXml::getJourneyData 9 - Query Failed";
        }

        query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection[@id='" + resultIds[i] + "']/Overview/Departure/BasicStop/Dep/Platform/Text/string()");
        QStringList depPlatResult;
        if (!query.evaluateTo(&depPlatResult))
        {
            qDebug() << "parserHafasXml::getJourneyData 10 - Query Failed";
        }

        query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection[@id='" + resultIds[i] + "']/Overview/Arrival/BasicStop/Arr/Time/string()");
        QStringList arrTimeResult;
        if (!query.evaluateTo(&arrTimeResult))
        {
            qDebug() << "parserHafasXml::getJourneyData 11 - Query Failed";
        }

        query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection[@id='" + resultIds[i] + "']/Overview/Arrival/BasicStop/Arr/Platform/Text/string()");
        QStringList arrPlatResult;
        if (!query.evaluateTo(&arrPlatResult))
        {
            qDebug() << "parserHafasXml::getJourneyData 12 - Query Failed";
        }

        query.setQuery("doc($path)/ResC/ConRes/ConnectionList/Connection[@id='" + resultIds[i] + "']/Overview/XMLHandle/@url/string()");
        QStringList xmlHandleResult;
        if (!query.evaluateTo(&xmlHandleResult))
        {
            qDebug() << "parserHafasXml::getJourneyData 13 - Query Failed";
        }

        /*
        qDebug()<<"  Date:"<<dateResult.join("").trimmed();
        qDebug()<<"  Transfers:"<<transfersResult.join("").trimmed();
        qDebug()<<"  Duration:"<<durationResult;
        qDebug()<<"  Trains:"<<trainsResult;
        qDebug()<<"  DepartureSt:"<<depResult;
        qDebug()<<"  DepartureTime:"<<depTimeResult;
        qDebug()<<"  DeparturePlatform:"<<depPlatResult;
        qDebug()<<"  ArrivalSt:"<<arrResult;
        qDebug()<<"  ArrivalTime:"<<arrTimeResult;
        qDebug()<<"  ArrivalPlatform:"<<arrPlatResult;
        qDebug()<<"  xmlHandle:"<<xmlHandleResult;
        */

        QDate date = QDate::fromString(dateResult.join("").trimmed(), "yyyyMMdd");
        ResultItem item;
        item.tripDate   = date;
        item.id         = resultIds[i];
        item.changes    = transfersResult.join("").trimmed();
        item.duration   = cleanHafasDate(durationResult.join("").trimmed());
        item.state      = "";
        item.trainType  = trainsResult.join(",").trimmed();
        item.fromTime   = cleanHafasDate(depTimeResult.join("").trimmed());
        item.toTime     = cleanHafasDate(arrTimeResult.join("").trimmed());
        item.detailsUrl = xmlHandleResult.join("").trimmed();

        bool hasInline = false;

        if (item.detailsUrl.count() > 0 && item.detailsUrl.indexOf("extxml.exe")) {
            hasInline = true;
        }

        if (item.detailsUrl.count() > 0) {
            item.detailsUrl.remove(0, item.detailsUrl.indexOf("query.exe") + 9);
            item.detailsUrl.prepend(baseUrl);
        } else {
            hasInline = true;
        }

        if (hasInline) {
            item.detailsInfo = parseJourneyDataDetails(data, resultIds[i], date);
        }

        item.detailsUrl.prepend(resultIds[i] + "=");

        if (append) {
            lastItems.append(item);
        } else {
            lastItems.insert(i, item);
        }

        results.fromStation = depResult.join("").trimmed();
        results.toStation = arrResult.join("").trimmed();
        results.timeInfo = date.toString();
    }

    //Query for next and prev stuff
    query.setQuery("doc($path)/ResC/ConRes/ConResCtxt/string()");
    QStringList ConResCtxtResult;
    if (!query.evaluateTo(&ConResCtxtResult))
    {
        qDebug() << "parserHafasXml::getJourneyData 14 - Query Failed";
    }

    results.laterUrl = "F" + ConResCtxtResult.join("");
    results.earlierUrl = "B" + ConResCtxtResult.join("");
    results.items = lastItems;

    return results;
}


QString parserHafasXml::getTrainRestrictionsCodes(int trainrestrictions)
{
    QString trainrestr = "1111111111111111";
    if (trainrestrictions == 0) {
        trainrestr = "1111111111111111"; //ALL
    } else if (trainrestrictions == 1) {
        trainrestr = "0111111111000000"; //All without ICE
    } else if (trainrestrictions == 2) {
        trainrestr = "00011111110000000"; //Only local transport
    } else if (trainrestrictions == 3) {
        trainrestr = "0001011111000000"; //Only local transport without sbahn
    }

    return trainrestr;
}


QStringList parserHafasXml::getTrainRestrictions()
{
    QStringList result;
    result.append(tr("All"));
    result.append(tr("All without ICE"));
    result.append(tr("Only local transport"));
    result.append(tr("Local transport without S-Bahn"));
    return result;
}

ResultInfo parserHafasXml::getJourneyData(QString destinationStation, QString arrivalStation, QString viaStation, QDate date, QTime time, int mode, int trainrestrictions)
{
    Q_UNUSED(viaStation);
    lastItems.clear();

    QString trainrestr = getTrainRestrictionsCodes(trainrestrictions);

    //First Request, to get external ids

    QByteArray apostData = "";
    apostData.append("<?xml version=\"1.0\" encoding=\"UTF-8\" ?><ReqC  accessId=\"" + hafasHeader.accessid + "\" ver=\"" + hafasHeader.ver + "\" prod=\"" + hafasHeader.prod + "\" lang=\"EN\">");
    apostData.append("<LocValReq id=\"from\" maxNr=\"1\"><ReqLoc match= \"");
    apostData.append(destinationStation);
    apostData.append("\" type=\"ST\"/></LocValReq>");
    apostData.append("<LocValReq id=\"to\" maxNr=\"1\"><ReqLoc match= \"");
    apostData.append(arrivalStation);
    apostData.append("\" type=\"ST\"/></LocValReq>");
    apostData.append("</ReqC>");

    QUrl aurl(baseUrl);

    http->setHost(aurl.host(), QHttp::ConnectionModeHttp, aurl.port() == -1 ? 0 : aurl.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    QHttpRequestHeader aheader;
    aheader.setRequest("POST", aurl.path() + "?" + aurl.encodedQuery());
    aheader.setValue("Host", aurl.host());

    currentRequestId = http->request(aheader, apostData, filebuffer);

    loop.exec();

    filebuffer->close();

    QBuffer readBuffer;
    readBuffer.setData(filebuffer->buffer());
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    //Query for station infos
    query.setQuery("doc($path)/ResC/LocValRes/Station/@externalId/string()");

    QStringList stationsResult;
    if (!query.evaluateTo(&stationsResult))
    {
        qDebug() << "parserHafasXml::getJourneyData 1 - Query Failed";
    }

    //Second request with the search itself

    apostData = "";
    apostData.append("<?xml version=\"1.0\" encoding=\"UTF-8\" ?><ReqC  accessId=\"" + hafasHeader.accessid + "\" ver=\"" + hafasHeader.ver + "\" prod=\"" + hafasHeader.prod + "\" lang=\"EN\">");
    apostData.append("<ConReq>");
    apostData.append("<Start min=\"0\">");
    apostData.append("<Station externalId=\"");
    apostData.append(stationsResult[0]);
    apostData.append("\" distance=\"0\"></Station>");
    apostData.append("<Prod prod=\"");
    apostData.append(trainrestr);
    apostData.append("\"></Prod>");
    apostData.append("</Start>");
    apostData.append("<Dest min=\"0\">");
    apostData.append("<Station externalId=\"");
    apostData.append(stationsResult[1]);
    apostData.append("\" distance=\"0\"></Station>");
    apostData.append("</Dest>");
    apostData.append("<Via></Via>");
    apostData.append("<ReqT time=\"");
    apostData.append(time.toString("hh:mm"));
    apostData.append("\" date=\"");
    apostData.append(date.toString("yyyyMMdd"));
    apostData.append("\" a=\"");
    apostData.append((mode == 0) ? "1" : "0");
    apostData.append("\"></ReqT>");

    if (mode == 0) {
        apostData.append("<RFlags b=\"");
        apostData.append("4"); //From count
        apostData.append("\" f=\"");
        apostData.append("1"); //To count
        apostData.append("\"></RFlags>");
    } else {
        apostData.append("<RFlags b=\"");
        apostData.append("1"); //From count
        apostData.append("\" f=\"");
        apostData.append("4"); //To count
        apostData.append("\"></RFlags>");
    }

    apostData.append("<GISParameters><Front></Front><Back></Back></GISParameters>");
    apostData.append("</ConReq>");
    apostData.append("</ReqC>");

    http->setHost(aurl.host(), QHttp::ConnectionModeHttp, aurl.port() == -1 ? 0 : aurl.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    aheader.setRequest("POST", aurl.path() + "?" + aurl.encodedQuery());
    aheader.setValue("Host", aurl.host());

    currentRequestId = http->request(aheader, apostData, filebuffer);

    loop.exec();

    filebuffer->close();

    ResultInfo results = parseJourneyData(filebuffer->buffer(), true);

    //Some interfaces only return one result so we call the laterUrl to get more :)
    if (results.items.count() <= 1) {
        results = getJourneyData(results.laterUrl);
    }

    return results;
}

DetailResultInfo parserHafasXml::parseJourneyDataDetails(QByteArray data, QString connectionId, QDate date)
{
    QBuffer readBuffer;
    readBuffer.setData(data);
    readBuffer.open(QIODevice::ReadOnly);

    QXmlQuery query;
    query.bindVariable("path", &readBuffer);
    query.setQuery("doc($path)/ResC/Err//@text/string()");

    DetailResultInfo results;

    QStringList errorResult;
    if (!query.evaluateTo(&errorResult))
    {
        qDebug() << "parserHafasXml::ErrorTest - Query Failed";
        return results;
    }

    if (errorResult.count() > 0 ) {
        return results;
    }

    query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection/Departure/BasicStop/Station/@name/string()");
    QStringList departureResults;
    if (!query.evaluateTo(&departureResults))
    {
        qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 1 Failed";
    }
    query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection/Arrival/BasicStop/Station/@name/string()");
    QStringList arrivalResults;
    if (!query.evaluateTo(&arrivalResults))
    {
        qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 2 Failed";
    }

    query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection/Departure/BasicStop/Location/Station/HafasName/Text/string()");
    QStringList departure2Results;
    if (!query.evaluateTo(&departure2Results))
    {
        qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 1b Failed";
    }
    query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection/Arrival/BasicStop/Location/Station/HafasName/Text/string()");
    QStringList arrival2Results;
    if (!query.evaluateTo(&arrival2Results))
    {
        qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 2b Failed";
    }

    query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection/Departure/BasicStop/Dep/Time/string()");
    QStringList depTimeResult;
    if (!query.evaluateTo(&depTimeResult))
    {
        qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 3 Failed";
    }

    query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection/Departure/BasicStop/Dep/Platform/Text/string()");
    QStringList depPlatResult;
    if (!query.evaluateTo(&depPlatResult))
    {
        qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 4 Failed";
    }

    query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection/Arrival/BasicStop/Arr/Time/string()");
    QStringList arrTimeResult;
    if (!query.evaluateTo(&arrTimeResult))
    {
        qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 5 Failed";
    }

    query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection/Arrival/BasicStop/Arr/Platform/Text/string()");
    QStringList arrPlatResult;
    if (!query.evaluateTo(&arrPlatResult))
    {
        qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 6 Failed";
    }

    //It is possible, that the stationname is in two seperate fields
    if (departureResults.count() == 0 && departure2Results.count() > 0)
    {
        departureResults = departure2Results;
        arrivalResults = arrival2Results;
    }

    if (departureResults.count() == arrivalResults.count())
    {
        for(int i = 0; i < departureResults.count(); i++)
        {
           DetailResultItem item;
           /*
           qDebug()<<"   "<<"Journey "<<i;
           qDebug()<<"     DepartureSt:"<<departureResults[i].trimmed();
           qDebug()<<"     DepartureTime:"<<depTimeResult[i].trimmed();
           qDebug()<<"     DeparturePlatform:"<<depPlatResult[i].trimmed();
           qDebug()<<"     ArrivalSt:"<<arrivalResults[i].trimmed();
           qDebug()<<"     ArrivalTime:"<<arrTimeResult[i].trimmed();
           qDebug()<<"     ArrivalPlatform:"<<arrPlatResult[i].trimmed();
           */

           item.fromTime  = cleanHafasDateTime(depTimeResult[i].trimmed(),date);
           item.toTime    = cleanHafasDateTime(arrTimeResult[i].trimmed(),date);
           item.fromStation = departureResults[i].trimmed();
           item.toStation = arrivalResults[i].trimmed();
           item.fromInfo  = item.fromTime.toString("hh:mm");
           if (depPlatResult[i].trimmed() != "")
           {
                item.fromInfo  +=  " " + tr("Platform ") + depPlatResult[i].trimmed();
           }
           item.fromInfo += " " + tr("on") + " " + item.fromTime.toString("dd.MM.yyyy");
           item.toInfo  = item.toTime.toString("hh:mm");
           if (arrPlatResult[i].trimmed() != "")
           {
                item.toInfo    += " " + tr("Platform ") + arrPlatResult[i].trimmed();
           }
           item.toInfo += " " + tr("on") + " " + item.toTime.toString("dd.MM.yyyy");

           //Check for train or if walking
           query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection[" + QString::number(i + 1) + "]/Journey/JourneyAttributeList/JourneyAttribute/Attribute[@type='NAME']/AttributeVariant/Text/string()");
           QStringList trainResult;
           if (!query.evaluateTo(&trainResult))
           {
               qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 7 Failed";
           }

           if (trainResult.count() > 0)
           {
               //qDebug()<<"     Train:"<<trainResult.join("").trimmed();
               item.train = trainResult.join("").trimmed();
           } else
           {
               query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection[" + QString::number(i + 1) + "]/Walk/Duration/Time/string()");
               QStringList walkResult;
               if (!query.evaluateTo(&walkResult))
               {
                   qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 8 Failed";
               }

               //Maybe its gisroute?
               if (walkResult.count() == 0)
               {
                   query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection[" + QString::number(i + 1) + "]/GisRoute/Duration/Time/string()");
                   QStringList gisrouteResult;
                   if (!query.evaluateTo(&gisrouteResult))
                   {
                       qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 9 Failed";
                   }

                   //Ok its a gisroute
                   if (gisrouteResult.count() > 0)
                   {
                       query.setQuery("doc($path)/ResC/ConRes//Connection[@id='" + connectionId + "']/ConSectionList/ConSection[" + QString::number(i + 1) + "]/GisRoute/@type/string()");
                       QStringList gisroutetypeResult;
                       if (!query.evaluateTo(&gisroutetypeResult))
                       {
                           qDebug() << "parserHafasXml::parseJourneyDataDetails - Query 10 Failed";
                       }

                       QString gisrouteType = gisroutetypeResult.join("").trimmed();
                       if (gisrouteType == "FOOT")
                       {
                         item.info = tr("Walk for ") + cleanHafasDate(gisrouteResult.join("").trimmed()+ tr(" min."));
                       } else if (gisrouteType == "BIKE")
                       {
                         item.info = tr("Use a Bike for ") + cleanHafasDate(gisrouteResult.join("").trimmed()+ tr(" min."));
                       } else if (gisrouteType == "CAR")
                       {
                         item.info = tr("Use a car for ") + cleanHafasDate(gisrouteResult.join("").trimmed()+ tr(" min."));
                       } else if (gisrouteType == "TAXI")
                       {
                          item.info = tr("Take a taxi for ") + cleanHafasDate(gisrouteResult.join("").trimmed()+ tr(" min."));
                       }
                   }
               } else {
                   item.info = tr("Walk for ") + cleanHafasDate(walkResult.join("").trimmed() + tr(" min."));
               }
           }

           results.items.append(item);
        }
    } else
    {
        qDebug()<<"Something went wrong ;(";
    }

    return results;
}

QDateTime parserHafasXml::cleanHafasDateTime(QString time, QDate date)
{
    QDateTime result;
    result.setDate(date);
    QRegExp tmpRegexp = QRegExp("(\\d\\d)d(\\d\\d):(\\d\\d):(\\d\\d)");
    tmpRegexp.setMinimal(true);
    tmpRegexp.indexIn(time);

    result.setTime(QTime::fromString(tmpRegexp.cap(2) + tmpRegexp.cap(3) + tmpRegexp.cap(4), "hhmmss"));

    if (tmpRegexp.cap(1) != "00") {
        result = result.addDays(tmpRegexp.cap(1).toInt());
    }

    return result;
}

QString parserHafasXml::cleanHafasDate(QString time)
{
    QRegExp tmpRegexp = QRegExp("(\\d\\d)d(\\d\\d):(\\d\\d):(\\d\\d)");
    tmpRegexp.setMinimal(true);
    tmpRegexp.indexIn(time);

    QString result = "";
    if (tmpRegexp.cap(1) != "00") {
        result.append(tmpRegexp.cap(1) + tr("d") + " ");
    }

    result.append(tmpRegexp.cap(2) + ":");
    result.append(tmpRegexp.cap(3));

    return result;
}

QStringList parserHafasXml::getStationsByGPS(qreal latitude, qreal longitude)
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

    QString fullUrl = baseUrl + "/eol?look_x=" +
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
        qDebug() << "parserHafasXml::getStationsByGPS - Query Failed";
    }

    delete filebuffer;

    result.removeDuplicates();

    return result;
}


QStringList parserHafasXml::getStationsByName(QString stationName)
{
    stationName.replace("\"", "");
    QByteArray postData = "";
    postData.append("<?xml version=\"1.0\" encoding=\"UTF-8\" ?><ReqC  accessId=\"" + hafasHeader.accessid + "\" ver=\"" + hafasHeader.ver + "\" prod=\"" + hafasHeader.prod + "\" lang=\"EN\"><MLcReq><MLc n=\"");
    postData.append(stationName);
    postData.append("\" t=\"ST\" /></MLcReq></ReqC>");

    qDebug() << "parserHafasXml::getStationsByName";

    QUrl url(baseUrl);

    http->setHost(url.host(), QHttp::ConnectionModeHttp, url.port() == -1 ? 0 : url.port());


//    request.setUrl(url);
//    request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");






    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    QHttpRequestHeader header;
    header.setRequest("POST", url.path() + "?" + url.encodedQuery());
    header.setValue("Host", url.host());

    currentRequestId = http->request(header, postData, filebuffer);


//    request.setHeader(
//    request.

//    NetworkManager->post(request, postData);



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
        qDebug() << "parserHafasXml::getStationsByName - Query Failed";
    }

    delete filebuffer;

    return result;
}
