// SPDX-License-Identifier: GPL-3.0-only
#include "journalstore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringDecoder>
#include <QUuid>
#include <algorithm>

JournalStore::JournalStore(QString dataDirectory)
    : root(QDir(dataDirectory).filePath("document")) {}
QString JournalStore::directory() const {
    return root;
}
QString JournalStore::path(QDate date) const {
    return date.isValid() && date.year() >= 1 && date.year() <= 9999
               ? QDir(root).filePath(date.toString("yyyy-MM-dd") + ".md")
               : QString();
}
bool JournalStore::safe(QString *error) const {
    if (error)
        error->clear();
    const QFileInfo folder(root);
    if (folder.isSymLink() || (folder.exists() && !folder.isDir())) {
        if (error)
            *error = "总结目录不可用，不能是文件或目录链接。";
        return false;
    }
    return true;
}
QList<QDate> JournalStore::dates() const {
    QList<QDate> result;
    if (!safe(nullptr))
        return result;
    for (const auto &file : QDir(root).entryInfoList({"*.md"}, QDir::Files | QDir::NoSymLinks)) {
        const auto date = QDate::fromString(file.completeBaseName(), "yyyy-MM-dd");
        if (!path(date).isEmpty() && file.fileName() == date.toString("yyyy-MM-dd") + ".md")
            result.append(date);
    }
    std::sort(result.begin(), result.end(), std::greater<QDate>());
    return result;
}
bool JournalStore::read(QDate date, Document *document, QString *error) const {
    if (!safe(error))
        return false;
    const auto name = path(date);
    if (name.isEmpty()) {
        if (error)
            *error = "日期无效。";
        return false;
    }
    QFileInfo info(name);
    if (info.isSymLink() || (info.exists() && !info.isFile())) {
        if (error)
            *error = "总结文件不是普通文件，已停止读取。";
        return false;
    }
    Document loaded;
    loaded.exists = info.exists();
    if (loaded.exists) {
        QFile file(name);
        if (!file.open(QIODevice::ReadOnly)) {
            if (error)
                *error = file.errorString();
            return false;
        }
        loaded.bytes = file.read(MaximumBytes + 1);
        if (file.error() != QFile::NoError || loaded.bytes.size() > MaximumBytes) {
            if (error)
                *error = "总结读取失败或超过 2 MB，请使用外部编辑器检查。";
            return false;
        }
        QStringDecoder decoder(QStringDecoder::Utf8, QStringConverter::Flag::Stateless);
        loaded.text = decoder.decode(loaded.bytes);
        if (decoder.hasError()) {
            if (error)
                *error = "总结不是有效的 UTF-8 文档，原文件已保留。";
            return false;
        }
    }
    *document = loaded;
    return true;
}
bool JournalStore::unchanged(QDate date, const Document &previous, QString *error) const {
    Document current;
    if (!read(date, &current, error))
        return false;
    if (current.exists != previous.exists || current.bytes != previous.bytes) {
        if (error)
            *error = "文件已被其他程序修改。请另存副本，或重新读取后继续编辑。";
        return false;
    }
    return true;
}
bool JournalStore::save(QDate date, const QString &text, const Document &previous,
                        QString *error) const {
    if (!unchanged(date, previous, error))
        return false;
    if (!previous.exists && text.trimmed().isEmpty())
        return true;
    const auto bytes = text.toUtf8();
    if (bytes.size() > MaximumBytes) {
        if (error)
            *error = "总结超过 2 MB，尚未保存。";
        return false;
    }
    if (!QDir().mkpath(root)) {
        if (error)
            *error = "无法创建总结目录，请检查保存位置。";
        return false;
    }
    QSaveFile file(path(date));
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}
bool JournalStore::recycle(QDate date, const Document &previous, QString *error) const {
    if (!unchanged(date, previous, error))
        return false;
    if (!previous.exists)
        return true;
    const auto trash = QDir(root).filePath(".trash");
    if (QFileInfo(trash).isSymLink() || !QDir().mkpath(trash)) {
        if (error)
            *error = "无法创建回收目录，原文件已保留。";
        return false;
    }
    const auto destination =
        QDir(trash).filePath(date.toString("yyyy-MM-dd") + "-" +
                             QUuid::createUuid().toString(QUuid::WithoutBraces) + ".md");
    QFile file(path(date));
    if (!file.rename(destination)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}
