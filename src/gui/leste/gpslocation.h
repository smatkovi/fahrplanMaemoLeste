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

#ifndef FAHRPLAN_GPSLOCATION_H
#define FAHRPLAN_GPSLOCATION_H

#include <QObject>
#include <QTimer>

// One position fix from the Maemo location framework (liblocation, which
// location-daemon and gpsd serve on Maemo Leste). It honours the positioning
// methods chosen in the Location settings of the control panel.
class GpsLocation : public QObject
{
    Q_OBJECT

public:
    explicit GpsLocation(QObject *parent = 0);
    ~GpsLocation();

    bool isActive() const;

public slots:
    // Starts looking for a position; emits exactly one of the signals below.
    void requestPosition(int timeoutMs = 90000);
    void cancel();

signals:
    void positionFound(qreal latitude, qreal longitude);
    void positionError(const QString &message);

private slots:
    void onTimeout();
    void stop();

private:
    static void onChanged(void *device, void *self);
    static void onError(void *control, void *self);
    void handleChanged(bool fromSignal);
    void finishFromSignal();

    void *m_control;
    void *m_device;
    unsigned long m_changedHandler;
    unsigned long m_errorHandler;
    bool m_active;
    QTimer m_timeout;
};

#endif // FAHRPLAN_GPSLOCATION_H
