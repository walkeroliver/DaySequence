// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "countdown.h"
#include <algorithm>

Countdown::Countdown(QObject *parent) : QObject(parent) {
    pulse.setInterval(100);
    pulse.setTimerType(Qt::PreciseTimer);
    connect(&pulse, &QTimer::timeout, this, &Countdown::updateTime);
}
bool Countdown::setDuration(qint64 milliseconds) {
    if (status == State::Running || status == State::Paused || milliseconds < 1 ||
        milliseconds > 86399000)
        return false;
    total = pending = milliseconds;
    status = State::Ready;
    emit changed();
    return true;
}
qint64 Countdown::remaining() const {
    return status == State::Running ? std::max<qint64>(0, pending - elapsed.elapsed()) : pending;
}
void Countdown::start() {
    if (status == State::Running)
        return;
    if (status != State::Paused)
        pending = total;
    elapsed.start();
    status = State::Running;
    pulse.start();
    emit changed();
}
void Countdown::pause() {
    if (status != State::Running)
        return;
    updateTime();
    if (status != State::Running)
        return;
    pending = remaining();
    pulse.stop();
    status = State::Paused;
    emit changed();
}
void Countdown::reset() {
    pulse.stop();
    status = State::Ready;
    pending = total;
    emit changed();
}
void Countdown::updateTime() {
    if (status != State::Running)
        return;
    if (remaining() == 0) {
        pulse.stop();
        pending = 0;
        status = State::Finished;
        emit changed();
        emit finished();
    } else
        emit changed();
}