/****************************************************************************
**
**  This file is a part of Fahrplan.
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License along
**  with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

// glib before Qt: Qt's "signals" keyword would clash with glib identifiers.
// (The liblocation headers declare C linkage themselves.)
#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>

#include "gpslocation.h"

#include <QCoreApplication>
#include <QDebug>

GpsLocation::GpsLocation(QObject *parent)
    : QObject(parent)
    , m_control(0)
    , m_device(0)
    , m_changedHandler(0)
    , m_errorHandler(0)
    , m_active(false)
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout, SIGNAL(timeout()), SLOT(onTimeout()));
}

GpsLocation::~GpsLocation()
{
    stop();
}

bool GpsLocation::isActive() const
{
    return m_active;
}

void GpsLocation::requestPosition(int timeoutMs)
{
    if (m_active)
        return;
    // A previous request may still wait for its teardown.
    if (m_control || m_device)
        stop();

    LocationGPSDControl *control = location_gpsd_control_get_default();
    if (!control) {
        emit positionError(QCoreApplication::translate("StationSelectPage", "GPS is not available"));
        return;
    }
    g_object_set(G_OBJECT(control),
                 "preferred-method", LOCATION_METHOD_USER_SELECTED,
                 "preferred-interval", LOCATION_INTERVAL_DEFAULT,
                 NULL);

    LocationGPSDevice *device = LOCATION_GPS_DEVICE(g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL));

    m_control = control;
    m_device = device;
    m_active = true;

    m_errorHandler = g_signal_connect(control, "error", G_CALLBACK(&GpsLocation::onError), this);
    m_changedHandler = g_signal_connect(device, "changed", G_CALLBACK(&GpsLocation::onChanged), this);

    location_gpsd_control_start(control);
    m_timeout.start(timeoutMs);

    // A recent fix may already be there.
    handleChanged(false);
}

void GpsLocation::cancel()
{
    stop();
}

void GpsLocation::stop()
{
    m_timeout.stop();
    if (m_device) {
        if (m_changedHandler)
            g_signal_handler_disconnect(m_device, m_changedHandler);
        g_object_unref(m_device);
    }
    if (m_control) {
        if (m_errorHandler)
            g_signal_handler_disconnect(m_control, m_errorHandler);
        location_gpsd_control_stop(LOCATION_GPSD_CONTROL(m_control));
        g_object_unref(m_control);
    }
    m_device = 0;
    m_control = 0;
    m_changedHandler = 0;
    m_errorHandler = 0;
    m_active = false;
}

void GpsLocation::onChanged(void *, void *self)
{
    static_cast<GpsLocation *>(self)->handleChanged(true);
}

void GpsLocation::onError(void *, void *self)
{
    GpsLocation *that = static_cast<GpsLocation *>(self);
    if (!that->m_active)
        return;
    that->finishFromSignal();
    emit that->positionError(QCoreApplication::translate("StationSelectPage", "GPS is not available"));
}

// Inside a GObject signal handler the objects must not be released yet;
// mark the request as done and tear down from the event loop.
void GpsLocation::finishFromSignal()
{
    m_active = false;
    m_timeout.stop();
    QMetaObject::invokeMethod(this, "stop", Qt::QueuedConnection);
}

void GpsLocation::handleChanged(bool fromSignal)
{
    LocationGPSDevice *device = static_cast<LocationGPSDevice *>(m_device);
    if (!m_active || !device || !device->fix)
        return;
    if (!(device->fix->fields & LOCATION_GPS_DEVICE_LATLONG_SET))
        return;

    const qreal latitude = device->fix->latitude;
    const qreal longitude = device->fix->longitude;
    qDebug() << "GPS fix" << latitude << longitude;
    if (fromSignal)
        finishFromSignal();
    else
        stop();
    emit positionFound(latitude, longitude);
}

void GpsLocation::onTimeout()
{
    stop();
    emit positionError(QCoreApplication::translate("StationSelectPage", "No GPS fix, please try again outdoors"));
}
