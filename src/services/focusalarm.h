// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QTimer>
class FocusAlarm final : public QObject {
    Q_OBJECT
  public:
    explicit FocusAlarm(QObject *parent = nullptr);
    static QUrl preset(const QString &name);
    void play(const QUrl &source, bool repeat, bool preview = false);
    void stop();
    void setVolume(int volume);
    bool active() const {
        return requested;
    }
    bool previewing() const {
        return previewMode;
    }
  signals:
    void activeChanged(bool active);
    void failed(QString message);

  private:
    QMediaPlayer player;
    QAudioOutput output;
    QTimer previewLimit, loadingLimit;
    bool requested = false, previewMode = false, looping = false, fallback = false;
    void load(const QUrl &source);
    void failure();
};
