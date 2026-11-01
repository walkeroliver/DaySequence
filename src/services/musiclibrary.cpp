// SPDX-License-Identifier: GPL-3.0-only
#include "musiclibrary.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
namespace {
const qint64 maximumSize = 100LL * 1024 * 1024;
bool supported(const QString &name) {
    return QStringList{"wav", "mp3", "flac", "m4a", "ogg"}.contains(
        QFileInfo(name).suffix().toLower());
}
bool copy(QFile &input, const QString &destination, QString *error) {
    QSaveFile output(destination);
    if (!output.open(QIODevice::WriteOnly)) {
        if (error)
            *error = "音乐文件夹不可写，请检查数据目录权限。";
        return false;
    }
    qint64 total = 0;
    while (!input.atEnd()) {
        const auto bytes = input.read(256 * 1024);
        total += bytes.size();
        if (input.error() != QFile::NoError || total > maximumSize ||
            (bytes.isEmpty() && !input.atEnd()) || output.write(bytes) != bytes.size()) {
            if (error)
                *error = "音乐复制失败或文件超过 100 MB，原文件保持不变。";
            return false;
        }
    }
    if (!output.commit()) {
        if (error)
            *error = "音乐保存失败，请检查磁盘空间。";
        return false;
    }
    return true;
}
QByteArray digest(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&f))
        return {};
    return hash.result();
}
} // namespace
MusicLibrary::MusicLibrary(QString path) : dataDirectory(QDir(path).absolutePath()) {}
QString MusicLibrary::directory() const {
    return QDir(dataDirectory).filePath("music");
}
bool MusicLibrary::validRelative(const QString &value) {
    return value.startsWith("music/") && value.count('/') == 1 && !value.contains('\\') &&
           supported(value) && QFileInfo(value).fileName() != "." &&
           QFileInfo(value).fileName() != "..";
}
bool MusicLibrary::ensurePresets(QString *error) const {
    if (error)
        error->clear();
    if (QFileInfo(directory()).isSymLink() || !QDir().mkpath(directory())) {
        if (error)
            *error = "无法创建 music 文件夹，请检查保存位置。";
        return false;
    }
    for (const auto &id : {"chime", "soft", "bell"}) {
        const auto name = QString(id) + ".wav", target = QDir(directory()).filePath(name);
        if (QFile::exists(target))
            continue;
        QFile input(":/sounds/" + name);
        if (!input.open(QIODevice::ReadOnly) || !copy(input, target, error))
            return false;
    }
    return true;
}
QStringList MusicLibrary::tracks() const {
    QStringList result;
    if (QFileInfo(directory()).isSymLink())
        return result;
    for (const auto &file : QDir(directory())
                                .entryInfoList(QDir::Files | QDir::Readable | QDir::NoSymLinks,
                                               QDir::Name | QDir::IgnoreCase))
        if (supported(file.fileName()) && file.size() > 0 && file.size() <= maximumSize)
            result.append("music/" + file.fileName());
    return result;
}
QUrl MusicLibrary::source(const QString &value) const {
    if (!validRelative(value) || QFileInfo(directory()).isSymLink() ||
        QFileInfo(QDir(dataDirectory).filePath(value)).isSymLink())
        return {};
    return QUrl::fromLocalFile(QDir(dataDirectory).filePath(value));
}
QString MusicLibrary::displayName(const QString &relative) {
    if (relative == "music/chime.wav")
        return "清晨铃";
    if (relative == "music/soft.wav")
        return "柔和音";
    if (relative == "music/bell.wav")
        return "双叩铃";
    return QFileInfo(relative).completeBaseName();
}
QString MusicLibrary::add(const QString &path, QString *error, const QString &preferredName) const {
    if (error)
        error->clear();
    auto fail = [&](const QString &message) {
        if (error)
            *error = message;
        return QString();
    };
    const QFileInfo info(path);
    QFile input(path);
    if (info.isSymLink() || !supported(info.fileName()) || !input.open(QIODevice::ReadOnly))
        return fail("请选择可读取的 WAV、MP3、FLAC、M4A 或 OGG 文件。");
    if (input.size() < 1 || input.size() > maximumSize)
        return fail("音乐文件不能为空，大小请控制在 100 MB 以内。");
    if (!ensurePresets(error))
        return {};
    QString name = QFileInfo(preferredName.isEmpty() ? info.fileName() : preferredName).fileName();
    if (!supported(name))
        name = info.fileName();
    const auto base = QFileInfo(name).completeBaseName(), extension = QFileInfo(name).suffix();
    int number = 2;
    while (QFile::exists(QDir(directory()).filePath(name))) {
        const auto destination = QDir(directory()).filePath(name);
        if (QFileInfo(destination).isSymLink())
            return fail("同名文件是链接，请选择其他音乐文件名。");
        if (QFileInfo(destination).canonicalFilePath() == info.canonicalFilePath() ||
            digest(destination) == digest(path))
            return "music/" + name;
        if (number > 10000)
            return fail("同名文件过多，请修改文件名后重试。");
        name = base + " (" + QString::number(number++) + ")." + extension;
    }
    if (!copy(input, QDir(directory()).filePath(name), error))
        return {};
    return "music/" + name;
}
