// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "planner.h"
#include "weatherdata.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>
#include <limits>

class PlannerTests : public QObject {
    Q_OBJECT
  private slots:
    void roundTrip() {
        QTemporaryDir dir(QDir::currentPath() + "/test-XXXXXX");
        Planner p(dir.filePath("plans.json"));
        auto id = p.add(QDate(2026, 10, 29), "  设计方案  ", 2);
        p.find(id)->done = true;
        QVERIFY(p.save());
        Planner loaded(p.filePath());
        QVERIFY(loaded.load());
        QCOMPARE(loaded.tasks.size(), 1);
        QCOMPARE(loaded.tasks[0].title, QString("设计方案"));
        QVERIFY(loaded.tasks[0].done);
        QCOMPARE(loaded.tasks[0].priority, 2);
    }
    void monthStats() {
        Planner p("unused");
        auto a = p.add(QDate(2026, 10, 1), "A", 0);
        p.find(a)->done = true;
        p.add(QDate(2026, 10, 1), "B", 0);
        auto b = p.add(QDate(2026, 10, 2), "C", 0);
        p.find(b)->done = true;
        p.add(QDate(2026, 11, 1), "other month", 0);
        p.add(QDate(2025, 10, 1), "other year", 0);
        auto s = p.stats(QDate(2026, 10, 1));
        QCOMPARE(s.total, 3);
        QCOMPARE(s.done, 2);
        QCOMPARE(s.plannedDays, 2);
        QCOMPARE(s.completeDays, 1);
        QCOMPARE(s.rate(), 67);
        QCOMPARE(s.dailyDone[0], 1);
        QCOMPARE(s.dailyTotal[0], 2);
    }
    void leapAndEmpty() {
        Planner p("unused");
        auto s = p.stats(QDate(2028, 2, 1));
        QCOMPARE(s.dailyTotal.size(), 29);
        QCOMPARE(s.rate(), 0);
        p.add(QDate(2028, 2, 29), "leap", 0);
        QCOMPARE(p.stats(QDate(2028, 2, 1)).total, 1);
    }
    void sortSearchReorder() {
        Planner p("unused");
        QDate date(2026, 10, 1);
        auto a = p.add(date, "alpha", 0);
        auto b = p.add(date, "beta", 2);
        p.find(a)->done = true;
        QCOMPARE(p.forDate(date, 1).first().id, b);
        QCOMPARE(p.forDate(date, 2).first().id, b);
        QCOMPARE(p.forDate(date, 0, "ALP").size(), 1);
        p.move(b, -1);
        QCOMPARE(p.forDate(date).first().id, b);
        p.move(b, -1);
        QCOMPARE(p.forDate(date).first().id, b);
        p.remove(a);
        QCOMPARE(p.tasks.size(), 1);
    }
    void damagedFile() {
        QTemporaryDir dir(QDir::currentPath() + "/test-XXXXXX");
        QFile f(dir.filePath("plans.json"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("broken");
        f.close();
        Planner p(f.fileName());
        QVERIFY(!p.load());
        QVERIFY(!p.error.isEmpty());
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArray("broken"));
    }
    void invalidInputs() {
        Planner p("unused");
        QVERIFY(p.add(QDate(), "A", 0).isEmpty());
        QVERIFY(p.add(QDate::currentDate(), "   ", 0).isEmpty());
        QVERIFY(p.tasks.isEmpty());
    }
    void invalidDataNeverReplacesExistingTasks() {
        QTemporaryDir dir(QDir::currentPath() + "/invalid-XXXXXX");
        QString file = dir.filePath("plans.json");
        Planner p(file);
        auto id = p.add(QDate::currentDate(), "retained", 0);
        QVERIFY(p.save());
        auto write = [&](const QByteArray &bytes) {
            QFile f(file);
            QVERIFY(f.open(QIODevice::WriteOnly));
            QCOMPARE(f.write(bytes), bytes.size());
        };
        write(R"({"version":3,"tasks":[]})");
        QVERIFY(!p.load());
        QCOMPARE(p.tasks[0].id, id);
        write(
            R"({"version":2,"tasks":[{"id":"x","title":"x","date":"2026-10-29","done":"true","priority":0,"order":0}]})");
        QVERIFY(!p.load());
        QCOMPARE(p.tasks[0].id, id);
        QJsonObject row{{"id", "x"},     {"title", "x"},  {"date", "2026-10-29"},
                        {"done", false}, {"priority", 0}, {"order", 0}};
        write(QJsonDocument(QJsonObject{{"version", 2}, {"tasks", QJsonArray{row, row}}}).toJson());
        QVERIFY(!p.load());
        QCOMPARE(p.tasks[0].id, id);
    }
    void invalidSavePreservesFile() {
        QTemporaryDir dir(QDir::currentPath() + "/invalid-save-XXXXXX");
        Planner p(dir.filePath("plans.json"));
        p.add(QDate::currentDate(), "kept", 0);
        QVERIFY(p.save());
        QFile file(p.filePath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        auto original = file.readAll();
        file.close();
        p.tasks[0].date = QDate();
        QVERIFY(!p.save());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), original);
    }
    void csvEscapesAndFilters() {
        Planner p("unused");
        p.add(QDate(2026, 10, 29), "comma, quote\"\nnext", 1);
        p.add(QDate(2026, 10, 29), "=1+1", 0);
        p.add(QDate(2026, 11, 1), "excluded", 0);
        auto csv = p.monthCsv(QDate(2026, 10, 1));
        QVERIFY(csv.startsWith(QByteArray::fromHex("efbbbf")));
        QVERIFY(csv.contains("\"comma, quote\"\"\nnext\""));
        QVERIFY(csv.contains("\"'=1+1\""));
        QVERIFY(!csv.contains("excluded"));
        QCOMPARE(p.stats(QDate()).total, 0);
    }
    void weatherValidationAndMapping() {
        QString error;
        auto reading = WeatherReading::parse(
            R"({"current":{"temperature_2m":22.8,"weather_code":2,"is_day":1}})", &error);
        QVERIFY(reading);
        QCOMPARE(reading->themeName(), QString("forest"));
        QVERIFY(error.isEmpty());
        reading->code = 85;
        QCOMPARE(reading->themeName(), QString("lilac"));
        reading->isDay = false;
        QCOMPARE(reading->themeName(), QString("night"));
        QVERIFY(!WeatherReading::parse(
            R"({"current":{"temperature_2m":null,"weather_code":0,"is_day":1}})", &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!WeatherReading::parse(
            R"({"current":{"temperature_2m":22,"weather_code":999,"is_day":1}})"));
        QVERIFY(!WeatherReading::parse("bad"));
        auto url = forecastUrl(31.22222, 121.45806).toEncoded();
        QVERIFY(url.contains("current=temperature_2m%2Cweather_code%2Cis_day"));
        QVERIFY(url.contains("latitude=31.22222"));
    }
    void orderBoundariesAndReschedule() {
        Planner p("unused");
        QDate first(2026, 10, 29), second(2026, 10, 30);
        auto a = p.add(first, "A", 0);
        p.find(a)->order = std::numeric_limits<int>::max();
        auto b = p.add(first, "B", 0);
        QCOMPARE(p.forDate(first).last().id, b);
        QVERIFY(p.find(b)->order >= 0);
        auto c = p.add(second, "C", 0);
        p.find(c)->order = std::numeric_limits<int>::max();
        p.reschedule(b, second);
        auto items = p.forDate(second);
        QCOMPARE(items.size(), 2);
        QCOMPARE(items.first().id, c);
        QCOMPARE(items.last().id, b);
        QCOMPARE(items.last().order, 1);
        p.reschedule(a, QDate());
        QCOMPARE(p.find(a)->date, first);
    }
};
QTEST_GUILESS_MAIN(PlannerTests)
#include "tests.moc"
