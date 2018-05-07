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

#ifndef CALENDARDIALOG_H
#define CALENDARDIALOG_H

#include <QDialog>
#include <QtGui>
#include <QtMaemo5>
#include <calendar-backend/CCalendar.h>
#include <calendar-backend/CMulticalendar.h>
#include <calendar-backend/CEvent.h>
#include <calendar-backend/CAlarm.h>
#include <calendar-backend/CalendarErrors.h>

#include "parser_abstract.h"
#include "fahrplanutils.h"

class CalendarUserData : public QObjectUserData
{
public:
    CCalendar *item;
};

class calendarDialog : public QDialog
{
    Q_OBJECT
public:
    explicit calendarDialog(DetailResultInfo lastSearchDetails, QWidget *mainParent = 0);
    void open();

private:
    QIcon createIcon(CalendarColour col);
    QString handlePrivateCalTranslation(QString name);
    QWidget *parent;
    DetailResultInfo searchDetails;

    void addToCalendar(CCalendar *calendar);

public:
    void addToDefaultCalendar();

private slots:
    void addToCalendarClicked();

signals:

public slots:

};

#endif // CALENDARDIALOG_H
