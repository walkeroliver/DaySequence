// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

class Countdown final : public QObject {
    Q_OBJECT
  public:
    enum class State { Ready, Running, Paused, Finished };
    Q_ENUM(State)
    explicit Countdown(QObject *parent = nullptr);
    bool setDuration(qint64 milliseconds);
    qint64 duration() const {
        return total;
    }
    qint64 remaining() const;
    State state() const {
        return status;
    }
    void start();
    void pause();
    void reset();

  signals:
    void changed();
    void finished();

  private:
    State status = State::Ready;
    qint64 total = 25 * 60 * 1000, pending = total;
    QElapsedTimer elapsed;
    QTimer pulse;
    void updateTime();
};