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

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), resultWindow(0), detailsWindow(0)
{
    setAttribute(Qt::WA_Maemo5AutoOrientation, true);
    connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(orientationChanged()));

    //About Menu Button
    QAction *aboutAct = new QAction(tr("About"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(aboutButtonClicked()));
    menuBar()->addAction(aboutAct);

    //Settings Menu Button
    QAction *settingsAct = new QAction(tr("Settings"), this);
    connect(settingsAct, SIGNAL(triggered()), this, SLOT(settingsButtonClicked()));
    menuBar()->addAction(settingsAct);

    //Toggle GPS Button
    gpsAct = new QAction(tr("Enable GPS"), this);
    connect(gpsAct, SIGNAL(triggered()), this, SLOT(gpsButtonClicked()));
    menuBar()->addAction(gpsAct);

    //Toggle GPS Button
    QAction *donateAct = new QAction(tr("Donate"), this);
    connect(donateAct, SIGNAL(triggered()), this, SLOT(donateButtonClicked()));
    menuBar()->addAction(donateAct);

    setWindowTitle("Fahrplan");

    QWidget *mainWidget = new QWidget;
    setCentralWidget(mainWidget);

    QGridLayout *layout = new QGridLayout();
    mainWidget->setLayout(layout);

    //From and To and Swap Buttons
    QGridLayout *stationsLayout = new QGridLayout();
    QWidget *stationsContainerWidget = new QWidget;

    stationDeparture = new QValueButton(tr("Station from"), tr("please select"));
    stationArrival   = new QValueButton(tr("Station to"), tr("please select"));

    stationSwap      = new QPushButton(QChar(8597));

    connect(stationDeparture, SIGNAL(clicked(bool)), this, SLOT(stationDepartureClicked()));
    connect(stationArrival, SIGNAL(clicked(bool)), this, SLOT(stationArrivalClicked()));
    connect(stationSwap, SIGNAL(clicked(bool)), this, SLOT(swapClicked()));

    stationsLayout->setMargin(0);
    stationsLayout->setSpacing(10);
    stationsLayout->addWidget(stationDeparture, 0, 0);
    stationsLayout->addWidget(stationArrival, 1, 0);
    stationsLayout->addWidget(stationSwap, 0, 1, 0, 1);
    stationsContainerWidget->setLayout(stationsLayout);

    layout->addWidget(stationsContainerWidget, 0, 0, 1, 0);

    //Date and Time Buttons
    dateSelector  = new QMaemo5ValueButton(tr("Date"));
    dateSelector->setPickSelector(new QMaemo5DatePickSelector());
    dateSelector->setValueLayout(QMaemo5ValueButton::ValueUnderTextCentered);

    timeSelector  = new QMaemo5ValueButton(tr("Time"));
    timeSelector->setPickSelector(new QMaemo5TimePickSelector());
    timeSelector->setValueLayout(QMaemo5ValueButton::ValueUnderTextCentered);

    //Departure/Arrival Selector Button
    modeButton = new QValueButton(tr("Use selected date/time as"), tr("Departure time"));
    ModeButtonUserData *mode = new ModeButtonUserData();
    mode->mode = 1;
    modeButton->setUserData(0, mode);
    connect(modeButton, SIGNAL(clicked(bool)), this, SLOT(modeClicked()));

    //Restrict Train Types
    traintypesButton = new QValueButton(tr("Restrict train types"), "");

    //Search Button
    searchButton = new QPushButton();
    searchButton->setText(tr("Search"));

    connect(searchButton, SIGNAL(clicked(bool)), this, SLOT(searchButtonClicked(bool)));

    progressbar = new QDial();

    progressbar->setEnabled(false);
    progressbar->setWrapping(true);
    progressbar->hide();

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateProgress()));



    //Add all to Layout
    #ifdef Q_WS_MAEMO_5
        setAttribute(Qt::WA_Maemo5StackedWindow);
    #endif

    //Load Settings
    settings = new QSettings("smurfy", "fahrplan");

    //Init GPS if in settings
    locationDataSource = QGeoPositionInfoSource::createDefaultSource(this);
    locationDataSource->setPreferredPositioningMethods(locationDataSource->AllPositioningMethods);

    QObject::connect(locationDataSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(positionUpdated(QGeoPositionInfo)));

    if (settings->value("autostartGPS", true).toBool() ==  true)
    {
        locationDataSource->startUpdates();
        locationDataSourceIsUpdating = true;
        gpsAct->setText(tr("Disable GPS"));
    } else
    {
        locationDataSourceIsUpdating = false;
        gpsAct->setText(tr("Enable GPS"));
    }

    //Prefill stations
    if ((settings->value("prefillsearchbox", true).toBool() ==  true) &&
        (settings->value("storelastsearch", true).toBool() ==  true)
        )
    {
        QStringList stationsList;
        stationsList = settings->value("lastsearch/stations", stationsList).toStringList();
        //we reverse the list to get the latest stations on the top
        stationsList = fahrplanUtils::reverseQStringList(stationsList);

        if (stationsList.count() > 1)
        {
            stationDeparture->setDescription(stationsList[1]);
            stationArrival->setDescription(stationsList[0]);
        }
    }

    initBackend();
    orientationChanged();

    //Show first launch msg
    if (settings->value("firstLaunch", "") != APP_VER && APP_FIRSTLAUNCH)
    {
        QMessageBox::about(this,
                           tr("Welcome to a new version of fahrplan this has changed"),
                           tr(""));

        settings->setValue("firstLaunch", APP_VER);
    }
}

void MainWindow::orientationChanged()
{
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    QGridLayout *layout = (QGridLayout*)this->centralWidget()->layout();


    if (screenGeometry.width() > screenGeometry.height()) //landscape
    {
        layout->addWidget(dateSelector, 1, 0);
        layout->addWidget(timeSelector, 1, 1);
        layout->addWidget(modeButton, 2, 0);
        layout->addWidget(traintypesButton, 2, 1);
        layout->addWidget(searchButton, 3, 0, 1, 0);

        layout->addWidget(progressbar, 0, 0);

        progressbar->setFixedWidth(800);
        progressbar->setFixedHeight(350);

        dateSelector->setFixedHeight(78);
        timeSelector->setFixedHeight(78);
        modeButton->setFixedHeight(78);
        traintypesButton->setFixedHeight(78);
        stationDeparture->setFixedHeight(78);
        stationArrival->setFixedHeight(78);
        stationSwap->setFixedHeight(160);
        stationSwap->setFixedWidth(100);
        searchButton->setFixedHeight(78);

    } else { //portrait
        layout->addWidget(dateSelector, 1, 0, 1, 0);
        layout->addWidget(timeSelector, 2, 0, 1, 0);
        layout->addWidget(modeButton, 3, 0, 1, 0);
        layout->addWidget(traintypesButton, 4, 0, 1, 0);
        layout->addWidget(searchButton, 5, 0, 1, 0);

        layout->addWidget(progressbar, 0, 0);

        progressbar->setFixedWidth(440);
        progressbar->setFixedHeight(650);

        dateSelector->setFixedHeight(98);
        timeSelector->setFixedHeight(98);
        modeButton->setFixedHeight(98);
        traintypesButton->setFixedHeight(98);
        stationDeparture->setFixedHeight(98);
        stationArrival->setFixedHeight(98);
        stationSwap->setFixedHeight(200);
        stationSwap->setFixedWidth(100);
        searchButton->setFixedHeight(98);
    }
}




void MainWindow::updateProgress(){
    static int progress = 0;
    progress++;
    //qDebug() << progress;
    if (!resultWindow){
        progressbar->setValue((progress*5) % 100);
    }
    else{
        progressbarResult->setValue((progress*5) % 100);
    }

    if(progress > 1500){ // 20ms * 50 * 30 = 30 sec
        backendParser->httpRequestTimeout();
        timeout = true;
        progress = 0;
    }
}



void MainWindow::initBackend(bool resetStations)
{
    QString backendType = settings->value("backend", "parserMobileBahnDe").toString();



    //Init Parser Backend
    if (backendType == "parserMobileBahnDe")
    {
        backendParser = new parserMobileBahnDe();
        setWindowTitle("Fahrplan (DB)");
    } else if (backendType == "parserMvvDe")
    {
        backendParser = new parserMvvDe();
        setWindowTitle("Fahrplan (MVV)");
    }/* else if (backendType == "parserSbbCh")  //this backend is currently broken. 10. June 2011, CaCO3
    {
        backendParser = new parserSbbCh();
        setWindowTitle("Fahrplan (SBB-Legacy)");
    }*/ else if (backendType == "parser131500ComAu")
    {
        backendParser = new parser131500ComAu();
        setWindowTitle("Fahrplan (NSW)");
    } else if (backendType == "parserXmlOebbAt")
    {
        backendParser = new parserXmlOebbAt();
        setWindowTitle("Fahrplan (ÖBB)");
    } else if (backendType == "parserXmlSbbCh")
    {
        backendParser = new parserXmlSbbCh();
        setWindowTitle("Fahrplan (SBB)");
    } else if (backendType == "parserXmlRejseplanenDk")
    {
        backendParser = new parserXmlRejseplanenDk();
        setWindowTitle("Fahrplan (Rejseplanen)");
    } else if (backendType == "parserHafasXml")
    {
        backendParser = new parserHafasXml();
        setWindowTitle("Fahrplan (hafasXml - alpha)");
    } else if (backendType == "parserTranslink")
    {
        backendParser = new parserTranslink();
        setWindowTitle("Fahrplan Translink");
    } else
    {
       //fallback is bahn.de
       backendParser = new parserMobileBahnDe();
       setWindowTitle("Fahrplan (bahn.de)");
    }

    if (resetStations) {
        stationArrival->setDescription(tr("please select"));
        stationDeparture->setDescription(tr("please select"));
    }

    //Update gui based on Backend
    QStringList trainRestrictions = backendParser->getTrainRestrictions();
    traintypesButton->setDescription(tr("not supported"));
    traintypesButton->setEnabled(false);
    if (trainRestrictions.count() > 0)
    {
        QStandardItemModel * trainrestrModel = new QStandardItemModel(0, 1);
        for (int i = 0; i < trainRestrictions.count(); i++)
        {
            QStandardItem *item = new QStandardItem(trainRestrictions[i]);
            item->setTextAlignment(Qt::AlignCenter);
            item->setEditable(false);
            trainrestrModel->appendRow(item);
        }
        traintypesButton->setModel(trainrestrModel);
        traintypesButton->setModelIndex(0);
        traintypesButton->setEnabled(true);
    }
}

void MainWindow::positionUpdated(QGeoPositionInfo geoPositionInfo)
{
    if (!myPositionInfo.isValid() && geoPositionInfo.isValid())
    {
        QMaemo5InformationBox::information(this, tr("<br>GPS is now available!<br>"), QMaemo5InformationBox::DefaultTimeout);
    }
    myPositionInfo = geoPositionInfo;
}

void MainWindow::gpsButtonClicked()
{
    if (locationDataSourceIsUpdating)
    {
        locationDataSource->stopUpdates();
        locationDataSourceIsUpdating = false;
        gpsAct->setText(tr("Enable GPS"));
        QMaemo5InformationBox::information(this, tr("<br>GPS is now <b>disabled</b><br>"), QMaemo5InformationBox::DefaultTimeout);
    } else
    {
        locationDataSource->startUpdates();
        locationDataSourceIsUpdating = true;
        gpsAct->setText(tr("Disable GPS"));
        QMaemo5InformationBox::information(this, tr("<br>GPS is now <b>enabled</b><br>"), QMaemo5InformationBox::DefaultTimeout);
    }
}

void MainWindow::swapClicked()
{
    QString tmp = stationDeparture->description();
    stationDeparture->setDescription(stationArrival->description());
    stationArrival->setDescription(tmp);
}

void MainWindow::modeClicked()
{
    QValueButton *button = (QValueButton *)sender();
    ModeButtonUserData *mode = (ModeButtonUserData*)button->userData(0);
    if (mode->mode == 0)
    {
        mode->mode = 1;
        modeButton->setDescription(tr("Departure time"));

    } else {
        mode->mode = 0;
        modeButton->setDescription(tr("Arrival time"));
    }
    button->setUserData(0,mode);
}

void MainWindow::stationDepartureClicked()
{
    QPointer<selectStationDialog> selectDialog = new selectStationDialog(stationDeparture, &myPositionInfo, backendParser, settings);
    selectDialog->show();
}

void MainWindow::stationArrivalClicked()
{
    QPointer<selectStationDialog> selectDialog = new selectStationDialog(stationArrival, &myPositionInfo, backendParser, settings);
    selectDialog->show();
}

void MainWindow::alarmButtonClicked()
{
    calendarDialog *calDlg = new calendarDialog(lastSearchDetails, this);
    calDlg->open();
}


void MainWindow::alarmDefaultButtonClicked()
{
    calendarDialog *calDlg = new calendarDialog(lastSearchDetails, this);
    calDlg->addToDefaultCalendar();
}

void MainWindow::donateButtonClicked()
{
    QDesktopServices::openUrl(QUrl("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=DW3LGJ6VEKFVS"));
    QMessageBox::about(this,
                       tr("Donate"),
                       tr("Thank you for Donating!"));
}

void MainWindow::aboutButtonClicked()
{
    QString version = APP_VER;
    QString aboutMsg = tr("by smurfy (maemo@smurfy.de)<br>and CaCO3 (george@ruinelli.ch)<br>Version ") + version + tr("<br><br>Current supported databackends:<br><b>DB</b>, <b>MVV</b>, <b>SBB</b>, <b>ÖBB</b>, <b>Rejseplanen.dk</b>, <b>131500.com.au</b>, <b>Translink QLD</b><br><br>This or prior versions uses code contribution by:<br><i>hcm</i>, <i>thp</i>, <i>qwerty12</i>, <i>qbast</i>, <i>dehapama</i>, <i>halftux</i>, <i>jonwil</i>");
    QMessageBox::about(this,
                       tr("About Fahrplan"),
                       tr(aboutMsg.toAscii()));

}

void MainWindow::settingsButtonClicked()
{
    configDialog *cfgDlg = new configDialog(settings, this);
    cfgDlg->show();
}

void MainWindow::resultItemClicked()
{
    QPushButton *button = (QPushButton *)sender();
    ResultItemUserData *data = (ResultItemUserData*)button->userData(0);

    qDebug() << "resultItemClicked";

    if (detailsLoading != true)
    {
        detailsLoading = true;


        timeout=false;
        progressbarResult->show();
        timer->start(20); //start timer with 20ms interval
qDebug() << data->item.detailsUrl;
        lastSearchDetails = backendParser->getJourneyDetailsData(data->item.detailsUrl);

        progressbarResult->hide();
//        scrollAreaResult->show();
        timer->stop();

        detailsLoading = false;

        showDetailsWindow();
    } else
    {
        QMaemo5InformationBox::information(this, tr("<br>Details already loading, please wait.<br>"), QMaemo5InformationBox::DefaultTimeout);
        return;
    }
}

void MainWindow::showDetailsWindow()
{
    if (detailsWindow)
    {
        detailsWindow->hide();
    }


    if(timeout){
        QMaemo5InformationBox::information(this, "<br>No response from server.<br>", QMaemo5InformationBox::DefaultTimeout);
        return;
    }
    else  if (lastSearchDetails.items.count() == 0)
    {
        QMaemo5InformationBox::information(this, tr("<br>Error opening the details.<br>") + lastSearchResult.errorMsg, QMaemo5InformationBox::DefaultTimeout);
        return;
    }

    //Details Window
    detailsWindow = new QMainWindow(resultWindow);
    detailsWindow->setWindowTitle(tr("Details"));
    #ifdef Q_WS_MAEMO_5
        detailsWindow->setAttribute(Qt::WA_Maemo5StackedWindow);
    #endif
    detailsWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(detailsWindow, SIGNAL(destroyed(QObject*)),
            this, SLOT(windowDestroyed(QObject*)));

    //Alarm Menu Button
    QAction *alarmActDefault = new QAction(tr("Add to default calendar"), this);
    connect(alarmActDefault, SIGNAL(triggered()), this, SLOT(alarmDefaultButtonClicked()));
    detailsWindow->menuBar()->addAction(alarmActDefault);


    QAction *alarmAct = new QAction(tr("Add to a calendar"), this);
    connect(alarmAct, SIGNAL(triggered()), this, SLOT(alarmButtonClicked()));
    detailsWindow->menuBar()->addAction(alarmAct);

    QWidget *itemsWidget = new QWidget;
    QVBoxLayout *itemLayout = new QVBoxLayout;
    itemLayout->setMargin(10);
    QScrollArea *scrollArea = new QScrollArea;


    qDebug() <<"Result:";
    qDebug() <<lastSearchDetails.duration;
    qDebug() <<lastSearchDetails.info;


    foreach(DetailResultItem item, lastSearchDetails.items)
    {
        QString buttonText = "";

        if (item.train != "")
        {
            buttonText.append("<b>" + item.train + "</b><br>");
        }

        buttonText.append( item.fromStation );

        if (item.fromInfo != "")
        {
            buttonText.append(" - " + item.fromInfo + "<br>");
        } else
        {
            buttonText.append("<br>");
        }

        buttonText.append( item.toStation );

        if (item.toInfo != "")
        {
            buttonText.append(" - " + item.toInfo);
        }

        if (item.info != "")
        {
            buttonText.append("<br><i>" + item.info + "</i>");
        }

        QLabel *itemButtonLabel = new QLabel(buttonText);
        itemButtonLabel->setFixedWidth(770);
        itemLayout->addWidget(itemButtonLabel);

        QFrame *line = new QFrame();
        line->setGeometry(QRect(20, 150, 381, 16));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        QPalette palette = line->palette();
        palette.setColor(QPalette::Dark, palette.color(QPalette::Mid));
        line->setPalette(palette);

        itemLayout->addWidget(line);
/*
        qDebug() <<"Item:";
        qDebug() <<item.train;
        qDebug() <<item.fromStation;
        qDebug() <<item.toStation;
        qDebug() <<item.toInfo;
        qDebug() <<item.fromInfo;
        qDebug() <<item.info;
*/
    }

    //Creating result data
    //Main Widget
    QWidget *mainWidget = new QWidget;
    detailsWindow->setCentralWidget(mainWidget);

    //Header
    QString headerText = "<center><b>" + lastSearchDetails.items[0].fromStation + "</b>" + tr(" to ") +
                         "<b>" + lastSearchDetails.items[lastSearchDetails.items.count() - 1].toStation + "</b>";

    if (lastSearchDetails.duration != "")
    {
        headerText.append(tr("<br><b>Duration: </b>") + lastSearchDetails.duration);
    }

    if (lastSearchDetails.info != "")
    {
        headerText.append("<br><i>" + lastSearchDetails.info + "</i>");
    }

    headerText.append("</center>");

    QLabel *headerLabel = new QLabel(headerText);
    itemsWidget->setLayout(itemLayout);

    scrollArea->setWidget(itemsWidget);

    //Main Layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(1);
    layout->addWidget(headerLabel);
    layout->addWidget(scrollArea);

    mainWidget->setLayout(layout);

    detailsWindow->show();
}

void MainWindow::showResultWindow()
{
    if (resultWindow)
    {
        resultWindow->hide();
    }

    if(timeout){
        QMaemo5InformationBox::information(this, "<br>No response from server.<br>", QMaemo5InformationBox::DefaultTimeout);
        searchButton->setEnabled(true);
        return;
    }
    else if (lastSearchResult.items.count() == 0)
    {
        QMaemo5InformationBox::information(this, tr("<br>No results found.<br>") + lastSearchResult.errorMsg + "<br>", QMaemo5InformationBox::DefaultTimeout);
        searchButton->setEnabled(true);
        return;
    }

    detailsLoading = false;

    /*
    qDebug() <<"Results:";
    qDebug() <<lastSearchResult.fromStation;
    qDebug() <<lastSearchResult.toStation;
    qDebug() <<lastSearchResult.timeInfo;
    qDebug() <<lastSearchResult.laterUrl;
    qDebug() <<lastSearchResult.earlierUrl;
    qDebug() <<lastSearchResult.items.count();
    */

    //Result Window
    resultWindow = new QMainWindow(this);
    resultWindow->setWindowTitle(tr("Results"));
    #ifdef Q_WS_MAEMO_5
        resultWindow->setAttribute(Qt::WA_Maemo5StackedWindow);
    #endif
    resultWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(resultWindow, SIGNAL(destroyed(QObject*)),
            this, SLOT(windowDestroyed(QObject*)));


    //Creating result data
    //Main Widget
    QWidget *mainWidget = new QWidget;
    resultWindow->setCentralWidget(mainWidget);

    progressbarResult = new QDial();

    progressbarResult->setEnabled(false);
    progressbarResult->setWrapping(true);
    progressbarResult->hide();


    //Header
    QLabel *headerLabel = new QLabel("<center><b>" + lastSearchResult.fromStation +
                                     "</b> " +
                                     tr("to") + " <b>" +
                                     lastSearchResult.toStation +
                                     "</b><br>" +
                                     "<b>" +
                                     lastSearchResult.timeInfo +
                                     "</center>"
                                     );

    //Later and earlier Menu Button
    if (lastSearchResult.earlierUrl != "")
    {
        QAction *earlierAct = new QAction(tr("Earlier"), this);
        connect(earlierAct, SIGNAL(triggered()), this, SLOT(earlierButtonClicked()));
        resultWindow->menuBar()->addAction(earlierAct);
    }
    if (lastSearchResult.laterUrl != "")
    {
        QAction *laterAct = new QAction(tr("Later"), this);
        connect(laterAct, SIGNAL(triggered()), this, SLOT(laterButtonClicked()));
        resultWindow->menuBar()->addAction(laterAct);
    }

    QWidget *itemsWidget = new QWidget;
    QVBoxLayout *itemLayout = new QVBoxLayout;
    itemLayout->setMargin(10);
    QScrollArea *scrollAreaResult = new QScrollArea;

    ResultItem item;
    foreach(item, lastSearchResult.items)
    {
        QString changes = item.changes + tr(" transfer");
        if (item.changes != "1")
        {
            changes = item.changes + tr(" transfers");
        }
        QPushButton *itemButton = new QPushButton();
        itemButton->setFlat(true);
        QString buttonText = "";

        if (item.fromTime != "")
        {
            //correct result displaying
//            qDebug() << item.fromTime << item.toTime;
            QStringList list = item.toTime.split(" ");
//            qDebug() << list.size();
            if(list.size()>1){ //arrival is not on same day
                item.toTime = list[1];
            }
//            qDebug() << item.fromTime << item.toTime;

            buttonText.append("<b>" + item.fromTime + " - " +
                              item.toTime + "</b> ");
        }

        //correct result displaying
        item.trainType.replace(" ", "");
        item.trainType.replace(",", ", ");

        buttonText.append(item.trainType + " (" +
                          changes + ") - " +
                          item.duration);

        itemButton->setFixedHeight(72);
        if (item.state == "+0")
        {
            buttonText.append(tr("<br>OnTime"));
            itemButton->setFixedHeight(100);
        } else if (item.state != "")
        {
            buttonText.append("<br>" + item.state);
            itemButton->setFixedHeight(100);
        }

        QVBoxLayout *itemButtonLayout = new QVBoxLayout;
        itemButton->setFixedWidth(770);
        QGtkStyle *gtkStyle = new QGtkStyle();
        itemButton->setStyle(gtkStyle);

        ResultItemUserData *data = new ResultItemUserData();
        data->item = item;
        itemButton->setUserData(0, data);

        QLabel *itemButtonLabel = new QLabel(buttonText, itemButton);
        itemButtonLabel->installEventFilter(this);
        itemButtonLayout->addWidget(itemButtonLabel);

        itemButtonLayout->setMargin(10);
        itemButtonLayout->setSpacing(5);
        itemButton->setLayout(itemButtonLayout);
        itemLayout->addWidget(itemButton);

        connect(itemButton, SIGNAL(clicked()), this, SLOT(resultItemClicked()));

        QFrame *line = new QFrame();
        line->setGeometry(QRect(20, 150, 381, 16));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        QPalette palette = line->palette();
        palette.setColor(QPalette::Dark, palette.color(QPalette::Mid));
        line->setPalette(palette);

        itemLayout->addWidget(line);
        /*
        qDebug() <<"Item:";
        qDebug() <<item.fromTime;
        qDebug() <<item.toTime;
        qDebug() <<item.trainType;
        qDebug() <<item.duration;
        qDebug() <<item.state;
        qDebug() <<item.changes;
        */

    }

    itemsWidget->setLayout(itemLayout);

    scrollAreaResult->setWidget(itemsWidget);

    //Main Layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(1);
    layout->addWidget(headerLabel);
    layout->addWidget(scrollAreaResult);
    layout->addWidget(progressbarResult);


    progressbarResult->setFixedHeight(100);

    mainWidget->setLayout(layout);



    resultWindow->show();
    //qDebug() << "Window" << resultWindow;
}

bool MainWindow::eventFilter(QObject* pObject, QEvent* pEvent)
{
    if ((pEvent->type() == QEvent::MouseButtonPress) || (pEvent->type() == QEvent::MouseButtonRelease))
    {
        pEvent->ignore();
        return false;
    } else
    {
        return QObject::eventFilter(pObject, pEvent);
    }
}

void MainWindow::earlierButtonClicked()
{
    lastSearchResult = backendParser->getJourneyData(lastSearchResult.earlierUrl);
    showResultWindow();
}

void MainWindow::laterButtonClicked()
{
    lastSearchResult = backendParser->getJourneyData(lastSearchResult.laterUrl);
    showResultWindow();
}




void MainWindow::searchButtonClicked(bool checked)
{
    Q_UNUSED(checked);

    if (!resultWindow) {

        QMaemo5TimePickSelector *timeSel = (QMaemo5TimePickSelector *)timeSelector->pickSelector();
        QMaemo5DatePickSelector *dateSel = (QMaemo5DatePickSelector *)dateSelector->pickSelector();
        ModeButtonUserData *mode = (ModeButtonUserData*)modeButton->userData(0);

        int trainrestrictions = 0;
        if (backendParser->getTrainRestrictions().count() > 0)
        {
            trainrestrictions = traintypesButton->modelIndex();
        }

        if (stationDeparture->description() != stationArrival->description() && stationDeparture->description() != tr("please select") && stationArrival->description() != tr("please select"))
        {
            //Store last search data
            if (settings->value("storelastsearch", true).toBool() ==  true)
            {
                QStringList oldList;
                oldList = settings->value("lastsearch/stations", oldList).toStringList();
                oldList.append(stationDeparture->description().trimmed());
                oldList.append(stationArrival->description().trimmed());

                //We need to reverse the order before removing duplicates
                //to ensure the latest station is later on the bottom
                oldList = fahrplanUtils::reverseQStringList(oldList);
                oldList.removeDuplicates();
                oldList = fahrplanUtils::reverseQStringList(oldList);

                //We will store only the last active stations used
                while (oldList.count() > 20)
                {
                    oldList.removeFirst();
                }

                settings->setValue("lastsearch/stations", oldList);
            }

            searchButton->setEnabled(false);

            timeout=false;
            progressbar->show();
            timer->start(20); //start timer with 20ms interval

            lastSearchResult = backendParser->getJourneyData(stationDeparture->description(), stationArrival->description(), "", dateSel->currentDate(), timeSel->currentTime(), mode->mode, trainrestrictions);

            progressbar->hide();
            timer->stop();

            showResultWindow();
        } else
        {
            QMaemo5InformationBox::information(this, tr("<br>Please select a from and to station and make sure they are not the same.<br>") + lastSearchResult.errorMsg, QMaemo5InformationBox::DefaultTimeout);
        }
    }
}






void MainWindow::closeEvent(QCloseEvent *event)
{
    //qDebug() <<"close event";
    hide();
    event->accept();
}

int MainWindow::top_application()
{
    //qDebug() <<"top event";
    show();
    return 0;
}

void MainWindow::windowDestroyed(QObject* obj)
{
    disconnect(obj, SIGNAL(destroyed(QObject*)),
               this, SLOT(windowDestroyed(QObject*)));
    //qDebug() << "No window" << resultWindow;

    searchButton->setEnabled(true);
}
