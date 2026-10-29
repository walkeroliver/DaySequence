// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include "visuals.h"
#include <QComboBox>
class QFrame;
class QListView;
class ChoiceDelegate;
class ChoiceBox final : public QComboBox {
    Q_OBJECT
  public:
    explicit ChoiceBox(QWidget *parent = nullptr);
    void showPopup() override;
    void hidePopup() override;
    void setTheme(const Theme &theme);
    QSize sizeHint() const override;

  protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;

  private:
    void activate(const QModelIndex &index);
    Theme theme;
    QFrame *popup;
    QListView *list;
    ChoiceDelegate *delegate;
};
