// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include "countdown.h"
#include "visuals.h"
#include <QWidget>
class QLabel;
class QPushButton;
class QSpinBox;
class QCheckBox;
class QButtonGroup;
class QSettings;
class FocusDial;

class FocusClock final : public QWidget {
    Q_OBJECT
  public:
    explicit FocusClock(QSettings &settings, QWidget *parent = nullptr);
    void setTheme(const Theme &theme);
    Countdown *countdown() {
        return &timer;
    }

  signals:
    void completed(bool sound);

  private:
    Countdown timer;
    QSettings &settings;
    FocusDial *dial;
    QLabel *status, *finishTime;
    QPushButton *startButton, *resetButton;
    QSpinBox *hours, *minutes, *seconds;
    QCheckBox *sound;
    QButtonGroup *presets;
    void setSeconds(int value);
    void configure();
    void refresh();
};