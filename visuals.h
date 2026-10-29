// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include "planner.h"
#include <QColor>
#include <QWidget>
#include <functional>

struct ThemePreset {
    QString id, label;
};
struct Theme {
    QColor bg, panel, text, muted, line, accent, soft;
    static Theme named(const QString &name);
    static QVector<ThemePreset> presets();
    static QString displayName(const QString &id);
};
class CalendarView : public QWidget {
  public:
    CalendarView(QWidget *parent = nullptr);
    QDate month, selected;
    MonthStats stats;
    Theme theme;
    std::function<void(QDate)> onSelect;
    QSize minimumSizeHint() const override {
        return {270, 230};
    }

  protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

  private:
    QRectF cell(int day) const;
};
class ChartView : public QWidget {
  public:
    explicit ChartView(bool donut, QWidget *parent = nullptr);
    MonthStats stats;
    Theme theme;
    QDate month;
    QSize minimumSizeHint() const override {
        return {260, 230};
    }

  protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

  private:
    bool donut;
};
