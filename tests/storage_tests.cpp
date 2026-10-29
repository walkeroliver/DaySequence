// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "planner.h"
#include "storage.h"
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class StorageTests : public QObject {
    Q_OBJECT
  private:
    void write(const QString &path, const QByteArray &bytes) {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QCOMPARE(f.write(bytes), bytes.size());
    }
    QByteArray read(const QString &path) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return {};
        return f.readAll();
    }
  private slots:
    void defaultsToPortableDirectory() {
        QTemporaryDir d(QDir::currentPath() + "/storage-default-XXXXXX");
        StorageManager manager(d.path());
        QString error;
        QCOMPARE(manager.directory(&error), d.filePath("data"));
        QVERIFY(error.isEmpty());
        QVERIFY(!QFile::exists(manager.configurationFile()));
    }
    void movesTasksAndPreferencesAndRemembersSelection() {
        QTemporaryDir d(QDir::currentPath() + "/storage-move-XXXXXX");
        StorageManager manager(d.path());
        const auto old = d.filePath("data"), next = d.filePath("selected");
        QVERIFY(QDir().mkpath(old));
        Planner p(old + "/tasks.json");
        p.add(QDate::currentDate(), "kept task", 2);
        QVERIFY(p.save());
        {
            QSettings s(old + "/settings.ini", QSettings::IniFormat);
            s.setValue("theme", "chinese");
            s.sync();
        }
        const auto original = read(old + "/tasks.json");
        QString error;
        QVERIFY2(manager.relocate(old, next, &error), qPrintable(error));
        QCOMPARE(read(next + "/tasks.json"), original);
        QCOMPARE(read(old + "/tasks.json"), original);
        QSettings moved(next + "/settings.ini", QSettings::IniFormat);
        QCOMPARE(moved.value("theme").toString(), QString("chinese"));
        StorageManager reopened(d.path());
        QCOMPARE(reopened.directory(), QFileInfo(next).canonicalFilePath());
    }
    void neverOverwritesOtherData() {
        QTemporaryDir d(QDir::currentPath() + "/storage-conflict-XXXXXX");
        StorageManager manager(d.path());
        auto old = d.filePath("data"), next = d.filePath("selected");
        QVERIFY(QDir().mkpath(old));
        QVERIFY(QDir().mkpath(next));
        write(old + "/tasks.json", "source");
        write(next + "/tasks.json", "another record");
        QString error;
        QVERIFY(!manager.relocate(old, next, &error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(read(next + "/tasks.json"), QByteArray("another record"));
        QCOMPARE(manager.directory(), old);
        QVERIFY(!QFile::exists(manager.configurationFile()));
    }
    void rollsBackWhenLocationCannotBeSaved() {
        QTemporaryDir d(QDir::currentPath() + "/storage-rollback-XXXXXX");
        StorageManager manager(d.path());
        auto old = d.filePath("data"), next = d.filePath("selected");
        QVERIFY(QDir().mkpath(old));
        QVERIFY(QDir().mkpath(manager.configurationFile()));
        write(old + "/tasks.json", "original");
        write(old + "/settings.ini", "[General]\ntheme=chinese\n");
        QString error;
        QVERIFY(!manager.relocate(old, next, &error));
        QVERIFY(!QFile::exists(next + "/tasks.json"));
        QVERIFY(!QFile::exists(next + "/settings.ini"));
        QCOMPARE(read(old + "/tasks.json"), QByteArray("original"));
    }
    void rejectsDataDirectoryInUse() {
        QTemporaryDir d(QDir::currentPath() + "/storage-lock-XXXXXX");
        StorageManager manager(d.path());
        auto old = d.filePath("data"), next = d.filePath("selected");
        QVERIFY(QDir().mkpath(old));
        QVERIFY(QDir().mkpath(next));
        QLockFile lock(next + "/app.lock");
        QVERIFY(lock.tryLock());
        QString error;
        QVERIFY(!manager.relocate(old, next, &error));
        QVERIFY(!QFile::exists(manager.configurationFile()));
    }
    void detectsDamagedPointerInsteadOfOpeningEmptyData() {
        QTemporaryDir d(QDir::currentPath() + "/storage-config-XXXXXX");
        StorageManager manager(d.path());
        write(manager.configurationFile(), "broken");
        QString error;
        QVERIFY(manager.directory(&error).isEmpty());
        QVERIFY(!error.isEmpty());
    }
};
QTEST_GUILESS_MAIN(StorageTests)
#include "storage_tests.moc"
