// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "planner.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
bool integer(const QJsonValue &value, int minimum, int maximum) {
    if (!value.isDouble())
        return false;
    double number = value.toDouble();
    return std::isfinite(number) && number == std::floor(number) && number >= minimum &&
           number <= maximum;
}
bool validTasks(const QVector<Task> &tasks, QString &error) {
    QSet<QString> ids;
    ids.reserve(tasks.size());
    for (const auto &task : tasks) {
        if (task.id.isEmpty() || !task.date.isValid() || task.title.trimmed().isEmpty() ||
            task.priority < 0 || task.priority > 2 || task.order < 0) {
            error = QStringLiteral("计划文件包含无效记录，原文件已保留。");
            return false;
        }
        if (ids.contains(task.id)) {
            error = QStringLiteral("计划文件存在重复编号，原文件已保留。");
            return false;
        }
        ids.insert(task.id);
    }
    return true;
}
QString csvCell(QString text) {
    if (!text.isEmpty() && QStringLiteral("=+-@\t\r\n").contains(text.front()))
        text.prepend('\'');
    text.replace('"', "\"\"");
    return '"' + text + '"';
}
} // namespace
int MonthStats::rate() const {
    return total ? qRound(done * 100.0 / total) : 0;
}
Planner::Planner(QString file) : path(std::move(file)) {}
bool Planner::load() {
    error.clear();
    QFile file(path);
    if (!file.exists()) {
        tasks.clear();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        error = file.errorString();
        return false;
    }
    QJsonParseError parse;
    auto doc = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError || !doc.isObject()) {
        error = QStringLiteral("计划文件不是有效的 JSON，原文件已保留。");
        return false;
    }
    auto root = doc.object();
    if (!integer(root["version"], 2, 2)) {
        error = QStringLiteral("不支持此数据版本，请使用匹配的软件版本打开。原文件已保留。");
        return false;
    }
    if (!root["tasks"].isArray()) {
        error = QStringLiteral("计划列表格式不正确，原文件已保留。");
        return false;
    }
    auto values = root["tasks"].toArray();
    QVector<Task> loaded;
    loaded.reserve(values.size());
    for (const auto &value : values) {
        if (!value.isObject()) {
            error = QStringLiteral("计划记录格式不正确，原文件已保留。");
            return false;
        }
        auto object = value.toObject();
        if (!object["id"].isString() || !object["title"].isString() || !object["date"].isString() ||
            !object["done"].isBool() || !integer(object["priority"], 0, 2) ||
            !integer(object["order"], 0, std::numeric_limits<int>::max())) {
            error = QStringLiteral("计划记录中的字段类型不正确，原文件已保留。");
            return false;
        }
        auto dateText = object["date"].toString();
        QDate date = QDate::fromString(dateText, Qt::ISODate);
        if (date.toString(Qt::ISODate) != dateText) {
            error = QStringLiteral("计划日期格式不正确，原文件已保留。");
            return false;
        }
        loaded.append({object["id"].toString(), object["title"].toString(), date,
                       object["done"].toBool(), object["priority"].toInt(),
                       object["order"].toInt()});
    }
    if (!validTasks(loaded, error))
        return false;
    tasks = std::move(loaded);
    return true;
}
bool Planner::save() {
    error.clear();
    if (!validTasks(tasks, error))
        return false;
    QJsonArray array;
    for (const auto &task : tasks)
        array.append(QJsonObject{{"id", task.id},
                                 {"title", task.title},
                                 {"date", task.date.toString(Qt::ISODate)},
                                 {"done", task.done},
                                 {"priority", task.priority},
                                 {"order", task.order}});
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        error = QStringLiteral("无法创建数据目录。");
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        error = file.errorString();
        return false;
    }
    auto bytes = QJsonDocument(QJsonObject{{"version", 2}, {"tasks", array}}).toJson();
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        error = file.errorString();
        return false;
    }
    return true;
}
QString Planner::add(const QDate &date, const QString &title, int priority) {
    if (!date.isValid() || title.trimmed().isEmpty())
        return {};
    int order = -1;
    for (const auto &entry : tasks)
        if (entry.date == date)
            order = qMax(order, entry.order);
    if (order == std::numeric_limits<int>::max()) {
        const auto ordered = forDate(date);
        QHash<QString, int> ranks;
        for (int i = 0; i < ordered.size(); ++i)
            ranks.insert(ordered[i].id, i);
        for (auto &entry : tasks)
            if (ranks.contains(entry.id))
                entry.order = ranks.value(entry.id);
        order = ordered.size();
    } else
        ++order;
    auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    tasks.append({id, title.trimmed().left(200), date, false, qBound(0, priority, 2), order});
    return id;
}
QVector<Task> Planner::forDate(const QDate &date, int sort, const QString &filter) const {
    QVector<Task> result;
    for (const auto &task : tasks)
        if (task.date == date && task.title.contains(filter, Qt::CaseInsensitive))
            result.append(task);
    std::stable_sort(result.begin(), result.end(), [sort](const Task &a, const Task &b) {
        if (sort == 1 && a.done != b.done)
            return !a.done;
        if (sort == 2 && a.priority != b.priority)
            return a.priority > b.priority;
        if (sort == 3) {
            int comparison = QString::localeAwareCompare(a.title, b.title);
            if (comparison != 0)
                return comparison < 0;
        }
        return a.order < b.order;
    });
    return result;
}
MonthStats Planner::stats(const QDate &month) const {
    MonthStats result;
    if (!month.isValid())
        return result;
    result.dailyTotal.fill(0, month.daysInMonth());
    result.dailyDone.fill(0, month.daysInMonth());
    for (const auto &task : tasks)
        if (task.date.isValid() && task.date.year() == month.year() &&
            task.date.month() == month.month()) {
            ++result.total;
            ++result.dailyTotal[task.date.day() - 1];
            if (task.done) {
                ++result.done;
                ++result.dailyDone[task.date.day() - 1];
            }
        }
    for (int i = 0; i < result.dailyTotal.size(); ++i)
        if (result.dailyTotal[i]) {
            ++result.plannedDays;
            if (result.dailyTotal[i] == result.dailyDone[i])
                ++result.completeDays;
        }
    return result;
}
Task *Planner::find(const QString &id) {
    for (auto &task : tasks)
        if (task.id == id)
            return &task;
    return nullptr;
}
void Planner::remove(const QString &id) {
    tasks.erase(
        std::remove_if(tasks.begin(), tasks.end(), [&](const Task &task) { return task.id == id; }),
        tasks.end());
}
void Planner::move(const QString &id, int delta) {
    auto task = find(id);
    if (!task)
        return;
    auto ordered = forDate(task->date);
    int row = -1;
    for (int i = 0; i < ordered.size(); ++i)
        if (ordered[i].id == id) {
            row = i;
            break;
        }
    const qint64 other = qint64(row) + delta;
    if (row < 0 || other < 0 || other >= ordered.size())
        return;
    QHash<QString, int> rank;
    for (int i = 0; i < ordered.size(); ++i)
        rank.insert(ordered[i].id, i);
    rank[id] = int(other);
    rank[ordered[int(other)].id] = row;
    for (auto &entry : tasks)
        if (rank.contains(entry.id))
            entry.order = rank.value(entry.id);
}
QByteArray Planner::monthCsv(const QDate &month) const {
    auto ordered = tasks;
    std::stable_sort(ordered.begin(), ordered.end(), [](const Task &a, const Task &b) {
        return a.date == b.date ? a.order < b.order : a.date < b.date;
    });
    QString text = QStringLiteral("日期,目标,状态,优先级\r\n");
    for (const auto &task : ordered)
        if (task.date.year() == month.year() && task.date.month() == month.month())
            text += QStringLiteral("%1,%2,%3,%4\r\n")
                        .arg(task.date.toString(Qt::ISODate), csvCell(task.title),
                             task.done ? QStringLiteral("已完成") : QStringLiteral("未完成"),
                             QStringList{"普通", "重要", "紧急"}.value(task.priority));
    return QByteArray::fromHex("efbbbf") + text.toUtf8();
}

void Planner::reschedule(const QString &id, const QDate &date) {
    auto task = find(id);
    if (!task || !date.isValid() || task->date == date)
        return;
    auto ordered = forDate(date);
    QHash<QString, int> ranks;
    for (int i = 0; i < ordered.size(); ++i)
        ranks.insert(ordered[i].id, i);
    for (auto &entry : tasks)
        if (ranks.contains(entry.id))
            entry.order = ranks.value(entry.id);
    task->date = date;
    task->order = ordered.size();
}