// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "mainwindow.h"
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>
#include <QtWidgets>

class LifecycleTests : public QObject {
    Q_OBJECT
  private slots:
    void exitsProcess_data() {
        QTest::addColumn<QString>("scenario");
        for (const auto &scenario :
             {"close", "window-close", "sidebar", "hidden-sidebar", "hidden-tray", "minimize"})
            QTest::newRow(scenario) << QString(scenario);
    }
    void exitsProcess() {
        QFETCH(QString, scenario);
        QTemporaryDir dir(QDir::currentPath() + "/lifecycle-XXXXXX");
        QVERIFY(dir.isValid());
        QProcess child;
        child.start(QCoreApplication::applicationFilePath(),
                    {"-platform", "offscreen", "--scenario", scenario, dir.path()});
        QVERIFY(child.waitForStarted());
        const bool exited = child.waitForFinished(5000);
        if (!exited) {
            child.kill();
            child.waitForFinished();
        }
        QVERIFY2(exited, "Exit left the application process running");
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QCOMPARE(child.exitCode(), 0);
        Planner saved(dir.filePath("tasks.json"));
        QVERIFY(saved.load());
        QCOMPARE(saved.tasks.size(), 1);
        QCOMPARE(saved.tasks[0].title, QString("keep on exit"));
        QSettings settings(dir.filePath("settings.ini"), QSettings::IniFormat);
        QVERIFY(settings.contains("windowGeometry"));
    }
};
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    const auto args = app.arguments();
    const auto index = args.indexOf("--scenario");
    if (index < 0) {
        LifecycleTests tests;
        return QTest::qExec(&tests, argc, argv);
    }
    // Hidden windows do not cause lastWindowClosed; explicit exits must still end the process.
    app.setQuitOnLastWindowClosed(false);
    const auto scenario = args.value(index + 1), path = args.value(index + 2) + "/tasks.json";
    Planner plan(path);
    plan.add(QDate::currentDate(), "keep on exit", 0);
    if (!plan.save())
        return 2;
    MainWindow window(path);
    QObject::connect(&window, &MainWindow::exitRequested, &app, &QCoreApplication::quit,
                     Qt::QueuedConnection);
    window.show();
    QTimer::singleShot(20, &window, [&] {
        auto tray = window.findChild<QSystemTrayIcon *>();
        if (scenario.startsWith("hidden"))
            window.hide();
        if (scenario == "close") {
            window.findChild<QPushButton *>("closeButton")->click();
        } else if (scenario == "window-close") {
            window.close();
        } else if (scenario == "hidden-tray") {
            for (auto action : tray->contextMenu()->actions())
                if (action->text() == QStringLiteral("退出日序"))
                    action->trigger();
        } else if (scenario == "minimize") {
            window.showMinimized();
            QTimer::singleShot(80, &window, [&] {
                if (window.isHidden() || !window.isMinimized()) {
                    app.exit(3);
                    return;
                }
                window.findChild<QPushButton *>("exitEntry")->click();
            });
        } else {
            window.findChild<QPushButton *>("exitEntry")->click();
        }
    });
    return app.exec();
}
#include "lifecycle_tests.moc"