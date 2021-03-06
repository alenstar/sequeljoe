/*
 * Copyright 2014 Oliver Giles
 *
 * This file is part of SequelJoe. SequelJoe is licensed under the
 * GNU GPL version 3. See LICENSE or <http://www.gnu.org/licenses/>
 * for more information
 */
#include "loadingoverlay.h"

#include <QPainter>
#include <QPaintEvent>
#include <QApplication>
#include <QTimeLine>

Spinner* LoadingOverlay::spinner = nullptr;

class Spinner {
public:
    Spinner(int rOuter, int rInner) :
        pixmap_(2*rOuter, 2*rOuter)
    {
        QPainterPath path;
        path.setFillRule(Qt::OddEvenFill);
        path.addEllipse(QPointF(rOuter, rOuter), rOuter, rOuter);
        path.addEllipse(QPointF(rOuter, rOuter), rInner, rInner);

        pixmap_.fill(Qt::transparent);
        QPainter p(&pixmap_);

        p.setPen(Qt::NoPen);
        p.setBrush(Qt::transparent);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawPath(path);

        QConicalGradient gradient(QPointF(rOuter,rOuter), 0.0);
        gradient.setColorAt(0.0, Qt::transparent);
        gradient.setColorAt(0.05, QApplication::palette().color(QPalette::Highlight));
        gradient.setColorAt(0.75, Qt::transparent);
        p.setBrush(gradient);
        p.drawPath(path);
        p.end();
    }

    const QPixmap& pixmap() const { return pixmap_; }

private:
    QPixmap pixmap_;
};



LoadingOverlay::LoadingOverlay(QWidget *parent) :
    QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    if(spinner == nullptr)
        spinner = new Spinner(60, 50);

    timeline = new QTimeLine(1000);
    timeline->setFrameRange(0,360);
    timeline->setCurveShape(QTimeLine::LinearCurve);
    connect(timeline, SIGNAL(frameChanged(int)), this, SLOT(repaint()));
    timeline->setLoopCount(0); // loop forever

    fade = new QTimeLine(400);
    fade->setFrameRange(0,255);
    fade->setCurveShape(QTimeLine::EaseInCurve);
    connect(fade, SIGNAL(frameChanged(int)), this, SLOT(repaint()));

}

LoadingOverlay::~LoadingOverlay() {
    delete timeline;
}

void LoadingOverlay::showEvent(QShowEvent *e) {
    QWidget::showEvent(e);
    timeline->start();
    fade->start();
}

void LoadingOverlay::hideEvent(QHideEvent * e) {
    QWidget::hideEvent(e);
    timeline->stop();
    fade->stop();
}

void LoadingOverlay::paintEvent(QPaintEvent * e) {
    QWidget::paintEvent(e);
    QPainter painter(this);
    QRect r = e->rect();

    painter.setOpacity(fade->currentFrame()/255.);

    painter.fillRect(r, QColor(255,255,255,96));

    const QPixmap& px = spinner->pixmap();
    painter.translate(r.width()/2,r.height()/2);
    painter.rotate(timeline->currentFrame());
    painter.drawPixmap(-px.width()/2,-px.width()/2,spinner->pixmap());
}

