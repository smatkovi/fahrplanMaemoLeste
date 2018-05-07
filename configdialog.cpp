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

#include "configdialog.h"

configDialog::configDialog(QSettings *mainSettings, QWidget *mainParent)
{
    setAttribute(Qt::WA_Maemo5AutoOrientation, true);
    connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(orientationChanged()));

    settings = mainSettings;
    parent   = mainParent;

    setWindowTitle(tr("Settings"));

    QGridLayout *layout = new QGridLayout();

    //Ok Button
    okButton = new QPushButton(tr("Save"));
    connect(okButton, SIGNAL(clicked(bool)), this, SLOT(okButtonClicked()));

    //Store last search
    storelastSearchBox     = new QCheckBox();
    storelastSearchBox->setText(tr("Store last searched stations"));
    storelastSearchBox->setChecked(settings->value("storelastsearch", true).toBool());
    connect(storelastSearchBox, SIGNAL(stateChanged(int)), this, SLOT(storelastSearchToggled(int)));

    //Store last search
    prefillSearchBox     = new QCheckBox();
    prefillSearchBox->setText(tr("Prefill last searched station"));
    prefillSearchBox->setChecked(settings->value("prefillsearchbox", true).toBool());
    prefillSearchBox->setEnabled(storelastSearchBox->isChecked());

    //Gps early start checkbox
    gpsBox     = new QCheckBox();
    gpsBox->setText(tr("Launch GPS on startup"));
    gpsBox->setChecked(settings->value("autostartGPS", true).toBool());

    //Gps fallback backend checkbox
    gpsBackendBox     = new QCheckBox();
    gpsBackendBox->setText(tr("Use OpenStreetMap as fallback for GPS"));
    gpsBackendBox->setChecked(settings->value("openstreetmapfallback", true).toBool());

    //Backend Selector
    QStandardItemModel * backendModel = new QStandardItemModel(0, 1);
    QStandardItem *item = new QStandardItem(tr("DB - Germany"));
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    backendModel->appendRow(item);
    item = new QStandardItem(tr("MVV - Germany"));
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    backendModel->appendRow(item);
/*    item = new QStandardItem(tr("SBB Legacy - Swiss")); //this backend is currently broken. 10. June 2011, CaCO3
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    backendModel->appendRow(item); */
    item = new QStandardItem(tr("NSW - Australia"));
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    backendModel->appendRow(item);
    item = new QStandardItem(tr("ÖBB - Austria"));
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    backendModel->appendRow(item);
    item = new QStandardItem(tr("SBB - Swiss"));
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    backendModel->appendRow(item);
    item = new QStandardItem(tr("Rejseplanen - Denmark"));
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    backendModel->appendRow(item);
    item = new QStandardItem(tr("Translink - Australia"));
    item->setTextAlignment(Qt::AlignCenter);
    item->setEditable(false);
    backendModel->appendRow(item);
    QMaemo5ValueButton *backendButton = new QMaemo5ValueButton(tr("Databackend"));
    backendButton->setValueLayout(QMaemo5ValueButton::ValueUnderTextCentered);
    backendSelector = new QMaemo5ListPickSelector;
    backendSelector->setModel(backendModel);
    backendButton->setPickSelector(backendSelector);

    QString backendType = settings->value("backend", "parserMobileBahnDe").toString();
    //Read Settings
    if (backendType == "parserMobileBahnDe") //DB
    {
        backendSelector->setCurrentIndex(0);
    } else if (backendType == "parserMvvDe") //MVV
    {
            backendSelector->setCurrentIndex(1);
    }/* else if (backendType == "parserSbbCh")   //this backend is currently broken. 10. June 2011, CaCO3
    {
            backendSelector->setCurrentIndex(2);
    }*/ else if (backendType == "parser131500ComAu") //NSW
    {
            backendSelector->setCurrentIndex(2);
    } else if (backendType == "parserXmlOebbAt") //OeBB
    {
            backendSelector->setCurrentIndex(3);
    } else if (backendType == "parserXmlSbbCh") //SBB
    {
        backendSelector->setCurrentIndex(4);
    } else if (backendType == "parserXmlRejseplanenDk") //Denmark
    {
        backendSelector->setCurrentIndex(5);
    } else if (backendType == "parserTranslink") //Queensland
    {
        backendSelector->setCurrentIndex(6);
    } else
    {
       //fallback is bahn.de
        backendSelector->setCurrentIndex(0);
    }

    //Create Layout
    layout->addWidget(gpsBox, 0, 0);
    layout->addWidget(gpsBackendBox, 1, 0);
    layout->addWidget(storelastSearchBox, 2, 0);
    layout->addWidget(prefillSearchBox, 3, 0);
    layout->addWidget(backendButton, 4, 0);
    layout->addWidget(okButton, 4, 1);
    setLayout(layout);

    orientationChanged();
}

void configDialog::orientationChanged()
{
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    QGridLayout *layout = (QGridLayout*)this->layout();
    if (screenGeometry.width() > screenGeometry.height())
    {
        layout->addWidget(okButton, 4, 1);
    } else {
        layout->addWidget(okButton, 5, 0);
    }
}

void configDialog::storelastSearchToggled(int state)
{
    Q_UNUSED(state);
    prefillSearchBox->setEnabled(storelastSearchBox->isChecked());
}

void configDialog::okButtonClicked()
{
    settings->setValue("autostartGPS", gpsBox->isChecked());
    settings->setValue("openstreetmapfallback", gpsBackendBox->isChecked());
    settings->setValue("storelastsearch", storelastSearchBox->isChecked());
    settings->setValue("prefillsearchbox", prefillSearchBox->isChecked());

    QString oldBackend = settings->value("backend", "parserMobileBahnDe").toString();

    if (backendSelector->currentIndex() == 0)
    {
        settings->setValue("backend", "parserMobileBahnDe");
    } else if (backendSelector->currentIndex() == 1)
    {
        settings->setValue("backend", "parserMvvDe");
    }/* else if (backendSelector->currentIndex() == 2)  //this backend is currently broken. 10. June 2011, CaCO3
    {
        settings->setValue("backend", "parserSbbCh");
    }*/ else if (backendSelector->currentIndex() == 2)
    {
        settings->setValue("backend", "parser131500ComAu");
    } else if (backendSelector->currentIndex() == 3)
    {
        settings->setValue("backend", "parserXmlOebbAt");
    } else if (backendSelector->currentIndex() == 4)
    {
        settings->setValue("backend", "parserXmlSbbCh");
    } else if (backendSelector->currentIndex() == 5)
    {
        settings->setValue("backend", "parserXmlRejseplanenDk");
    } else if (backendSelector->currentIndex() == 6)
    {
        settings->setValue("backend", "parserTranslink");
    } else
    {
        settings->setValue("backend", "parserMobileBahnDe");
    }

    QString newBackend = settings->value("backend", "parserMobileBahnDe").toString();

    MainWindow *mainWnd = (MainWindow*)parent;
    mainWnd->initBackend(oldBackend != newBackend);

    hide();
}
