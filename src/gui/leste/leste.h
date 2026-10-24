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

#ifndef FAHRPLAN_LESTE_H
#define FAHRPLAN_LESTE_H

#include <QString>
#include <QStyledItemDelegate>

class QAbstractScrollArea;
class QMainWindow;
class QWidget;

// Small helpers around what the Maemo Leste platform plugin (qt-platform-maemo)
// and the QtMaemo5 library offer to Qt 5 programs. Qt 4 on Fremantle had
// widget attributes for this (Qt::WA_Maemo5StackedWindow and friends); on
// Leste the platform plugin reads dynamic properties instead.
namespace Leste
{
    // Turns a top-level window into a Hildon stackable window. A window whose
    // QWidget parent is a stacked window is pushed on top of it and gets the
    // back arrow in the title bar. Must be called before show().
    void makeStackedWindow(QWidget *window);

    // The actions of the menu bar of a QMainWindow make up its Hildon
    // application menu (opened from the title bar). The platform plugin only
    // hides the menu bar widget itself when it has submenus, so hide it here;
    // call this after the actions have been added.
    void useApplicationMenu(QMainWindow *window);

    // The spinning progress indicator in the Hildon title bar.
    void setBusy(QWidget *window, bool busy);

    // Kinetic (finger) scrolling for lists and scroll areas.
    void enableKineticScrolling(QAbstractScrollArea *area);

    // A Hildon information banner.
    void showBanner(QWidget *parent, const QString &text);

    // Theme colours as #rrggbb for use in rich text.
    QString secondaryTextColor();
    QString activeTextColor();

    // Escapes text for use inside rich text.
    QString esc(const QString &text);

    // The finger-friendly height of a single list row.
    int rowHeight(const QWidget *widget);

    // Debugging aid for devices without a screenshot tool: saves the window
    // on top (or an open dialog) to the given PNG file every two seconds.
    // Enabled with the environment variable FAHRPLAN_GRAB=<file.png>.
    void installWindowGrabber(const QString &fileName);
}

// Renders the Qt::DisplayRole of an item as rich text (one or more lines),
// so that list rows can have a bold first line and a smaller grey second one.
class RichItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit RichItemDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    QString html(const QModelIndex &index) const;
};

#endif // FAHRPLAN_LESTE_H
