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

#include "calendardialog.h"

calendarDialog::calendarDialog(DetailResultInfo lastSearchDetails, QWidget *mainParent) :
    QDialog(parent)
{
    setAttribute(Qt::WA_Maemo5AutoOrientation, true);

    searchDetails = lastSearchDetails;
    parent        = mainParent;

    setWindowTitle(tr("Select target calendar"));

    QVBoxLayout *subLayout = new QVBoxLayout;
    subLayout->setMargin(1);
    subLayout->setStretch(0, 2);

    CMulticalendar *multiCalendar = CMulticalendar::MCInstance();
    vector<CCalendar*> vector_of_cal  = multiCalendar->getListCalFromMc();
    QVector<CCalendar*> qvector_of_cal = QVector< CCalendar  *>::fromStdVector(vector_of_cal);

    if(!qvector_of_cal.empty())
    {
        foreach(CCalendar *ccal, qvector_of_cal )
        {
            if (!ccal->IsReadOnly() && QString::fromStdString(ccal->getCalendarName()) != "cal_ti_smart_birthdays")
            {
                qDebug()<<QString::fromStdString(ccal->getCalendarName());
                QPushButton *itemButton = new QPushButton(handlePrivateCalTranslation(QString::fromStdString(ccal->getCalendarName())));
                itemButton->setFlat(true);

                CalendarUserData *data = new CalendarUserData();
                data->item = ccal;
                itemButton->setUserData(0, data);
                itemButton->setStyleSheet("QPushButton {text-align: left;}");
                itemButton->setIcon(createIcon(ccal->getCalendarColor()));
                connect(itemButton, SIGNAL(clicked()), this, SLOT(addToCalendarClicked()));
                subLayout->addWidget(itemButton);

                QFrame *line = new QFrame();
                line->setGeometry(QRect(20, 150, 381, 16));
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                line->setLineWidth(1);
                QPalette palette = line->palette();
                palette.setColor(QPalette::Dark, palette.color(QPalette::Mid));
                line->setPalette(palette);

                subLayout->addWidget(line);
            }
        }
    }

    QWidget *subWidget = new QWidget;
    subWidget->setFixedWidth(780);
    subWidget->setLayout(subLayout);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidget(subWidget);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(1);
    layout->setStretch(0, 2);
    layout->addWidget(scrollArea);

    setLayout(layout);
}


void calendarDialog::open()
{
    if (searchDetails.items.count() <= 0)
    {
        close();
        return;
    }

//    QDateTime tsFrom = searchDetails.items.first().fromTime;
    QDateTime tsTo = searchDetails.items.first().toTime;
//    qDebug() << 1 << tsTo.toTime_t() << QDateTime::currentDateTime().toTime_t();
//    if (tsFrom.toTime_t() < QDateTime::currentDateTime().toTime_t())
    if (tsTo.toTime_t() < QDateTime::currentDateTime().toTime_t()) //new: you can add a journey to the calendar if the end time is in the future, even the start time is in the past
    {
        QMaemo5InformationBox::information(parent, tr("<br>Entry is in the past!<br>"), QMaemo5InformationBox::DefaultTimeout);
        close();
        return;
    }
    show();
}

QIcon calendarDialog::createIcon(CalendarColour col)
{
    QPixmap pix_map = QPixmap(40,40);
    if(col == COLOUR_DARKBLUE)
    {
        pix_map.fill(QColor("#29a6ff"));
        return QIcon(pix_map);
    } else if( col == COLOUR_DARKGREEN)
    {
        pix_map.fill(QColor("#18cf39"));
        return QIcon(pix_map);
    } else if( col == COLOUR_DARKRED)
    {
        pix_map.fill(QColor("#ff599c"));
        return QIcon(pix_map);
    } else if( col == COLOUR_ORANGE)
    {
        pix_map.fill(QColor("#ff7d21"));
        return QIcon(pix_map);
    } else if( col == COLOUR_VIOLET)
    {
        pix_map.fill(QColor("#d679ff"));
        return QIcon(pix_map);
    } else if( col == COLOUR_YELLOW)
    {
        pix_map.fill(QColor("#fff329"));
        return QIcon(pix_map);
    } else if( col == COLOUR_WHITE)
    {
        pix_map.fill(QColor("#f7f3f7"));
        return QIcon(pix_map);
    } else if( col == COLOUR_BLUE)
    {
        pix_map.fill(QColor("#31ebf7"));
        return QIcon(pix_map);
    } else if( col == COLOUR_RED)
    {
        pix_map.fill(QColor("#ff96a5"));
        return QIcon(pix_map);
    } else if( col == COLOUR_GREEN)
    {
        pix_map.fill(QColor("#c6ff18"));
        return QIcon(pix_map);
    }
    pix_map.fill(QColor("#f7f3f7"));
    return QIcon(pix_map);
}

QString calendarDialog::handlePrivateCalTranslation(QString name)
{
    if (name == "cal_ti_calendar_private")
    {
        return tr("Private");
    }
    return name;
}


void calendarDialog::addToCalendarClicked()
{
    QPushButton *button = (QPushButton *)sender();
    CalendarUserData *data = (CalendarUserData*)button->userData(0);
    CCalendar *calendar = data->item;

    qDebug() << button;
    qDebug() << data;

    addToCalendar(calendar);

    close();
}



void calendarDialog::addToDefaultCalendar()
{
    qDebug() << "addToDefaultCalendar";

    CMulticalendar *multiCalendar = CMulticalendar::MCInstance();
    vector<CCalendar*> vector_of_cal  = multiCalendar->getListCalFromMc();
    QVector<CCalendar*> qvector_of_cal = QVector< CCalendar  *>::fromStdVector(vector_of_cal);

    if(!qvector_of_cal.empty())
    {
//        foreach(CCalendar *ccal, qvector_of_cal )
//        {
//            if (!ccal->IsReadOnly() && QString::fromStdString(ccal->getCalendarName()) != "cal_ti_smart_birthdays")
//                qDebug() << QString::fromStdString(ccal->getCalendarName());;
//                if(QString::fromStdString(ccal->getCalendarName()) == "Device main calendar"){
//                    addToCalendar(ccal);
//                }
//        }
        qDebug() << qvector_of_cal;
        CCalendar *ccal = qvector_of_cal[0];
        qDebug() << "Add to calendar:" << QString::fromStdString(ccal->getCalendarName());
        addToCalendar(ccal);
    }
}




void calendarDialog::addToCalendar(CCalendar *calendar)
{
    if (!calendar)
    {
        return;
    }

//    qDebug() << "calendar:" << calendar;

    QString title       = tr("Journey: ") + searchDetails.items.first().fromStation + tr(" to ") + searchDetails.items.last().toStation;
    QString description = searchDetails.items.first().fromStation + tr(" to ") + searchDetails.items.last().toStation + "\n";

    if (searchDetails.info != "")
    {
        description.append(searchDetails.info + "\n");
    }
    if (searchDetails.duration != "")
    {
        description.append(tr("Duration: ") + searchDetails.duration + "\n");
    }

    description.append("\n");

    foreach(DetailResultItem item, searchDetails.items)
    {
        if (item.train != "")
        {
            description.append(item.train + "\n");
        }

        description.append( item.fromStation );

        if (item.fromInfo != "")
        {
            description.append(" - " + item.fromInfo + "\n");
        } else
        {
            description.append("\n");
        }

        description.append( item.toStation );

        if (item.toInfo != "")
        {
            description.append(" - " + item.toInfo);
        }

        if (item.info != "")
        {
            description.append("\n" + item.info);
        }

        description.append("\n\n");
    }

    description.append(tr("\n\n(added by fahrplan app, please recheck informations before travel.)"));

    title       = fahrplanUtils::removeUmlauts(title);
    description = fahrplanUtils::removeUmlauts(description);

    QDateTime tsFrom = searchDetails.items.first().fromTime;
    QDateTime tsTo   = searchDetails.items.last().toTime;

 // if (tsFrom.toTime_t() < QDateTime::currentDateTime().toTime_t())
//    qDebug() << 2 << tsTo.toTime_t() << QDateTime::currentDateTime().toTime_t();
     if (tsTo.toTime_t() < QDateTime::currentDateTime().toTime_t()) //new: you can add a journey to the calendar if the end time is in the future, even the start time is in the past
    {
        QMaemo5InformationBox::information(parent, tr("<br>Entry is in the past!<br>"), QMaemo5InformationBox::DefaultTimeout);
    } else
    {

        /*
        qDebug() <<title;
        qDebug() <<tsFrom.toString("dd.MM.yyyy hh:mm");
        qDebug() <<tsTo.toString("dd.MM.yyyy hh:mm");
        qDebug() <<description;
        qDebug() <<calendar->getCalendarId();
        */

        CEvent *pEvent = new CEvent(title.toStdString(), description.toStdString(), "", tsFrom.toTime_t(), tsTo.toTime_t());
        int errorCode = 0;

        CMulticalendar *multiCalendar = CMulticalendar::MCInstance();
        multiCalendar->addEvent(pEvent, calendar->getCalendarId(), errorCode);
        if (errorCode == CALENDAR_OPERATION_SUCCESSFUL)
        {
            QMaemo5InformationBox::information(parent, tr("<br>Entry added to the ") + handlePrivateCalTranslation(QString::fromStdString(calendar->getCalendarName())) + tr(" calendar.<br>"), QMaemo5InformationBox::DefaultTimeout);
        } else if (errorCode == CALENDAR_ENTRY_DUPLICATED){
            QMaemo5InformationBox::information(parent, tr("<br>Entry already exists!<br>"), QMaemo5InformationBox::DefaultTimeout);
        } else {
            QMaemo5InformationBox::information(parent, tr("<br>An error occured while adding the entry!<br>"), QMaemo5InformationBox::DefaultTimeout);
        }
    }

//    close();
}
