// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "icons.h"
#include <QIconEngine>
#include <QPainter>

namespace {
class VectorIcon final : public QIconEngine {
  public:
    VectorIcon(QString kind, QColor color) : kind(std::move(kind)), color(color) {}
    QIconEngine *clone() const override {
        return new VectorIcon(kind, color);
    }
    QString key() const override {
        return "DaySequenceVectorIcon";
    }
    void paint(QPainter *p, const QRect &rect, QIcon::Mode mode, QIcon::State) override {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->translate(rect.topLeft());
        p->scale(rect.width() / 24.0, rect.height() / 24.0);
        QColor ink = color;
        if (mode == QIcon::Disabled)
            ink.setAlpha(90);
        p->setPen(QPen(ink, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->setBrush(Qt::NoBrush);
        if (kind == "min")
            p->drawLine(6, 12, 18, 12);
        else if (kind == "max")
            p->drawRect(QRectF(6, 6, 12, 12));
        else if (kind == "restore") {
            p->drawRect(QRectF(5, 8, 11, 11));
            p->drawLine(9, 5, 19, 5);
            p->drawLine(19, 5, 19, 15);
        } else if (kind == "close") {
            p->drawLine(7, 7, 17, 17);
            p->drawLine(17, 7, 7, 17);
        } else if (kind == "power") {
            p->drawArc(QRectF(5, 5, 14, 14), 130 * 16, 280 * 16);
            p->drawLine(12, 3, 12, 11);
        } else if (kind == "refresh") {
            p->drawArc(QRectF(5, 5, 14, 14), 35 * 16, 290 * 16);
            p->drawLine(19, 4, 19, 10);
            p->drawLine(19, 10, 13, 10);
        } else {
            p->drawLine(5, 7, 19, 7);
            p->drawLine(5, 17, 19, 17);
            p->setBrush(ink);
            p->drawEllipse(QPointF(9, 7), 2, 2);
            p->drawEllipse(QPointF(15, 17), 2, 2);
        }
        p->restore();
    }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override {
        return scaledPixmap(size, mode, state, 1);
    }
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state,
                         qreal scale) override {
        QPixmap result(size * scale);
        result.setDevicePixelRatio(scale);
        result.fill(Qt::transparent);
        QPainter p(&result);
        paint(&p, QRect(QPoint(), size), mode, state);
        return result;
    }

  private:
    QString kind;
    QColor color;
};
} // namespace
QIcon appIcon() {
    return QIcon(":/app.svg");
}
QIcon lineIcon(const QString &kind, const QColor &color) {
    return QIcon(new VectorIcon(kind, color));
}
