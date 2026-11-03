// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QFocusEvent>
#include <QStyle>
#include <QWidget>

// Keep keyboard focus visible without adding a ring after a mouse click.
class KeyboardFocusIndicator final : public QObject {
  public:
    explicit KeyboardFocusIndicator(QWidget *widget) : QObject(widget) {
        widget->setProperty("keyboardFocus", false);
        widget->installEventFilter(this);
    }

  protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        auto widget = qobject_cast<QWidget *>(object);
        if (!widget)
            return false;
        bool visible = widget->property("keyboardFocus").toBool();
        if (event->type() == QEvent::FocusIn) {
            const auto reason = static_cast<QFocusEvent *>(event)->reason();
            visible = reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason ||
                      reason == Qt::ShortcutFocusReason;
        } else if (event->type() == QEvent::FocusOut || event->type() == QEvent::MouseButtonPress) {
            visible = false;
        } else
            return false;
        if (visible != widget->property("keyboardFocus").toBool()) {
            widget->setProperty("keyboardFocus", visible);
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
            widget->update();
        }
        return false;
    }
};
