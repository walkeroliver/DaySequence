// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

inline QLabel *label(const QString &text, const QString &name = {}) {
    auto widget = new QLabel(text);
    widget->setTextFormat(Qt::PlainText);
    widget->setObjectName(name);
    return widget;
}
inline QPushButton *button(const QString &text, const QString &name = {}) {
    auto widget = new QPushButton(text);
    widget->setObjectName(name);
    widget->setCursor(Qt::PointingHandCursor);
    return widget;
}
inline QFrame *card() {
    auto widget = new QFrame;
    widget->setObjectName("card");
    return widget;
}
inline QVBoxLayout *column(QWidget *widget, int margin = 24) {
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(16);
    return layout;
}

inline QScrollArea *scrollPage(QWidget *page) {
    auto scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(page);
    page->setAutoFillBackground(false);
    scroll->viewport()->setAutoFillBackground(false);
    return scroll;
}