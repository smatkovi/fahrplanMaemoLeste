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

#include "backupgpsopenstreetmap.h"

backupGpsOpenstreetmap::backupGpsOpenstreetmap()
{
    http = new QHttp(this);

    connect(http, SIGNAL(requestFinished(int,bool)),
            this, SLOT(httpRequestFinished(int,bool)));
}

QString backupGpsOpenstreetmap::getNearestStreet(qreal latitude, qreal longitude)
{
    QString sLongitude = QString::number(longitude).replace(QChar(','), QChar('.'));
    QString sLatitude  = QString::number(latitude).replace(QChar(','), QChar('.'));

    QString fullUrl = "http://nominatim.openstreetmap.org/reverse?lon=" +
                      sLongitude +
                      "&lat=" +
                      sLatitude +
                      "&format=xml";

    QUrl url(fullUrl);

    http->setHost(url.host(), QHttp::ConnectionModeHttp, url.port() == -1 ? 0 : url.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    currentRequestId = http->get(url.path() + "?" + url.encodedQuery(), filebuffer);

    loop.exec();

    //qDebug(filebuffer->buffer());

    filebuffer->close();

    QRegExp regexp = QRegExp("<road>(.*)</road>");
    regexp.setMinimal(true);

    QString str = QString::fromUtf8(filebuffer->buffer());

    regexp.indexIn(str);
    if (regexp.cap(1) != "")
    {
        return regexp.cap(1).trimmed();
    }
    return "";
}

QString backupGpsOpenstreetmap::getNearestCity(qreal latitude, qreal longitude)
{
    QString sLongitude = QString::number(longitude).replace(QChar(','), QChar('.'));
    QString sLatitude  = QString::number(latitude).replace(QChar(','), QChar('.'));

    QString fullUrl = "http://nominatim.openstreetmap.org/reverse?lon=" +
                      sLongitude +
                      "&lat=" +
                      sLatitude +
                      "&format=xml";

    QUrl url(fullUrl);

    http->setHost(url.host(), QHttp::ConnectionModeHttp, url.port() == -1 ? 0 : url.port());

    filebuffer = new QBuffer();

    if (!filebuffer->open(QIODevice::WriteOnly))
    {
        qDebug() << "Can't open Buffer";
    }

    currentRequestId = http->get(url.path() + "?" + url.encodedQuery(), filebuffer);

    loop.exec();

    //qDebug(filebuffer->buffer());

    filebuffer->close();

    QString str = QString::fromUtf8(filebuffer->buffer());

    QRegExp regexp = QRegExp("<city>(.*)</city>");
    regexp.setMinimal(true);
    regexp.indexIn(str);
    if (regexp.cap(1) != "")
    {
        return regexp.cap(1).trimmed();
    }

    regexp = QRegExp("<town>(.*)</town>");
    regexp.setMinimal(true);
    regexp.indexIn(str);
    if (regexp.cap(1) != "")
    {
        return regexp.cap(1).trimmed();
    }

    regexp = QRegExp("<village>(.*)</village>");
    regexp.setMinimal(true);
    regexp.indexIn(str);
    if (regexp.cap(1) != "")
    {
        return regexp.cap(1).trimmed();
    }
    return "";
}

void backupGpsOpenstreetmap::httpRequestFinished(int requestId, bool error)
{
    Q_UNUSED(error);
    if(currentRequestId != requestId) return;
    loop.exit();
}
