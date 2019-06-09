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
#include "imageviewsidebar.h"
#include "ui_imageviewsidebar.h"

#include <QMouseEvent>
#include <QLabel>
#include <tvariantanimation.h>

struct ImageViewSidebarPrivate {
    tVariantAnimation* preferredWidth;
    QWidget* infoContainer = nullptr;
};

ImageViewSidebar::ImageViewSidebar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImageViewSidebar)
{
    ui->setupUi(this);
    d = new ImageViewSidebarPrivate();

    d->preferredWidth = new tVariantAnimation();
    d->preferredWidth->setStartValue(0);
    d->preferredWidth->setEndValue(SC_DPI(300));
    d->preferredWidth->setDuration(250);
    d->preferredWidth->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->preferredWidth, &tVariantAnimation::valueChanged, this, [=](QVariant value) {
        this->setFixedWidth(value.toInt());
    });
}

ImageViewSidebar::~ImageViewSidebar()
{
    delete ui;
    d->preferredWidth->deleteLater();
    if (d->infoContainer != nullptr) d->infoContainer->deleteLater();
    delete d;
}

void ImageViewSidebar::show() {
    d->preferredWidth->setDirection(tVariantAnimation::Forward);
    d->preferredWidth->start();
    QWidget::show();
}

void ImageViewSidebar::hide() {
    d->preferredWidth->setDirection(tVariantAnimation::Backward);
    d->preferredWidth->start();
}

void ImageViewSidebar::setSections(QList<ImageViewSidebarSection> sections) {
    if (d->infoContainer != nullptr) {
        ui->infoLayout->removeWidget(d->infoContainer);
        d->infoContainer->deleteLater();
    }

    d->infoContainer = new QWidget();
    d->infoContainer->setFixedWidth(SC_DPI(300));
    QBoxLayout* mainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    d->infoContainer->setLayout(mainLayout);
    ui->infoLayout->addWidget(d->infoContainer);

    for (ImageViewSidebarSection section : sections) {
        //Check that at least one field has a value
        bool validSection = false;
        for (QPair<QString, QString> value : section.fields) {
            if (value.second != "") {
                validSection = true;
            }
        }
        if (!validSection) continue; //Don't bother making stuff for this section; it doesn't contain anything

        QGridLayout* layout = new QGridLayout();
        layout->setContentsMargins(9, 9, 9, 9);
        layout->setSpacing(6);

        QLabel* label = new QLabel(d->infoContainer);
        label->setText(section.title.toUpper());

        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
        layout->addWidget(label, 0, 0, 1, 0);

        int i = 1;
        for (QPair<QString, QString> value : section.fields) {
            if (value.second == "") continue; //No value here, don't bother displaying it

            QLabel* firstLabel = new QLabel(d->infoContainer);
            firstLabel->setText(value.first);
            QLabel* secondLabel = new QLabel(d->infoContainer);
            secondLabel->setText(value.second);

            layout->addWidget(firstLabel, i, 0);
            layout->addWidget(secondLabel, i, 1);
            i++;
        }

        mainLayout->addLayout(layout);

        QFrame* line = new QFrame(d->infoContainer);
        line->setFixedHeight(1);
        line->setFrameShape(QFrame::HLine);
        mainLayout->addWidget(line);
    }

    mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
}

void ImageViewSidebar::mousePressEvent(QMouseEvent *event) {
    event->accept();
}

void ImageViewSidebar::mouseMoveEvent(QMouseEvent *event) {
    event->accept();
}

void ImageViewSidebar::mouseReleaseEvent(QMouseEvent *event) {
    event->accept();
}
