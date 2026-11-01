// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "focusclock.h"
#include "choicebox.h"
#include "focusalarm.h"
#include "uicomponents.h"
#include <QSaveFile>
#include <QtWidgets>
#include <cmath>

namespace {
QString timeText(qint64 milliseconds) {
    const auto seconds = (milliseconds + 999) / 1000;
    const auto mm = seconds / 60, ss = seconds % 60;
    if (seconds >= 3600)
        return QString("%1:%2:%3")
            .arg(seconds / 3600, 2, 10, QChar('0'))
            .arg(mm % 60, 2, 10, QChar('0'))
            .arg(ss, 2, 10, QChar('0'));
    return QString("%1:%2").arg(mm, 2, 10, QChar('0')).arg(ss, 2, 10, QChar('0'));
}
} // namespace
class ToneSwitch final : public QCheckBox {
  public:
    explicit ToneSwitch(const QString &text) : QCheckBox(text) {
        setCursor(Qt::PointingHandCursor);
    }
    Theme theme = Theme::named("forest");
    QSize sizeHint() const override {
        return {fontMetrics().horizontalAdvance(text()) + 52, 28};
    }

  protected:
    bool hitButton(const QPoint &p) const override {
        return rect().contains(p);
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setOpacity(isEnabled() ? 1 : .45);
        const qreal y = (height() - 22) / 2.;
        p.setPen(Qt::NoPen);
        p.setBrush(isChecked() ? theme.accent : theme.line);
        p.drawRoundedRect(QRectF(0, y, 38, 22), 11, 11);
        p.setBrush(theme.panel);
        p.drawEllipse(QRectF(isChecked() ? 19 : 3, y + 3, 16, 16));
        p.setPen(theme.text);
        p.setFont(font());
        p.drawText(QRect(47, 0, width() - 47, height()), Qt::AlignVCenter, text());
        if (hasFocus()) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(theme.accent, 1, Qt::DotLine));
            p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 4, 4);
        }
    }
};
class DurationField final : public QSpinBox {
  public:
    using QSpinBox::QSpinBox;

  protected:
    void wheelEvent(QWheelEvent *event) override {
        if (hasFocus())
            QSpinBox::wheelEvent(event);
        else
            event->ignore();
    }
    QString textFromValue(int value) const override {
        return QString("%1").arg(value, 2, 10, QChar('0'));
    }
};
class FocusDial final : public QWidget {
  public:
    explicit FocusDial(Countdown &timer) : timer(timer) {
        setMinimumSize(290, 300);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setAccessibleName(QStringLiteral("倒计时剩余时间"));
    }
    Theme theme = Theme::named("forest");
    bool valid = true;
    QSize sizeHint() const override {
        return {420, 300};
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const qreal side = qMin<qreal>(qMin(width() - 22, height() - 30), 370);
        const auto center = QPointF(width() / 2.0, height() / 2.0);
        const qreal radius = side / 2 - 16;
        const QRectF circle(center.x() - radius, center.y() - radius, radius * 2, radius * 2);
        const bool done = valid && timer.state() == Countdown::State::Finished;
        const qreal fraction =
            valid ? qBound(0.0, double(timer.remaining()) / timer.duration(), 1.0) : 0;
        const bool running = timer.state() == Countdown::State::Running;
        QColor shadow = theme.accent;
        shadow.setAlpha(12);
        p.setPen(Qt::NoPen);
        p.setBrush(shadow);
        p.drawEllipse(circle.adjusted(-13, -10, 13, 16));
        QLinearGradient face(circle.topLeft(), circle.bottomRight());
        face.setColorAt(0, theme.panel);
        face.setColorAt(1, theme.soft);
        p.setBrush(face);
        p.drawEllipse(circle.adjusted(12, 12, -12, -12));
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(theme.line, 1));
        p.drawEllipse(circle.adjusted(20, 20, -20, -20));
        for (int i = 0; i < 60; ++i) {
            const double a = (i * 6 - 90) * 3.141592653589793 / 180;
            const bool major = i % 5 == 0;
            const qreal outer = radius + 12, inner = outer - (major ? 6 : 2);
            QColor ink = major ? theme.accent : theme.muted;
            ink.setAlpha(major ? 150 : 65);
            p.setPen(QPen(ink, major ? 1.5 : 1, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(center + QPointF(std::cos(a) * inner, std::sin(a) * inner),
                       center + QPointF(std::cos(a) * outer, std::sin(a) * outer));
        }
        p.setPen(QPen(theme.line, 7, Qt::SolidLine, Qt::RoundCap));
        p.drawEllipse(circle);
        QConicalGradient progress(center, 90);
        QColor faint = theme.accent;
        faint.setAlpha(90);
        progress.setColorAt(0, theme.accent);
        progress.setColorAt(1, faint);
        p.setPen(QPen(QBrush(progress), 7, Qt::SolidLine, Qt::RoundCap));
        if (done || fraction >= .99999)
            p.drawEllipse(circle);
        else if (fraction > 0)
            p.drawArc(circle, 90 * 16, -qRound(fraction * 360 * 16));
        const double angle = (-90 + 360 * (done ? 1 : fraction)) * 3.141592653589793 / 180;
        const QPointF endpoint =
            center + QPointF(std::cos(angle) * radius, std::sin(angle) * radius);
        QColor glow = theme.accent;
        glow.setAlpha(30);
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(endpoint, 11, 11);
        p.setBrush(theme.accent);
        p.drawEllipse(endpoint, 5, 5);
        p.setBrush(theme.panel);
        p.drawEllipse(endpoint, 2, 2);
        auto draw = [&](const QString &text, int size, QColor color, qreal y, bool bold = false) {
            QFont font("Microsoft YaHei UI");
            font.setPixelSize(size);
            font.setBold(bold);
            p.setFont(font);
            p.setPen(color);
            p.drawText(QRectF(center.x() - radius + 14, y, radius * 2 - 28, size + 14),
                       Qt::AlignCenter, text);
        };
        draw(done ? QStringLiteral("本次专注已完成") : QStringLiteral("此刻，只专注一件事"), 12,
             theme.muted, center.y() - 68);
        QFont digits("Segoe UI");
        digits.setWeight(QFont::DemiBold);
        digits.setPixelSize(timer.duration() >= 3600000 ? qRound(side * .155) : qRound(side * .22));
        digits.setLetterSpacing(QFont::AbsoluteSpacing, -1);
        p.setFont(digits);
        p.setPen(theme.text);
        p.drawText(QRectF(center.x() - radius + 8, center.y() - 38, radius * 2 - 16, 82),
                   Qt::AlignCenter, timeText(valid ? timer.remaining() : 0));
        const auto percent = qRound((1 - fraction) * 100);
        draw(done                                        ? QStringLiteral("休息一下，再从容出发")
             : running                                   ? QString("已完成 %1%").arg(percent)
             : timer.state() == Countdown::State::Paused ? QStringLiteral("暂歇片刻，随时继续")
                                                         : QStringLiteral("准备好，慢慢开始"),
             11, theme.accent, center.y() + 48);
    }

  private:
    Countdown &timer;
};
FocusClock::FocusClock(QSettings &preferences, QWidget *parent)
    : QWidget(parent), timer(this), settings(preferences) {
    setObjectName("focusClock");
    alarm = new FocusAlarm(this);
    alarm->setObjectName("focusAlarm");
    music = std::make_unique<MusicLibrary>(QFileInfo(settings.fileName()).absolutePath());
    auto layout = column(this, 0);
    layout->setSpacing(16);
    alarmBanner = new QFrame;
    alarmBanner->setObjectName("focusAlarmBanner");
    auto bannerLayout = new QHBoxLayout(alarmBanner);
    bannerLayout->setContentsMargins(18, 12, 18, 12);
    auto alarmText = label("专注已完成，铃声正在提醒你。", "sectionTitle");
    bannerLayout->addWidget(alarmText, 1);
    stopAlarmButton = button("停止响铃", "clockStopAlarm");
    bannerLayout->addWidget(stopAlarmButton);
    connect(stopAlarmButton, &QPushButton::clicked, this, &FocusClock::stopAlarm);
    layout->addWidget(alarmBanner);
    alarmBanner->hide();
    auto panel = card();
    auto outer = column(panel, 20);
    outer->setSpacing(12);
    auto heading = new QHBoxLayout;
    auto headingText = new QVBoxLayout;
    headingText->setSpacing(6);
    headingText->addWidget(label("专注空间", "sectionTitle"));
    heading->addLayout(headingText, 1);
    status = label("准备就绪", "pill");
    status->setObjectName("clockStatus");
    heading->addWidget(status, 0, Qt::AlignTop);
    outer->addLayout(heading);
    auto body = new QHBoxLayout;
    body->setSpacing(28);
    dial = new FocusDial(timer);
    dial->setObjectName("focusDial");
    body->addWidget(dial, 1);
    auto controls = new QWidget;
    controls->setObjectName("clockControls");
    controls->setFixedWidth(260);
    auto controlLayout = column(controls, 16);
    controlLayout->setSpacing(12);
    controlLayout->addStretch();
    controlLayout->addWidget(label("设定你的专注时间", "sectionTitle"));
    auto quick = new QHBoxLayout;
    quick->setSpacing(6);
    presets = new QButtonGroup(this);
    presets->setExclusive(true);
    for (int value : {15, 25, 45, 60}) {
        auto preset = button(QString::number(value) + " 分", "clockPreset");
        preset->setCheckable(true);
        preset->setAccessibleName(QString("专注 %1 分钟").arg(value));
        presets->addButton(preset, value);
        quick->addWidget(preset);
        connect(preset, &QPushButton::clicked, this, [this, value] {
            setSeconds(value * 60);
            configure();
        });
    }
    controlLayout->addLayout(quick);
    auto timeInputs = new QHBoxLayout;
    timeInputs->setSpacing(8);
    auto input = [&](const QString &name, const QString &unit, int maximum) {
        auto box = new DurationField;
        box->setObjectName(name);
        box->setRange(0, maximum);
        box->setButtonSymbols(QAbstractSpinBox::NoButtons);
        box->setAlignment(Qt::AlignCenter);
        box->setFixedHeight(54);
        box->setMinimumWidth(0);
        box->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        box->setAccessibleName(unit);
        box->setKeyboardTracking(false);
        auto cell = new QVBoxLayout;
        cell->setSpacing(7);
        cell->addWidget(box);
        auto caption = label(unit, "smallMuted");
        caption->setAlignment(Qt::AlignCenter);
        cell->addWidget(caption);
        timeInputs->addLayout(cell, 1);
        connect(box, &QSpinBox::valueChanged, this, [this] { configure(); });
        return box;
    };
    hours = input("clockHours", "小时", 23);
    minutes = input("clockMinutes", "分钟", 59);
    seconds = input("clockSeconds", "秒钟", 59);
    controlLayout->addLayout(timeInputs);
    startButton = button("开始专注", "clockStart");
    startButton->setMinimumHeight(44);
    auto actions = new QHBoxLayout;
    actions->setSpacing(8);
    actions->addWidget(startButton, 1);
    resetButton = button("重置计时", "clockReset");
    resetButton->setFixedWidth(76);
    actions->addWidget(resetButton);
    controlLayout->addLayout(actions);
    finishTime = label("", "smallMuted");
    finishTime->setWordWrap(true);
    finishTime->setObjectName("clockFinishTime");
    controlLayout->addWidget(finishTime);
    controlLayout->addStretch();
    body->addWidget(controls);
    outer->addLayout(body, 1);
    layout->addWidget(panel, 1);
    auto audioCard = card();
    auto audioLayout = column(audioCard, 18);
    audioLayout->setSpacing(12);
    auto audioHeader = new QHBoxLayout;
    audioHeader->addWidget(label("完成提醒", "sectionTitle"), 1);
    sound = new ToneSwitch("声音提醒");
    sound->setObjectName("clockSound");
    sound->setChecked(settings.value("focusSound", true).toBool());
    loopSound = new ToneSwitch("持续响铃");
    loopSound->setObjectName("clockLoopSound");
    loopSound->setToolTip("开启后循环播放，直到点击停止响铃；关闭时播放一遍。");
    loopSound->setChecked(settings.value("focusLoopSound", false).toBool());
    audioHeader->addWidget(sound);
    audioHeader->addSpacing(12);
    audioHeader->addWidget(loopSound);
    audioLayout->addLayout(audioHeader);
    auto audioRow = new QHBoxLayout;
    audioRow->setSpacing(10);
    audioChoice = new ChoiceBox;
    audioChoice->setObjectName("clockAudioChoice");
    audioRow->addWidget(audioChoice, 1);
    chooseAudioButton = button("添加音乐…", "clockChooseAudio");
    previewButton = button("试听", "clockPreview");
    audioRow->addWidget(chooseAudioButton);
    audioRow->addWidget(previewButton);
    volume = new QSlider(Qt::Horizontal);
    volume->setObjectName("clockVolume");
    volume->setRange(0, 100);
    volume->setValue(qBound(0, settings.value("focusVolume", 70).toInt(), 100));
    volume->setFixedWidth(90);
    volume->setAccessibleName("提醒音量");
    volumeCaption = label("", "smallMuted");
    audioRow->addWidget(volume);
    audioRow->addWidget(volumeCaption);
    audioLayout->addLayout(audioRow);
    audioMessage = label("", "clockAudioMessage");
    audioMessage->setWordWrap(true);
    audioMessage->hide();
    audioLayout->addWidget(audioMessage);
    auto audioNote = label("单次或循环，由你选择。收起到托盘后依然提醒。", "smallMuted");
    audioNote->setWordWrap(true);
    audioLayout->addWidget(audioNote);
    auto musicActions = new QHBoxLayout;
    auto musicHint = label("直接放入 music 文件夹的音乐，刷新后即可选择。", "smallMuted");
    musicHint->setWordWrap(true);
    musicActions->addWidget(musicHint, 1);
    auto openMusic = button("打开音乐文件夹", "clockMusicFolder");
    auto reload = button("刷新列表", "clockMusicReload");
    musicActions->addWidget(openMusic);
    musicActions->addWidget(reload);
    audioLayout->addLayout(musicActions);
    connect(openMusic, &QPushButton::clicked, this, [this] {
        QString error;
        if (!music->ensurePresets(&error) ||
            !QDesktopServices::openUrl(QUrl::fromLocalFile(music->directory()))) {
            audioMessage->setText(error.isEmpty() ? "无法打开音乐文件夹。" : error);
            audioMessage->show();
        }
    });
    connect(reload, &QPushButton::clicked, this, &FocusClock::reloadMusic);
    reloadMusic();
    layout->addWidget(audioCard);
    connect(audioChoice, &QComboBox::currentIndexChanged, this, [this] {
        alarm->stop();
        settings.setValue("focusAudio", audioChoice->currentData());
        settings.setValue("focusAudioFile", audioChoice->currentData());
        audioChoice->setToolTip(music->source(audioChoice->currentData().toString()).toLocalFile());
        audioMessage->hide();
        refreshAudio();
    });
    connect(chooseAudioButton, &QPushButton::clicked, this, [this] {
        const auto files = QFileDialog::getOpenFileNames(
            this, "添加喜欢的音乐", QDir::homePath(), "音频文件 (*.wav *.mp3 *.flac *.m4a *.ogg)");
        if (files.isEmpty())
            return;
        QStringList errors;
        int added = 0;
        for (const auto &file : files) {
            QString error;
            if (importAudio(file, &error))
                ++added;
            else
                errors.append(QFileInfo(file).fileName() + "：" + error);
        }
        audioMessage->setText(errors.isEmpty()
                                  ? QString("已添加 %1 首音乐，可随时切换试听。").arg(added)
                                  : QString("已添加 %1 首。\n").arg(added) + errors.join('\n'));
        audioMessage->show();
    });
    connect(previewButton, &QPushButton::clicked, this, [this] {
        if (alarm->active()) {
            stopAlarm();
            return;
        }
        audioMessage->hide();
        alarm->play(selectedAudio(), false, true);
    });
    connect(loopSound, &QCheckBox::toggled, this, [this](bool value) {
        settings.setValue("focusLoopSound", value);
        if (ringing()) {
            alarm->stop();
            alarm->play(selectedAudio(), value);
        }
    });
    connect(volume, &QSlider::valueChanged, this, [this](int value) {
        settings.setValue("focusVolume", value);
        refreshAudio();
    });
    connect(alarm, &FocusAlarm::failed, this, [this](const QString &error) {
        audioMessage->setText(error);
        audioMessage->show();
    });
    connect(alarm, &FocusAlarm::activeChanged, this, [this] {
        alarmBanner->setVisible(ringing());
        emit alarmActiveChanged(ringing());
        refreshAudio();
    });
    connect(startButton, &QPushButton::clicked, this, [this] {
        if (timer.state() != Countdown::State::Running && timer.state() != Countdown::State::Paused)
            alarm->stop();
        if (timer.state() == Countdown::State::Running)
            timer.pause();
        else
            timer.start();
    });
    connect(resetButton, &QPushButton::clicked, this, [this] {
        alarm->stop();
        timer.reset();
    });
    connect(&timer, &Countdown::changed, this, &FocusClock::refresh);
    connect(&timer, &Countdown::finished, this, [this] {
        audioMessage->hide();
        if (sound->isChecked() && volume->value() > 0)
            alarm->play(selectedAudio(), loopSound->isChecked());
        emit completed(sound->isChecked());
    });
    connect(sound, &QCheckBox::toggled, this, [this](bool enabled) {
        settings.setValue("focusSound", enabled);
        if (!enabled)
            alarm->stop();
        refreshAudio();
    });
    setSeconds(qBound(1, settings.value("focusSeconds", 1500).toInt(), 86399));
    configure();
    refreshAudio();
}
void FocusClock::setSeconds(int value) {
    const QSignalBlocker h(hours), m(minutes), s(seconds);
    hours->setValue(value / 3600);
    minutes->setValue(value / 60 % 60);
    seconds->setValue(value % 60);
}
void FocusClock::configure() {
    alarm->stop();
    const int value = hours->value() * 3600 + minutes->value() * 60 + seconds->value();
    if (value > 0 && timer.setDuration(qint64(value) * 1000))
        settings.setValue("focusSeconds", value);
    if (value == 0)
        timer.reset();
    presets->setExclusive(false);
    for (auto preset : presets->buttons())
        preset->setChecked(value == presets->id(preset) * 60);
    presets->setExclusive(true);
    refresh();
}
void FocusClock::refresh() {
    const auto state = timer.state();
    const bool active = state == Countdown::State::Running || state == Countdown::State::Paused;
    for (auto input : {hours, minutes, seconds})
        input->setEnabled(!active);
    for (auto preset : presets->buttons())
        preset->setEnabled(!active);
    const bool valid = hours->value() * 3600 + minutes->value() * 60 + seconds->value() > 0;
    startButton->setEnabled(valid);
    resetButton->setEnabled(state != Countdown::State::Ready);
    startButton->setText(state == Countdown::State::Running    ? "暂停计时"
                         : state == Countdown::State::Paused   ? "继续专注"
                         : state == Countdown::State::Finished ? "再来一次"
                                                               : "开始专注");
    status->setText(state == Countdown::State::Running    ? "正在专注"
                    : state == Countdown::State::Paused   ? "已暂停"
                    : state == Countdown::State::Finished ? "专注完成"
                                                          : "准备就绪");
    if (!valid)
        finishTime->setText("请设置大于 0 的时长");
    else if (state == Countdown::State::Running)
        finishTime->setText(
            "预计结束于  " +
            QDateTime::currentDateTime().addMSecs(timer.remaining()).toString("HH:mm:ss"));
    else if (state == Countdown::State::Paused)
        finishTime->setText("时间已暂停，按「继续专注」接着来。");
    else if (state == Countdown::State::Finished)
        finishTime->setText("做得很好，起身活动一下吧。");
    else
        finishTime->setText("支持自定义时、分、秒。");
    dial->valid = valid;
    dial->setAccessibleDescription(timeText(valid ? timer.remaining() : 0));
    dial->update();
}
void FocusClock::setTheme(const Theme &theme) {
    audioChoice->setTheme(theme);
    for (auto toggle : {sound, loopSound}) {
        static_cast<ToneSwitch *>(toggle)->theme = theme;
        toggle->update();
    }
    dial->theme = theme;
    dial->update();
}
bool FocusClock::ringing() const {
    return alarm->active() && !alarm->previewing();
}
void FocusClock::stopAlarm() {
    alarm->stop();
}
QUrl FocusClock::selectedAudio() const {
    const auto url = music->source(audioChoice->currentData().toString());
    return url.isEmpty() ? FocusAlarm::preset("chime") : url;
}
void FocusClock::reloadMusic() {
    QString error;
    music->ensurePresets(&error);
    QString selection = settings.value("focusAudio", "chime").toString();
    if (selection == "chime" || selection == "soft" || selection == "bell")
        selection = "music/" + selection + ".wav";
    if (selection == "custom") {
        const auto old = settings.value("focusAudioFile").toString();
        if (QRegularExpression("^sounds/[a-f0-9-]{36}\\.(wav|mp3|flac|m4a|ogg)$")
                .match(old)
                .hasMatch()) {
            const auto added = music->add(QFileInfo(settings.fileName()).dir().filePath(old),
                                          &error, settings.value("focusAudioName").toString());
            selection = added.isEmpty() ? QString("music/chime.wav") : added;
        } else
            selection = MusicLibrary::validRelative(old) ? old : QString("music/chime.wav");
    }
    const QSignalBlocker blocker(audioChoice);
    audioChoice->clear();
    for (const auto &track : music->tracks())
        audioChoice->addItem(MusicLibrary::displayName(track), track);
    if (audioChoice->findData(selection) < 0 && MusicLibrary::validRelative(selection))
        audioChoice->addItem(MusicLibrary::displayName(selection) + "（文件已丢失）", selection);
    if (audioChoice->count() == 0)
        audioChoice->addItem("清晨铃（内置备用）", "music/chime.wav");
    audioChoice->setCurrentIndex(qMax(0, audioChoice->findData(selection)));
    settings.setValue("focusAudio", audioChoice->currentData());
    settings.setValue("focusAudioFile", audioChoice->currentData());
    audioChoice->setToolTip(music->source(audioChoice->currentData().toString()).toLocalFile());
    if (!error.isEmpty()) {
        audioMessage->setText(error);
        audioMessage->show();
    }
}
void FocusClock::refreshAudio() {
    const bool active = alarm->active();
    loopSound->setEnabled(sound->isChecked());
    previewButton->setEnabled(volume->value() > 0 || active);
    previewButton->setText(active ? (alarm->previewing() ? "停止试听" : "停止响铃") : "试听");
    volumeCaption->setText(QString("%1%").arg(volume->value()));
    alarm->setVolume(volume->value());
    if (volume->value() == 0 && active)
        alarm->stop();
}
bool FocusClock::importAudio(const QString &file, QString *error) {
    const auto relative = music->add(file, error);
    if (relative.isEmpty())
        return false;
    alarm->stop();
    const auto oldSelection = settings.value("focusAudio", "music/chime.wav");
    const auto oldFile = settings.value("focusAudioFile");
    settings.setValue("focusAudio", relative);
    settings.setValue("focusAudioFile", relative);
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        settings.setValue("focusAudio", oldSelection);
        settings.setValue("focusAudioFile", oldFile);
        settings.sync();
        if (error)
            *error = "音乐已保存，但偏好保存失败，请检查数据目录权限。";
        return false;
    }
    reloadMusic();
    audioMessage->setText("音乐已加入 music 文件夹，之前添加的音乐也会保留。");
    audioMessage->show();
    refreshAudio();
    return true;
}
