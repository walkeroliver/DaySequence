// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include "planner.h"
#include "visuals.h"
#include <QAbstractListModel>
#include <QStyledItemDelegate>

class TaskListModel final : public QAbstractListModel {
    Q_OBJECT
  public:
    enum Role { IdRole = Qt::UserRole + 1, PriorityRole };
    explicit TaskListModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    void setTasks(QVector<Task> tasks);
    QModelIndex indexForId(const QString &id) const;
  signals:
    void completionRequested(const QString &id, bool done);

  private:
    QVector<Task> records;
};

class TaskDelegate final : public QStyledItemDelegate {
    Q_OBJECT
  public:
    explicit TaskDelegate(QObject *parent = nullptr);
    void setTheme(const Theme &value) {
        theme = value;
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;
  signals:
    void menuRequested(const QString &id, const QPoint &globalPosition);

  private:
    Theme theme = Theme::named("forest");
};
