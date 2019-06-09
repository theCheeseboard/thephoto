/****************************************
 *
 *   INSERT-PROJECT-NAME-HERE - INSERT-GENERIC-NAME-HERE
 *   Copyright (C) 2019 Victor Tran
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * *************************************/
#ifndef IMAGEVIEWSIDEBAR_H
#define IMAGEVIEWSIDEBAR_H

#include <QWidget>
#include <QMap>

namespace Ui {
    class ImageViewSidebar;
}

struct ImageViewSidebarSection {
    QString title;
    QList<QPair<QString, QString>> fields;
};

struct ImageViewSidebarPrivate;
class ImageViewSidebar : public QWidget
{
        Q_OBJECT

    public:
        explicit ImageViewSidebar(QWidget *parent = nullptr);
        ~ImageViewSidebar();

    public slots:
        void show();
        void hide();

        void setSections(QList<ImageViewSidebarSection> sections);

    private:
        Ui::ImageViewSidebar *ui;

        ImageViewSidebarPrivate* d;

        void mousePressEvent(QMouseEvent *event);
        void mouseMoveEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
};

#endif // IMAGEVIEWSIDEBAR_H
