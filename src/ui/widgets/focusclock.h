// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include "countdown.h"
#include "musiclibrary.h"
#include "visuals.h"
#include <QWidget>
#include <memory>
class QLabel;
class QPushButton;
class QSpinBox;
class QCheckBox;
class QButtonGroup;
class QSettings;
class FocusDial;
class FocusAlarm;
class MusicLibrary;
class ChoiceBox;
class QSlider;

class FocusClock final : public QWidget {
    Q_OBJECT
  public:
    explicit FocusClock(QSettings &settings, QWidget *parent = nullptr);
    void setTheme(const Theme &theme);
    void stopAlarm();
    bool importAudio(const QString &file, QString *error = nullptr);
    bool ringing() const;
    void reloadMusic();

    Countdown *countdown() {
        return &timer;
    }

  signals:
    void completed(bool sound);
    void alarmActiveChanged(bool active);

  private:
    Countdown timer;
    QSettings &settings;
    FocusDial *dial;
    QLabel *status, *finishTime;
    QPushButton *startButton, *resetButton;
    QSpinBox *hours, *minutes, *seconds;
    QCheckBox *sound, *loopSound;
    FocusAlarm *alarm;
    std::unique_ptr<MusicLibrary> music;
    ChoiceBox *audioChoice;
    QSlider *volume;
    QPushButton *previewButton, *stopAlarmButton, *chooseAudioButton;
    QLabel *audioMessage, *volumeCaption;
    QWidget *alarmBanner;
    QUrl selectedAudio() const;
    void refreshAudio();
    QButtonGroup *presets;
    void setSeconds(int value);
    void configure();
    void refresh();
};