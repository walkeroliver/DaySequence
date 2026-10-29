// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include <QLabel>
#include <QPainter>

// Keep the full path available to accessibility and tooltips without enlarging the layout.
class PathLabel final : public QLabel {
  public:
    explicit PathLabel(const QString &path) : QLabel(path) {
        setObjectName("storageLocation");
        setTextFormat(Qt::PlainText);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        setMinimumWidth(0);
        setToolTip(path);
        setAccessibleName(QStringLiteral("保存文件夹"));
    }
    QSize sizeHint() const override {
        return {200, fontMetrics().height() + 8};
    }
    QSize minimumSizeHint() const override {
        return {0, fontMetrics().height() + 8};
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setFont(font());
        painter.setPen(palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled,
                                       QPalette::WindowText));
        painter.drawText(contentsRect(), Qt::AlignLeft | Qt::AlignVCenter,
                         fontMetrics().elidedText(text(), Qt::ElideMiddle, contentsRect().width()));
    }
};