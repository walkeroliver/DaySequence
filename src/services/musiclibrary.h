// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QStringList>
#include <QUrl>
class MusicLibrary {
  public:
    explicit MusicLibrary(QString dataDirectory);
    QString directory() const;
    bool ensurePresets(QString *error = nullptr) const;
    QStringList tracks() const;
    QString add(const QString &file, QString *error = nullptr,
                const QString &preferredName = {}) const;
    QUrl source(const QString &relative) const;
    static bool validRelative(const QString &relative);
    static QString displayName(const QString &relative);

  private:
    QString dataDirectory;
};
