// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "storage.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QMap>
#include <QSaveFile>
#include <QTemporaryFile>

namespace {
QString absoluteDirectory(const QString &path) {
    auto info = QFileInfo(QDir::cleanPath(path));
    return info.exists() ? info.canonicalFilePath() : info.absoluteFilePath();
}
bool writeFile(const QString &path, const QByteArray &bytes, QString *error) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}
} // namespace
StorageManager::StorageManager(QString directory)
    : applicationDirectory(absoluteDirectory(directory)) {}
QString StorageManager::configurationFile() const {
    return QDir(applicationDirectory).filePath("storage.json");
}
QString StorageManager::directory(QString *error) const {
    if (error)
        error->clear();
    QFile file(configurationFile());
    if (!file.exists())
        return QDir(applicationDirectory).filePath("data");
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return {};
    }
    QJsonParseError parse;
    auto doc = QJsonDocument::fromJson(file.readAll(), &parse);
    const auto value = doc.object().value("dataDirectory");
    if (parse.error != QJsonParseError::NoError || !value.isString() ||
        value.toString().trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("保存位置配置无效，请检查程序旁的 storage.json。");
        return {};
    }
    const auto path = value.toString();
    return absoluteDirectory(
        QDir::isAbsolutePath(path) ? path : QDir(applicationDirectory).filePath(path));
}
bool StorageManager::relocate(const QString &sourceDirectory, const QString &destinationDirectory,
                              QString *error) const {
    if (error)
        error->clear();
    if (destinationDirectory.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("请选择保存文件夹。");
        return false;
    }
    const auto source = absoluteDirectory(sourceDirectory),
               target = absoluteDirectory(destinationDirectory);
    if (source.compare(target, Qt::CaseInsensitive) == 0)
        return true;
    const QStringList names = {"tasks.json", "settings.ini"};
    QMap<QString, QByteArray> contents;
    for (const auto &name : names) {
        QFile sourceFile(QDir(source).filePath(name));
        if (!sourceFile.exists())
            continue;
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            if (error)
                *error = QStringLiteral("无法读取当前数据：%1").arg(sourceFile.errorString());
            return false;
        }
        auto bytes = sourceFile.readAll();
        if (sourceFile.error() != QFile::NoError) {
            if (error)
                *error = sourceFile.errorString();
            return false;
        }
        QFile destination(QDir(target).filePath(name));
        if (destination.exists()) {
            if (!destination.open(QIODevice::ReadOnly) || destination.readAll() != bytes ||
                destination.error() != QFile::NoError) {
                if (error)
                    *error = QStringLiteral(
                        "所选文件夹中已有不同的任务或偏好，请选择空文件夹，避免覆盖现有记录。");
                return false;
            }
        } else
            contents.insert(name, bytes);
    }
    if (!QDir().mkpath(target)) {
        if (error)
            *error = QStringLiteral("无法创建所选文件夹。");
        return false;
    }
    QLockFile destinationLock(QDir(target).filePath("app.lock"));
    destinationLock.setStaleLockTime(0);
    if (!destinationLock.tryLock(0)) {
        if (error)
            *error = QStringLiteral("所选目录正在由另一个日序使用，请先退出该程序。");
        return false;
    }
    QTemporaryFile probe(QDir(target).filePath(".daysequence-write-XXXXXX"));
    if (!probe.open()) {
        if (error)
            *error = QStringLiteral("所选文件夹不可写入：%1").arg(probe.errorString());
        return false;
    }
    probe.close();
    QStringList created;
    auto rollback = [&] {
        for (const auto &path : created)
            QFile::remove(path);
    };
    for (auto it = contents.cbegin(); it != contents.cend(); ++it) {
        const auto path = QDir(target).filePath(it.key());
        if (!writeFile(path, it.value(), error)) {
            rollback();
            return false;
        }
        created.append(path);
    }
    const auto relative = QDir(applicationDirectory).relativeFilePath(target);
    const auto stored = relative == "data" ? relative : target;
    if (!writeFile(configurationFile(),
                   QJsonDocument(QJsonObject{{"dataDirectory", stored}}).toJson(), error)) {
        rollback();
        return false;
    }
    return true;
}
