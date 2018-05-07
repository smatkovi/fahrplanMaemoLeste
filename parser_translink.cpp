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

#include "parser_translink.h"

QString encodeStringAsFormData(const QString &array)
{
    static const char hexDigits[17] = "0123456789ABCDEF";

    // Same safe characters as Netscape for compatibility.
    static const char safeCharacters[] = "!\'()*-._~";

    QString buffer;

    // http://www.w3.org/TR/html4/interact/forms.html#h-17.13.4.1
    unsigned length = array.length();
    for (unsigned i = 0; i < length; ++i) {
        const unsigned char c = array.at(i).toAscii();

        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || strchr(safeCharacters, c))
            buffer += c;
        else if (c == ' ')
            buffer += '+';
        else if (c == '\n' || (c == '\r' && (i + 1 >= length || array.at(i + 1).toAscii() != '\n')))
            buffer += "%0D%0A";
        else if (c != '\r') {
            buffer += '%';
            buffer += hexDigits[c >> 4];
            buffer += hexDigits[c & 0xF];
        }
    }
    return buffer;
}

void parserTranslink::tsslErrors(const QList<QSslError> & errors)
{
    qDebug() << "SSL errors found";
    foreach( const QSslError &error, errors )
    {
        qDebug() << "SSL Error: " << error.errorString();
    }
}

parserTranslink::parserTranslink(QObject *parent)
{
     Q_UNUSED(parent);
     http = new QHttp(this);
     QSslSocket* sslSocket = new QSslSocket(this);
     sslSocket->setProtocol(QSsl::TlsV1);
     sslSocket->setPeerVerifyName("app.jp.translink.com.au");
     http->setSocket(sslSocket);

     connect(http, SIGNAL(requestFinished(int,bool)),
             this, SLOT(httpRequestFinished(int,bool)));
     connect(http, SIGNAL(sslErrors(const QList<QSslError> &)),
             this, SLOT(tsslErrors(const QList<QSslError> &)));
}

bool parserTranslink::supportsGps()
{
    return true;
}

QStringList parserTranslink::getStationsByName(QString stationName)
{
    QByteArray postData = "location=";
    postData.append(encodeStringAsFormData(stationName));

    qDebug() << "Translink: getStationsByName";

    QUrl url("https://app.jp.translink.com.au/plan-your-journey/location/find-location");

    http->setHost(url.host(), QHttp::ConnectionModeHttps, url.port() == -1 ? 0 : url.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    QHttpRequestHeader header;
    header.setRequest("POST", url.path());
    header.setValue("Host", url.host());
    header.setValue("Content-Type", "application/x-www-form-urlencoded");
    header.setValue("Content-Length", QString::number(postData.length()));

    currentRequestId = http->request(header, postData, filebuffer);

    loop.exec();

    filebuffer->close();

    QJson::Parser parser;

    bool ok;

    QStringList result;
    QVariantList list = parser.parse(filebuffer->buffer(),&ok).toList();
    foreach (QVariant station, list)
    {
        QVariantMap map = station.toMap();
        QString str;
        str = map["Description"].toString();
        str.append("@");
        str.append(map["LocationId"].toString());
        result.append(str);
    }

    return result;
}

QStringList parserTranslink::getTrainRestrictions()
{
    QStringList result;
    return result;
}

QStringList parserTranslink::getStationsByGPS(qreal latitude, qreal longitude)
{
    QByteArray postData = "lat=";
    postData.append(QString::number(latitude));
    postData.append("&lng=");
    postData.append(QString::number(longitude));
    postData.append("&includePostcode=true&includeCountry=false");

    qDebug() << "Translink: getStationsByGPS";

    QUrl url("https://app.jp.translink.com.au/plan-your-journey/location/reverse-geocode-coords");

    http->setHost(url.host(), QHttp::ConnectionModeHttps, url.port() == -1 ? 0 : url.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    QHttpRequestHeader header;
    header.setRequest("POST", url.path());
    header.setValue("Host", url.host());
    header.setValue("Content-Type", "application/x-www-form-urlencoded");
    header.setValue("Content-Length", QString::number(postData.length()));

    currentRequestId = http->request(header, postData, filebuffer);

    loop.exec();

    filebuffer->close();
    QByteArray arr = filebuffer->buffer();
    QStringList result;
    QStringList result2;
    if (arr.size() > 0)
    {
        arr.remove(arr.size() - 1,1);
        arr.remove(0,1);
        QString str;
        str = arr;
        str.append("@");
        str.append("GP:");
        str.append(QString::number(latitude));
        str.append(",");
        str.append(QString::number(longitude));
        result.append(str);
    }    

    return result;
}

ResultInfo parserTranslink::getJourneyData(QString destinationStation, QString arrivalStation, QString viaStation, QDate date, QTime time, int mode, int trainrestrictions)
{
    Q_UNUSED(viaStation);
    Q_UNUSED(trainrestrictions);
    QByteArray postData = "Start=";
    postData.append(encodeStringAsFormData(destinationStation.section('@',0,0)));
    postData.append("&End=");
    postData.append(encodeStringAsFormData(arrivalStation.section('@',0,0)));
    postData.append("&startLocationId=");
    postData.append(encodeStringAsFormData(destinationStation.section('@',1,1)));
    postData.append("&endLocationId=");
    postData.append(encodeStringAsFormData(arrivalStation.section('@',1,1)));
    postData.append("&SearchDate=");
    postData.append(encodeStringAsFormData(date.toString("d/MM/yyyy")));
    postData.append("+12\%3A00\%3A00+AM");
    postData.append("&TimeSearchMode=");
    if (mode == 0)
    {
        postData.append("ArriveBefore");
    }
    else
    {
        postData.append("LeaveAfter");
    }
    postData.append("&SearchTime=");
    postData.append(encodeStringAsFormData(time.toString("h:mmap")));
    postData.append("&SearchMinute=");
    postData.append(time.toString("m"));
    postData.append("&SearchHour=");
    int hour = time.hour();
    if (hour == 0)
    {
        hour = 12;
    }
    if (hour > 12)
    {
        hour -= 12;
    }
    postData.append(QString::number(hour));
    postData.append("&TimeMeridiem=");
    postData.append(time.toString("AP"));
    postData.append("&TransportModes%5B%5D=Bus&TransportModes%5B%5D=Train&TransportModes%5B%5D=Ferry&TransportModes%5B%5D=Tram&MaximumWalkingDistance=1500&WalkingSpeed=Normal&ServiceTypes%5B%5D=Regular&ServiceTypes%5B%5D=Express&ServiceTypes%5B%5D=NightLink&ServiceTypes%5B%5D=School&FareTypes%5B%5D=Standard&FareTypes%5B%5D=Prepaid&FareTypes%5B%5D=Free&journeyPlanIsValid=true");

    QUrl url("https://app.jp.translink.com.au/plan-your-journey/journey-planner");

    http->setHost(url.host(), QHttp::ConnectionModeHttps, url.port() == -1 ? 0 : url.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    QHttpRequestHeader header;
    header.setRequest("POST", url.path());
    header.setValue("Host", url.host());
    header.setValue("Content-Type", "application/x-www-form-urlencoded");
    header.setValue("Content-Length", QString::number(postData.length()));

    currentRequestId = http->request(header, postData, filebuffer);

    loop.exec();

    filebuffer->close();

    QJson::Parser parser;

    bool ok;

    ResultInfo result;
    QVariantMap map = parser.parse(filebuffer->buffer(),&ok).toMap();
    QVariantMap options = map["traveloptions"].toMap();
    QVariantMap enquiry = options["OriginalEnquiry"].toMap();
    result.fromStation = enquiry["Start"].toString();
    result.toStation = enquiry["End"].toString();
    QString sdate = options["searchDate"].toString();
    result.timeInfo = sdate.section(' ',0,0);
    QVariantList journeys = options["itineraries"].toList();
    foreach (QVariant j, journeys)
    {
        QVariantMap journey = j.toMap();
        ResultItem item;
        QStringList trains;
        QVariantList legs = journey["legs"].toList();
        int changes = 0;
        foreach (QVariant l, legs)
        {
            QVariantMap leg = l.toMap();
            if (leg["travelModeName"].toString() != "walk")
            {
                trains.append(leg["travelModeName"].toString());
                changes++;
            }
        }
        trains.removeDuplicates();
        item.trainType = trains.join(", ");
        int c = changes - 1;
        if (c < 0)
        {
            c = 0;
        }
        item.changes = QString::number(c);
        item.duration = journey["duration"].toString();
        QJson::Serializer serializer;
        bool ok;
        QByteArray jsonleg = serializer.serialize(journey, &ok);
        item.detailsUrl = jsonleg;
        result.items.append(item);
    }
    //QFile file("/home/user/post.txt");
    //file.open(QIODevice::WriteOnly);
    //file.write(filebuffer->buffer());
    //file.close();
    return result;
}

ResultInfo parserTranslink::getJourneyData(QString queryUrl)
{
    Q_UNUSED(queryUrl);
    ResultInfo result;
    return result;
}

DetailResultInfo parserTranslink::getJourneyDetailsData(QString queryUrl)
{
    QJson::Parser parser;

    bool ok;

    DetailResultInfo result;
    QVariantMap journey = parser.parse(queryUrl.toAscii(),&ok).toMap();
    result.duration = journey["duration"].toString();
    QVariantList legs = journey["legs"].toList();
    foreach (QVariant l, legs)
    {
        DetailResultItem item;
        QVariantMap leg = l.toMap();
        QString route = "";
        if (leg["travelModeName"].toString() == "walk")
        {
            route = "Walk ";
        }
        route.append(leg["routeLineLabel"].toString());
        item.train = route;
        item.fromStation = leg["origin"].toString();
        item.toStation = leg["destination"].toString();
        item.fromInfo = leg["start"].toString();
        item.toInfo = leg["end"].toString();
        QString duration = "Duration: ";
        duration.append(leg["duration"].toString());
        item.info = duration;
        result.items.append(item);
    }
    return result;
}
