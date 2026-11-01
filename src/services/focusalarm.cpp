// SPDX-License-Identifier: GPL-3.0-only
#include "focusalarm.h"
#include <QAudioDevice>
#include <QFileInfo>
#include <QMediaDevices>
FocusAlarm::FocusAlarm(QObject *parent) : QObject(parent), player(this), output(this) {
    player.setObjectName("focusAlarmPlayer");
    output.setObjectName("focusAlarmOutput");
    player.setAudioOutput(&output);
    output.setVolume(.7);
    previewLimit.setSingleShot(true);
    loadingLimit.setSingleShot(true);
    connect(&previewLimit, &QTimer::timeout, this, &FocusAlarm::stop);
    connect(&loadingLimit, &QTimer::timeout, this, &FocusAlarm::failure);
    connect(&player, &QMediaPlayer::errorOccurred, this, [this] {
        if (requested)
            failure();
    });
    connect(&player, &QMediaPlayer::playbackStateChanged, this, [this](auto state) {
        if (state == QMediaPlayer::PlayingState)
            loadingLimit.stop();
    });
    connect(&player, &QMediaPlayer::mediaStatusChanged, this, [this](auto status) {
        if (requested && status == QMediaPlayer::EndOfMedia && !looping)
            stop();
    });
}
QUrl FocusAlarm::preset(const QString &name) {
    const QString id = name == "soft" || name == "bell" ? name : "chime";
    return QUrl("qrc:/sounds/" + id + ".wav");
}
void FocusAlarm::setVolume(int value) {
    output.setVolume(qBound(0, value, 100) / 100.0);
}
void FocusAlarm::stop() {
    const bool wasActive = requested;
    requested = false;
    player.stop();
    previewLimit.stop();
    loadingLimit.stop();
    if (wasActive)
        emit activeChanged(false);
}
void FocusAlarm::play(const QUrl &source, bool repeat, bool preview) {
    stop();
    if (QMediaDevices::defaultAudioOutput().isNull()) {
        emit failed("未找到音频输出设备，请检查扬声器或耳机。");
        return;
    }
    previewMode = preview;
    looping = repeat && !preview;
    fallback = false;
    requested = true;
    emit activeChanged(true);
    if (source.isLocalFile() && !QFileInfo::exists(source.toLocalFile())) {
        fallback = true;
        emit failed("自选音频已丢失，本次使用「清晨铃」。");
        load(preset("chime"));
    } else
        load(source);
    if (preview)
        previewLimit.start(8000);
}
void FocusAlarm::load(const QUrl &source) {
    player.setLoops(looping ? QMediaPlayer::Infinite : QMediaPlayer::Once);
    loadingLimit.start(10000);
    player.setSource(source);
    player.play();
}
void FocusAlarm::failure() {
    if (!requested)
        return;
    const bool retry = !fallback && player.source() != preset("chime");
    const bool repeat = looping, preview = previewMode;
    stop();
    if (retry) {
        fallback = true;
        requested = true;
        looping = repeat;
        previewMode = preview;
        emit failed("该音频无法播放，本次改用「清晨铃」。");
        emit activeChanged(true);
        load(preset("chime"));
        if (preview)
            previewLimit.start(8000);
    } else
        emit failed("铃声播放失败，请检查音频设备后重试。");
}
