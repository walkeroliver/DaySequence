// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "visuals.h"
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

QVector<ThemePreset> Theme::presets() {
    return {{"forest", "苔绿 · 安静专注"},   {"ocean", "海盐蓝 · 轻盈清爽"},
            {"lilac", "雾紫 · 柔和灵感"},    {"sun", "暖杏 · 阳光午后"},
            {"night", "夜色 · 深色模式"},    {"chinese", "中式 · 宣纸朱砂"},
            {"european", "欧式 · 象牙酒红"}, {"american", "美式 · 雾灰海军蓝"}};
}
QString Theme::displayName(const QString &id) {
    for (const auto &preset : presets())
        if (preset.id == id)
            return preset.label.section(" · ", 0, 0);
    return QStringLiteral("苔绿");
}
Theme Theme::named(const QString &name) {
    if (name == "chinese")
        return {"#F5F1E8", "#FFFCF5", "#382F29", "#91816E", "#E6DCCB", "#A64437", "#F3E5DA"};
    if (name == "european")
        return {"#F2EEF0", "#FFFCF8", "#3E3039", "#927E89", "#E5DADF", "#784658", "#F0E4E8"};
    if (name == "american")
        return {"#EEF2F5", "#FFFFFF", "#283A4A", "#7F8E9B", "#DCE4EB", "#315C81", "#E5EDF5"};
    if (name == "night")
        return {"#171C24", "#222A35", "#EDF2F7", "#98A8BC", "#354150", "#92B6FE", "#2C3D55"};
    if (name == "ocean")
        return {"#F0F4F8", "#FFFFFF", "#253747", "#8795A4", "#E4EBF1", "#5F87B1", "#EAF0F7"};
    if (name == "lilac")
        return {"#F5F2F8", "#FFFFFF", "#383046", "#998EA7", "#EBE5F0", "#9880B7", "#F0EAF7"};
    if (name == "sun")
        return {"#FAF6EE", "#FFFFFF", "#493C2D", "#AA9984", "#EFE7DB", "#C39553", "#F8EFDF"};
    return {"#F3F5F1", "#FFFFFF", "#283D34", "#8A978F", "#E7ECE7", "#608870", "#EDF3EC"};
}
static void paintFont(QPainter &p, int size, bool bold = false) {
    QFont f("Microsoft YaHei UI");
    f.setPixelSize(size);
    f.setBold(bold);
    p.setFont(f);
}
CalendarView::CalendarView(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
QRectF CalendarView::cell(int day) const {
    if (!month.isValid())
        return {};
    const int offset = QDate(month.year(), month.month(), 1).dayOfWeek() - 1;
    const int position = offset + day - 1;
    const int rows = (offset + month.daysInMonth() + 6) / 7;
    const qreal columnWidth = width() / 7.0;
    const qreal rowHeight = qMax(0.0, height() - 36.0) / rows;
    const qreal side = qMax(0.0, qMin(44.0, qMin(columnWidth, rowHeight) - 8.0));
    return {position % 7 * columnWidth + (columnWidth - side) / 2,
            36 + position / 7 * rowHeight + (rowHeight - side) / 2, side, side};
}
void CalendarView::paintEvent(QPaintEvent *) {
    if (!month.isValid())
        return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    paintFont(p, 12);
    p.setPen(theme.muted);
    QStringList week = {"一", "二", "三", "四", "五", "六", "日"};
    for (int i = 0; i < 7; i++)
        p.drawText(QRectF(i * width() / 7.0, 0, width() / 7.0, 28), Qt::AlignCenter, week[i]);
    for (int day = 1; day <= month.daysInMonth(); day++) {
        auto r = cell(day);
        QDate d(month.year(), month.month(), day);
        bool active = d == selected;
        p.setPen(Qt::NoPen);
        p.setBrush(active ? theme.accent : (d == QDate::currentDate() ? theme.soft : theme.panel));
        p.drawRoundedRect(r, r.width() / 4, r.width() / 4);
        p.setPen(active ? QColor(Qt::white) : theme.text);
        paintFont(p, 14, active || d == QDate::currentDate());
        p.drawText(r, Qt::AlignCenter, QString::number(day));
    }
}
void CalendarView::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton || !month.isValid())
        return;
    for (int d = 1; d <= month.daysInMonth(); d++)
        if (cell(d).contains(e->position()) && onSelect)
            onSelect(QDate(month.year(), month.month(), d));
}
void CalendarView::mouseMoveEvent(QMouseEvent *e) {
    setCursor(Qt::ArrowCursor);
    for (int d = 1; d <= month.daysInMonth(); d++)
        if (cell(d).contains(e->position())) {
            setCursor(Qt::PointingHandCursor);
            int total = stats.dailyTotal.value(d - 1), done = stats.dailyDone.value(d - 1);
            QToolTip::showText(e->globalPosition().toPoint(),
                               total ? QString("%1日 · 完成 %2 / %3 项").arg(d).arg(done).arg(total)
                                     : QString("%1日 · 暂无计划").arg(d),
                               this);
            return;
        }
    QToolTip::hideText();
}
ChartView::ChartView(bool value, QWidget *parent) : QWidget(parent), donut(value) {
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
void ChartView::paintEvent(QPaintEvent *) {
    if (!month.isValid())
        return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if (donut) {
        qreal size = qMin(width() - 40.0, height() - 66.0);
        size = qMin(size, 210.0);
        QRectF ring((width() - size) / 2, 12, size, size);
        QPen pen(theme.line, 20, Qt::SolidLine, Qt::FlatCap);
        p.setPen(pen);
        p.drawEllipse(ring.adjusted(12, 12, -12, -12));
        if (stats.total && stats.done) {
            pen.setColor(theme.accent);
            p.setPen(pen);
            p.drawArc(ring.adjusted(12, 12, -12, -12), 90 * 16,
                      -qRound(stats.done * 5760.0 / stats.total));
        }
        p.setPen(theme.text);
        paintFont(p, 36, true);
        p.drawText(ring.adjusted(0, -8, 0, -8), Qt::AlignCenter,
                   QString::number(stats.rate()) + "%");
        p.setPen(theme.muted);
        paintFont(p, 12);
        p.drawText(QRectF(ring.left(), ring.center().y() + 24, size, 25), Qt::AlignCenter,
                   stats.total ? "目标完成率" : "暂无计划");
        paintFont(p, 12);
        p.setPen(theme.accent);
        p.drawText(
            QRectF(0, height() - 32, width(), 25), Qt::AlignCenter,
            QString("● 已完成 %1     ○ 未完成 %2").arg(stats.done).arg(stats.total - stats.done));
        return;
    }
    QRectF plot(36, 18, width() - 48, height() - 52);
    int days = month.daysInMonth();
    int max = 1;
    for (int n : stats.dailyTotal)
        max = qMax(max, n);
    int top = qMax(4, ((max + 3) / 4) * 4);
    paintFont(p, 10);
    for (int i = 0; i <= 4; i++) {
        qreal y = plot.bottom() - plot.height() * i / 4;
        p.setPen(theme.line);
        p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        p.setPen(theme.muted);
        p.drawText(QRectF(0, y - 9, 27, 18), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(top * i / 4));
    }
    qreal slot = plot.width() / days, bw = qMax(3.0, slot * 0.55);
    for (int i = 0; i < days; i++) {
        qreal x = plot.left() + slot * (i + 0.5) - bw / 2;
        int total = stats.dailyTotal.value(i), done = stats.dailyDone.value(i);
        if (total) {
            QRectF r(x, plot.bottom() - plot.height() * total / top, bw,
                     plot.height() * total / top);
            p.setPen(Qt::NoPen);
            p.setBrush(theme.line);
            p.drawRoundedRect(r, 3, 3);
            if (done) {
                r.setTop(plot.bottom() - plot.height() * done / top);
                p.setBrush(theme.accent);
                p.drawRoundedRect(r, 3, 3);
            }
        }
        if (i == 0 || ((i + 1) % 5 == 0 && days - i - 1 > 1) || i + 1 == days) {
            p.setPen(theme.muted);
            p.drawText(QRectF(x - 9, plot.bottom() + 8, bw + 18, 20), Qt::AlignCenter,
                       QString::number(i + 1));
        }
    }
    if (!stats.total) {
        p.setPen(theme.muted);
        paintFont(p, 13);
        p.drawText(plot, Qt::AlignCenter, "这个月的故事，等你写下第一项计划");
    }
}
void ChartView::mouseMoveEvent(QMouseEvent *e) {
    if (!month.isValid())
        return;
    if (donut) {
        QToolTip::showText(e->globalPosition().toPoint(),
                           QString("已完成 %1 项 / 总共 %2 项\n完成率按目标数量计算")
                               .arg(stats.done)
                               .arg(stats.total),
                           this);
        return;
    }
    QRectF plot(36, 18, width() - 48, height() - 52);
    if (!plot.contains(e->position())) {
        QToolTip::hideText();
        return;
    }
    int day = qBound(0, int((e->position().x() - plot.left()) / plot.width() * month.daysInMonth()),
                     month.daysInMonth() - 1);
    QToolTip::showText(e->globalPosition().toPoint(),
                       QString("%1月%2日\n已完成 %3 项 · 未完成 %4 项")
                           .arg(month.month())
                           .arg(day + 1)
                           .arg(stats.dailyDone.value(day))
                           .arg(stats.dailyTotal.value(day) - stats.dailyDone.value(day)),
                       this);
}
