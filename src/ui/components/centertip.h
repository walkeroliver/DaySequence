// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include <QEvent>
#include <QLabel>
#include <QTimer>

// A non-modal, mouse-transparent message inside the active window.
class CenterTip final : public QLabel {
  public:
    explicit CenterTip(QWidget *host) : QLabel(host) {
        setObjectName("centerTip");
        setTextFormat(Qt::PlainText);
        setWordWrap(true);
        setAlignment(Qt::AlignCenter);
        setContentsMargins(18, 12, 18, 12);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFocusPolicy(Qt::NoFocus);
        hide();
        host->installEventFilter(this);
        expiry.setSingleShot(true);
        expiry.setInterval(2600);
        connect(&expiry, &QTimer::timeout, this, &QWidget::hide);
    }
    void showMessage(const QString &message) {
        setText(message);
        center();
        show();
        raise();
        expiry.start();
    }

  protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (object == parentWidget() && event->type() == QEvent::Resize && isVisible())
            center();
        return QLabel::eventFilter(object, event);
    }

  private:
    QTimer expiry;
    void center() {
        const auto host = parentWidget();
        setFixedWidth(qMax(1, qMin(360, host->width() - 48)));
        setFixedHeight(qMax(64, heightForWidth(width())));
        move((host->width() - width()) / 2, (host->height() - height()) / 2);
    }
};