// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "focusclock.h"
#include "uicomponents.h"
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
        QRadialGradient halo(center, radius);
        QColor glow = theme.soft;
        glow.setAlpha(170);
        halo.setColorAt(0, glow);
        halo.setColorAt(0.75, theme.panel);
        halo.setColorAt(1, theme.panel);
        p.setPen(Qt::NoPen);
        p.setBrush(halo);
        p.drawEllipse(circle.adjusted(10, 10, -10, -10));
        for (int i = 0; i < 60; ++i) {
            const double angle = (i * 6 - 90) * 3.141592653589793 / 180;
            const qreal outer = radius + 13, inner = outer - (i % 5 == 0 ? 7 : 3);
            p.setPen(QPen(i % 5 == 0 ? theme.muted : theme.line, i % 5 == 0 ? 1.2 : 1));
            p.drawLine(center + QPointF(std::cos(angle) * inner, std::sin(angle) * inner),
                       center + QPointF(std::cos(angle) * outer, std::sin(angle) * outer));
        }
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(theme.soft, 5));
        p.drawEllipse(circle);
        p.setPen(QPen(theme.accent, 5, Qt::SolidLine, Qt::RoundCap));
        if (done)
            p.drawEllipse(circle);
        else if (fraction >= 0.99999)
            p.drawEllipse(circle);
        else if (fraction > 0)
            p.drawArc(circle, 90 * 16, -qRound(fraction * 360 * 16));
        const double angle = (-90 + 360 * (done ? 1 : fraction)) * 3.141592653589793 / 180;
        const QPointF endpoint =
            center + QPointF(std::cos(angle) * radius, std::sin(angle) * radius);
        QColor endpointGlow = theme.accent;
        endpointGlow.setAlpha(25);
        p.setPen(Qt::NoPen);
        p.setBrush(endpointGlow);
        p.drawEllipse(endpoint, 10, 10);
        p.setBrush(theme.accent);
        p.drawEllipse(endpoint, 4, 4);
        auto draw = [&](const QString &text, int size, QColor color, qreal y, bool bold = false) {
            QFont font("Microsoft YaHei UI");
            font.setPixelSize(size);
            font.setBold(bold);
            p.setFont(font);
            p.setPen(color);
            p.drawText(QRectF(center.x() - radius + 12, y, radius * 2 - 24, size + 12),
                       Qt::AlignCenter, text);
        };
        draw(done ? QStringLiteral("此刻，值得一个停顿") : QStringLiteral("一段时间 · 一件事情"),
             11, theme.muted, center.y() - 69);
        QFont digits("Segoe UI");
        digits.setWeight(QFont::Light);
        digits.setPixelSize(timer.duration() >= 3600000 ? qRound(side * 0.16)
                                                        : qRound(side * 0.22));
        p.setFont(digits);
        p.setPen(theme.text);
        p.drawText(QRectF(center.x() - radius + 8, center.y() - 36, radius * 2 - 16, 80),
                   Qt::AlignCenter, timeText(valid ? timer.remaining() : 0));
        draw(done ? QStringLiteral("本次专注已完成")
                  : (timer.state() == Countdown::State::Paused
                         ? QStringLiteral("暂歇片刻，随时继续")
                         : QStringLiteral("让注意力，回到当下")),
             11, theme.accent, center.y() + 49);
    }

  private:
    Countdown &timer;
};
FocusClock::FocusClock(QSettings &preferences, QWidget *parent)
    : QWidget(parent), timer(this), settings(preferences) {
    setObjectName("focusClock");
    auto layout = column(this, 0);
    layout->setSpacing(16);
    auto panel = card();
    auto outer = column(panel, 20);
    outer->setSpacing(12);
    auto heading = new QHBoxLayout;
    auto headingText = new QVBoxLayout;
    headingText->setSpacing(6);
    headingText->addWidget(label("专注时刻", "sectionTitle"));
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
    controls->setFixedWidth(244);
    auto controlLayout = column(controls, 0);
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
    sound = new QCheckBox("结束时播放提示音");
    sound->setObjectName("clockSound");
    sound->setChecked(settings.value("focusSound", true).toBool());
    controlLayout->addWidget(sound);
    finishTime = label("", "smallMuted");
    finishTime->setWordWrap(true);
    finishTime->setObjectName("clockFinishTime");
    controlLayout->addWidget(finishTime);
    controlLayout->addStretch();
    body->addWidget(controls);
    outer->addLayout(body, 1);
    auto rule = new QFrame;
    rule->setObjectName("divider");
    rule->setFixedHeight(1);
    outer->addWidget(rule);
    auto note = label("收起到托盘，专注仍在继续。退出程序将结束本次计时。", "smallMuted");
    note->setWordWrap(true);
    outer->addWidget(note);
    layout->addWidget(panel, 1);
    connect(startButton, &QPushButton::clicked, this, [this] {
        if (timer.state() == Countdown::State::Running)
            timer.pause();
        else
            timer.start();
    });
    connect(resetButton, &QPushButton::clicked, &timer, &Countdown::reset);
    connect(&timer, &Countdown::changed, this, &FocusClock::refresh);
    connect(&timer, &Countdown::finished, this, [this] { emit completed(sound->isChecked()); });
    connect(sound, &QCheckBox::toggled, this,
            [this](bool enabled) { settings.setValue("focusSound", enabled); });
    setSeconds(qBound(1, settings.value("focusSeconds", 1500).toInt(), 86399));
    configure();
}
void FocusClock::setSeconds(int value) {
    const QSignalBlocker h(hours), m(minutes), s(seconds);
    hours->setValue(value / 3600);
    minutes->setValue(value / 60 % 60);
    seconds->setValue(value % 60);
}
void FocusClock::configure() {
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
    dial->theme = theme;
    dial->update();
}