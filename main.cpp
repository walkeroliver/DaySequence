// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "mainwindow.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFontDatabase>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <memory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    QCoreApplication::setApplicationName("DaySequence");
    QCoreApplication::setOrganizationName("DaySequence");
    QCoreApplication::setApplicationVersion("2.1.0");
    if (QGuiApplication::platformName() == "offscreen") {
        const auto fonts = qEnvironmentVariable("WINDIR", "C:/Windows") + "/Fonts/";
        QFontDatabase::addApplicationFont(fonts + "msyh.ttc");
        QFontDatabase::addApplicationFont(fonts + "msyhbd.ttc");
        QFontDatabase::addApplicationFont(fonts + "segoeui.ttf");
    }
    QApplication::setFont(QFont("Microsoft YaHei UI", 10));
    const auto args = app.arguments();
    const bool preview = args.contains("--preview");
    std::unique_ptr<QTemporaryDir> temporary;
    QString directory;
    if (preview) {
        temporary = std::make_unique<QTemporaryDir>(QDir::currentPath() + "/.preview-XXXXXX");
        if (!temporary->isValid()) {
            QMessageBox::warning(nullptr, "无法生成预览", temporary->errorString());
            return 1;
        }
        directory = temporary->path();
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory);
    } else
        directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (directory.isEmpty() || !QDir().mkpath(directory)) {
        QMessageBox::warning(nullptr, "无法启动日序", "无法创建数据文件夹：\n" + directory);
        return 1;
    }
    QLockFile lock(directory + "/app.lock");
    lock.setStaleLockTime(0);
    const auto serverName =
        "DaySequence-" +
        QString(QCryptographicHash::hash(directory.toUtf8(), QCryptographicHash::Sha256)
                    .toHex()
                    .left(16));
    if (!lock.tryLock(100)) {
        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (socket.waitForConnected(1500)) {
            socket.write("show");
            socket.flush();
            socket.waitForBytesWritten(500);
            return 0;
        }
        QMessageBox::warning(
            nullptr, "暂时无法打开日序",
            "无法连接正在运行的日序，或数据文件夹不可写。请检查托盘中的日序后重试。");
        return 1;
    }
    QLocalServer::removeServer(serverName);
    QLocalServer server;
    if (!server.listen(serverName)) {
        QMessageBox::warning(nullptr, "无法启动日序", server.errorString());
        return 1;
    }
    MainWindow window(directory + "/plans-v2.json", nullptr, !preview);
    QObject::connect(&server, &QLocalServer::newConnection, &window, [&] {
        while (server.hasPendingConnections())
            server.nextPendingConnection()->deleteLater();
        window.restoreWindow();
    });
    if (preview) {
        int index = args.indexOf("--preview");
        window.preview(index + 1 < args.size() ? args[index + 1] : QDir::currentPath());
        return 0;
    }
    window.show();
    return app.exec();
}
