// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "tasklist.h"
#include <QAbstractItemView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTextDocument>
#include <QTextLayout>

TaskListModel::TaskListModel(QObject *parent) : QAbstractListModel(parent) {}
int TaskListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : records.size();
}
QVariant TaskListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= records.size())
        return {};
    const auto &task = records[index.row()];
    switch (role) {
    case Qt::DisplayRole:
        return task.title;
    case Qt::CheckStateRole:
        return task.done ? Qt::Checked : Qt::Unchecked;
    case Qt::ToolTipRole:
        return Qt::convertFromPlainText(task.title);
    case Qt::AccessibleTextRole:
        return task.title + (task.done ? QStringLiteral("，已完成") : QStringLiteral("，未完成"));
    case IdRole:
        return task.id;
    case PriorityRole:
        return task.priority;
    default:
        return {};
    }
}
Qt::ItemFlags TaskListModel::flags(const QModelIndex &index) const {
    return index.isValid() ? Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable
                           : Qt::NoItemFlags;
}
bool TaskListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() >= records.size() || role != Qt::CheckStateRole)
        return false;
    const auto &task = records[index.row()];
    bool done = value.toInt() == Qt::Checked;
    if (done == task.done)
        return false;
    emit completionRequested(task.id, done);
    return true;
}
void TaskListModel::setTasks(QVector<Task> tasks) {
    bool sameOrder = tasks.size() == records.size();
    if (sameOrder)
        for (int i = 0; i < tasks.size(); ++i)
            if (tasks[i].id != records[i].id) {
                sameOrder = false;
                break;
            }
    if (!sameOrder) {
        beginResetModel();
        records = std::move(tasks);
        endResetModel();
        return;
    }
    for (int i = 0; i < tasks.size(); ++i) {
        const auto &a = records[i];
        const auto &b = tasks[i];
        if (a.title == b.title && a.done == b.done && a.priority == b.priority)
            continue;
        records[i] = b;
        emit dataChanged(index(i), index(i),
                         {Qt::DisplayRole, Qt::CheckStateRole, Qt::ToolTipRole,
                          Qt::AccessibleTextRole, PriorityRole});
    }
}
QModelIndex TaskListModel::indexForId(const QString &id) const {
    for (int i = 0; i < records.size(); ++i)
        if (records[i].id == id)
            return index(i);
    return {};
}

TaskDelegate::TaskDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
QSize TaskDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const {
    return {240, 88};
}
void TaskDelegate::paint(QPainter *p, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const {
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    const auto rect = option.rect.adjusted(1, 1, -1, -1);
    if (option.state & (QStyle::State_MouseOver | QStyle::State_Selected)) {
        p->setPen(Qt::NoPen);
        p->setBrush(theme.soft);
        p->drawRoundedRect(rect.adjusted(0, 2, 0, -2), 9, 9);
    }
    p->setPen(theme.line);
    p->drawLine(rect.bottomLeft(), rect.bottomRight());
    bool done = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
    QRectF check(rect.left() + 10, rect.center().y() - 10, 20, 20);
    p->setPen(QPen(done ? theme.accent : theme.muted, 1));
    p->setBrush(done ? theme.accent : theme.panel);
    p->drawRoundedRect(check, 6, 6);
    if (done) {
        p->setPen(QPen(Qt::white, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath tick;
        tick.moveTo(check.left() + 5, check.top() + 10);
        tick.lineTo(check.left() + 8, check.top() + 13);
        tick.lineTo(check.left() + 15, check.top() + 6);
        p->drawPath(tick);
    }
    QRectF textRect(rect.left() + 44, rect.top() + 12, qMax(10, rect.width() - 78), 42);
    QFont font = option.font;
    font.setPixelSize(14);
    font.setStrikeOut(done);
    p->setFont(font);
    p->setPen(done ? theme.muted : theme.text);
    QString title = index.data().toString().simplified();
    QTextLayout text(title, font);
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    text.setTextOption(textOption);
    text.beginLayout();
    qreal y = textRect.top();
    for (int lineNumber = 0; lineNumber < 2; ++lineNumber) {
        auto line = text.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(textRect.width());
        if (lineNumber == 1)
            p->drawText(QPointF(textRect.left(), y + line.ascent()),
                        QFontMetrics(font).elidedText(title.mid(line.textStart()), Qt::ElideRight,
                                                      int(textRect.width())));
        else
            line.draw(p, QPointF(textRect.left(), y));
        y += line.height();
    }
    text.endLayout();
    font.setStrikeOut(false);
    font.setPixelSize(10);
    p->setFont(font);
    int priority = index.data(TaskListModel::PriorityRole).toInt();
    p->setPen(priority == 2 ? QColor("#C57969") : priority == 1 ? theme.accent : theme.muted);
    p->drawText(QPointF(textRect.left(), rect.bottom() - 13),
                QStringList{"普通目标", "● 重要", "● 紧急"}.value(priority));
    p->setPen(Qt::NoPen);
    p->setBrush(theme.muted);
    for (int i = -1; i <= 1; ++i)
        p->drawEllipse(QPointF(rect.right() - 15 + i * 4, rect.center().y()), 1, 1);
    if (option.state & QStyle::State_HasFocus) {
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(theme.accent, 1, Qt::DotLine));
        p->drawRoundedRect(rect.adjusted(1, 3, -1, -3), 8, 8);
    }
    p->restore();
}
bool TaskDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                               const QStyleOptionViewItem &option, const QModelIndex &index) {
    if (event->type() == QEvent::MouseButtonRelease) {
        auto mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() != Qt::LeftButton)
            return false;
        if (mouse->position().x() < option.rect.left() + 38) {
            bool done = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
            return model->setData(index, done ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
        }
        if (mouse->position().x() > option.rect.right() - 32) {
            emit menuRequested(index.data(TaskListModel::IdRole).toString(),
                               mouse->globalPosition().toPoint());
            return true;
        }
    }
    if (event->type() == QEvent::KeyPress &&
        static_cast<QKeyEvent *>(event)->key() == Qt::Key_Space) {
        bool done = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
        return model->setData(index, done ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
    }
    return false;
}
