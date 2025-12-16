#include "toggle_switch.h"

#include <QPainter>
#include <QMouseEvent>

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QAbstractButton(parent) {
    setCheckable(true);
    setFixedSize(100, 56);
}

QSize ToggleSwitch::sizeHint() const {
    return QSize(100, 56);
}

void ToggleSwitch::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal width = this->width();
    const qreal height = this->height();
    const qreal radius = height / 2.0;
    const qreal knobRadius = radius - 6;

    // Draw background track
    QColor trackColor = isChecked() ? QColor(255, 106, 0) : QColor(74, 74, 74);
    painter.setBrush(trackColor);
    painter.setPen(QPen(trackColor.lighter(110), 2));
    painter.drawRoundedRect(QRectF(0, 0, width, height), radius, radius);

    // Calculate knob position
    qreal knobX;
    if (isChecked()) {
        knobX = width - height / 2.0;
    } else {
        knobX = height / 2.0;
    }

    // Draw knob (white circle)
    painter.setBrush(QColor(255, 255, 255));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(knobX, height / 2.0), knobRadius, knobRadius);
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        toggle();
    }
    QAbstractButton::mouseReleaseEvent(event);
}
