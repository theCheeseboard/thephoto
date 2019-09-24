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
#include "imageview.h"
#include <tvariantanimation.h>
#include <QGenericMatrix>

struct ImageViewSidebarPrivate {
    tVariantAnimation* preferredWidth;
    QWidget* infoContainer = nullptr;
    ImageView* parentView;

    QImage editingImage;
    QGenericMatrix<3,3,qreal> currentTransform;
    int rightTransformAngle = 0;
    int scaleX = 1, scaleY = 1;
};

ImageViewSidebar::ImageViewSidebar(ImageView *parent) :
    QWidget(parent),
    ui(new Ui::ImageViewSidebar)
{
    ui->setupUi(this);
    d = new ImageViewSidebarPrivate();
    d->parentView = parent;

    d->preferredWidth = new tVariantAnimation();
    d->preferredWidth->setStartValue(0);
    d->preferredWidth->setEndValue(SC_DPI(300));
    d->preferredWidth->setDuration(250);
    d->preferredWidth->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->preferredWidth, &tVariantAnimation::valueChanged, this, [=](QVariant value) {
        this->setFixedWidth(value.toInt());
    });

    ui->stackedWidget->setCurrentAnimation(tStackedWidget::Fade);
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

void ImageViewSidebar::startEdit(QImage originalImage)
{
    d->currentTransform.setToIdentity();
    d->editingImage = originalImage.copy();
    ui->straightenSlider->setValue(0);

    ui->stackedWidget->setCurrentIndex(1);
}

void ImageViewSidebar::endEdit()
{
    //Delete matrix

    ui->stackedWidget->setCurrentIndex(0);
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

void ImageViewSidebar::animateNewTransform()
{
    QTransform newTransform;
    newTransform.reset();
    newTransform.rotate(d->rightTransformAngle + ui->straightenSlider->value());
    newTransform.scale(d->scaleX, d->scaleY);

    for (int i = 0; i < 9; i++) {
        //Ugly :(
        qreal matrixData = 0;
        switch (i) {
            case 0: matrixData = newTransform.m11(); break;
            case 1: matrixData = newTransform.m12(); break;
            case 2: matrixData = newTransform.m13(); break;
            case 3: matrixData = newTransform.m21(); break;
            case 4: matrixData = newTransform.m22(); break;
            case 5: matrixData = newTransform.m23(); break;
            case 6: matrixData = newTransform.m31(); break;
            case 7: matrixData = newTransform.m32(); break;
            case 8: matrixData = newTransform.m33(); break;
        }
        if (!qFuzzyCompare(matrixData, d->currentTransform.data()[i])) {
            qreal* valueToChange = &d->currentTransform.data()[i];
            tVariantAnimation* anim = new tVariantAnimation();
            anim->setStartValue(d->currentTransform.data()[i]);
            anim->setEndValue(matrixData);
            anim->setDuration(500);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            connect(anim, &tVariantAnimation::valueChanged, this, [=](QVariant value) {
                *valueToChange = value.toReal();
                d->parentView->editTransform(transformFromCurrentMatrix());
            });
            connect(anim, &tVariantAnimation::finished, anim, &tVariantAnimation::deleteLater);
            anim->start();
        }
    }
}

QTransform ImageViewSidebar::transformFromCurrentMatrix()
{
    QTransform transform(d->currentTransform(0,0), d->currentTransform(0,1), d->currentTransform(0,2),
                         d->currentTransform(1,0), d->currentTransform(1,1), d->currentTransform(1,2),
                         d->currentTransform(2,0), d->currentTransform(2,1), d->currentTransform(2,2));
    return transform;
}

void ImageViewSidebar::on_rotateClockwiseButton_clicked()
{
    d->rightTransformAngle += 90;
    this->animateNewTransform();
}

void ImageViewSidebar::on_rotateAnticlockwiseButton_clicked()
{
    d->rightTransformAngle -= 90;
    this->animateNewTransform();
}

void ImageViewSidebar::on_flipHorizontally_clicked()
{
    d->scaleX *= -1;
    this->animateNewTransform();
}

void ImageViewSidebar::on_flipVertically_clicked()
{
    d->scaleY *= -1;
    this->animateNewTransform();
}

void ImageViewSidebar::on_straightenSlider_valueChanged(int value)
{
    this->animateNewTransform();
}

void ImageViewSidebar::on_grayscaleButton_clicked()
{
    for (int y = 0; y < d->editingImage.height(); y++) {
        uchar* line = d->editingImage.scanLine(y);
        for (int x = 0; x < d->editingImage.width(); x++) {
            QRgb* px = reinterpret_cast<QRgb*>(line + x * 4);
            int gray = qGray(*px);
            *px = QColor(gray, gray, gray).rgb();
        }
    }
    d->parentView->editImage(d->editingImage);
}
