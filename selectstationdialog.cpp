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

#include "selectstationdialog.h"

selectStationDialog::selectStationDialog(QValueButton *sender, QGeoPositionInfo *position, parserAbstract *mainBackendParser, QSettings *mainSettings)
{
    setAttribute(Qt::WA_Maemo5AutoOrientation, true);
    connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(orientationChanged()));

    setWindowTitle(tr("City Stationname"));

    mainSender     = sender;
    myPositionInfo = position;
    backendParser  = mainBackendParser;
    settings       = mainSettings;

    QGtkStyle *gtkStyle = new QGtkStyle();

    searchboxCombo  = new QComboBox();
    searchboxText   = new QLineEdit();
    searchboxCombo->setEditable(true);
    //We using GTK style instead of the maemo one, because
    //i get design errors if i use maemo :D
    searchboxCombo->setStyle(gtkStyle);

    QStringList oldList;
    oldList = settings->value("lastsearch/stations", oldList).toStringList();

    //we reverse the list to get the latest stations on the top
    oldList = fahrplanUtils::reverseQStringList(oldList);

    for (int i = 0;i < oldList.size();i++)
    {
        QString str = oldList[i].section('@',0,0);
        oldList[i] = str;
    }

    searchboxCombo->addItems(oldList);
    searchboxCombo->setEditText("");

    gpsButton    = new QPushButton(tr("GPS"));
    searchButton = new QPushButton(tr("Search"));

    progressbar = new QDial();

    progressbar->setEnabled(false);
    progressbar->setWrapping(true);
    progressbar->hide();

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateProgress()));

    connect(searchButton, SIGNAL(clicked(bool)), this, SLOT(searchButtonClicked(bool)));
    connect(gpsButton, SIGNAL(clicked(bool)), this, SLOT(gpsButtonClicked(bool)));

    if (myPositionInfo->isValid())
    {
        gpsButton->setEnabled(backendParser->supportsGps());

        if (settings->value("openstreetmapfallback", true).toBool() == true)
        {
            gpsButton->setEnabled(true);
        }
    } else
    {
        gpsButton->setEnabled(false);
    }

    searchboxCombo->setFixedHeight(72);
    searchboxText->setFixedHeight(72);
    progressbar->setFixedWidth(72);

    QGridLayout *layout = new QGridLayout();

    setLayout(layout);
    orientationChanged();

    if (settings->value("storelastsearch", true).toBool() ==  true)
    {
        searchboxCombo->setFocus();
    } else
    {
        searchboxText->setFocus();
    }
}

void selectStationDialog::orientationChanged()
{
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    QGridLayout *layout = (QGridLayout*)this->layout();

    if (screenGeometry.width() > screenGeometry.height()) //landscape
    {
        gpsButton->setFixedWidth(210);
        searchButton->setFixedWidth(210);

        layout->addWidget(gpsButton, 0, 2);
        layout->addWidget(searchButton, 1, 2);
        if (settings->value("storelastsearch", true).toBool() ==  true)
        {
            layout->addWidget(searchboxCombo, 0, 0, 0, 1);
        } else
        {
            layout->addWidget(searchboxText, 0, 0, 0, 1);
        }
        layout->addWidget(progressbar, 0, 1, 0, 1);

    } else { //portrait
        gpsButton->setMinimumWidth(210);
        searchButton->setMinimumWidth(210);

        layout->addWidget(gpsButton, 1, 0);
        layout->addWidget(searchButton, 1, 1);
        if (settings->value("storelastsearch", true).toBool() ==  true)
        {
            layout->addWidget(searchboxCombo, 0, 0, 1, 0);
        } else
        {
            layout->addWidget(searchboxText, 0, 0, 1, 0);
        }
        layout->addWidget(progressbar, 0, 1);
    }
}



void selectStationDialog::updateProgress(){   
    progress++;
    qDebug() << progress;
    progressbar->setValue((progress*5) % 100);

    if(progress > 1500){ // 20ms * 50 * 30 = 30 sec
        backendParser->httpRequestTimeout();
        progress = 0;
    }
}


void selectStationDialog::showSelector(QStringList items)
{
    //If only one entry found we directly use it.
    if (items.count() == 1)
    {
        mainSender->setDescription(items[0]);
        this->hide();
        return;
    }

    stationsModel = new QStandardItemModel(0, 1);

    QString string;
    foreach(string, items)
    {
        //qDebug() << "- " << string;
        QStandardItem *item = new QStandardItem(string.section('@',0,0));
        item->setTextAlignment(Qt::AlignCenter);
        item->setEditable(false);
        item->setData(string.section('@',1,1));
        stationsModel->appendRow(item);
    }

    QMaemo5ListPickSelector *tmpSelector = new QMaemo5ListPickSelector;
    connect(tmpSelector, SIGNAL(selected(QString)), this, SLOT(stationSelected(QString)));
    tmpSelector->setModel(stationsModel);
    QPushButton *tmpButton = new QPushButton(tr("Select a station"));
    QWidget *w = tmpSelector->widget(tmpButton);
    w->show();
}



void selectStationDialog::stationSelected(QString value)
{
    if (value != "")
    {
        QString str = value;
        str.append("@");
        str.append(stationsModel->findItems(value)[0]->data().toString());
        mainSender->setDescription(str);
        this->hide();
    }
}



void selectStationDialog::searchButtonClicked(bool checked)
{
    Q_UNUSED(checked);

    searchButton->setEnabled(false);

    QStringList result;

    QString str = searchboxText->text();
    if (settings->value("storelastsearch", true).toBool() ==  true)
    {
        str = searchboxCombo->currentText();
    }

    progress=0;
    progressbar->show();
    timer->start(20); //start timer with 20ms interval

    result = backendParser->getStationsByName(str);

    progressbar->hide();
    timer->stop();

    searchButton->setEnabled(true);


    if (result.count() > 0)
    {
        showSelector(result);
    } else if(progress>250){
         QMaemo5InformationBox::information(this, "<br>No response from server.<br>", QMaemo5InformationBox::DefaultTimeout);
    } else
    {
        QMaemo5InformationBox::information(this, "<br>Station not found.<br>", QMaemo5InformationBox::DefaultTimeout);
    }
}

void selectStationDialog::gpsButtonClicked(bool checked)
{
    Q_UNUSED(checked);

    if (myPositionInfo->isValid())
    {
        gpsButton->setEnabled(false);

        // Get the current location as latitude and longitude
        QGeoCoordinate geoCoordinate = myPositionInfo->coordinate();
        qreal latitude = geoCoordinate.latitude();
        qreal longitude = geoCoordinate.longitude();

        QStringList result;

        progress=0;
        progressbar->show();
        timer->start(20); //start timer with 20ms interval

        result = backendParser->getStationsByGPS(latitude, longitude);

        progressbar->hide();
        timer->stop();

        gpsButton->setEnabled(true);

        if (result.count() > 0)
        {
            showSelector(result);
        } else
        {
            if (settings->value("openstreetmapfallback", true).toBool() == true)
            {
                backupGpsOpenstreetmap *fallbackGPS = new backupGpsOpenstreetmap();

                QString streetName = fallbackGPS->getNearestStreet(latitude, longitude);
                QString cityName   = fallbackGPS->getNearestCity(latitude, longitude);

                QStringList streetResult;
                QStringList cityResult;

                if (streetName != "")
                {
                    streetResult = backendParser->getStationsByName(streetName + ", " + cityName);
                }
                if (cityName != "")
                {
                    cityResult = backendParser->getStationsByName(cityName);
                }

                result.append(cityResult);
                result.append(streetResult);

                result.removeDuplicates();

                if (result.count() > 0)
                {
                    showSelector(result);
                } else
                {
                    QMaemo5InformationBox::information(this, "<br>No station found on GPS position.<br>", QMaemo5InformationBox::DefaultTimeout);
                }
            } else
            {
                QMaemo5InformationBox::information(this, "<br>No station found on GPS position.<br>", QMaemo5InformationBox::DefaultTimeout);
            }
        }
    }
}


void selectStationDialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    timer->stop(); //stop timer in case it is still running
}
