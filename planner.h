// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include <QDate>

#include <QString>
#include <QVector>

struct Task {
    QString id, title;
    QDate date;
    bool done = false;
    int priority = 0;
    int order = 0;
};
struct MonthStats {
    int total = 0, done = 0, plannedDays = 0, completeDays = 0;
    QVector<int> dailyTotal, dailyDone;
    int rate() const;
};
class Planner {
  public:
    explicit Planner(QString file);
    bool load();
    bool save();
    QString add(const QDate &date, const QString &title, int priority);
    QVector<Task> forDate(const QDate &date, int sort = 0, const QString &filter = {}) const;
    MonthStats stats(const QDate &month) const;
    QByteArray monthCsv(const QDate &month) const;
    Task *find(const QString &id);
    void remove(const QString &id);
    void move(const QString &id, int delta);
    void reschedule(const QString &id, const QDate &date);
    QVector<Task> tasks;
    QString error;
    QString filePath() const {
        return path;
    }

  private:
    QString path;
};
