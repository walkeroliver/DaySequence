// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QByteArray>
#include <QDate>
#include <QList>
#include <QString>

class JournalStore {
  public:
    struct Document {
        QString text;
        QByteArray bytes;
        bool exists = false;
    };
    static constexpr qint64 MaximumBytes = 2 * 1024 * 1024;
    explicit JournalStore(QString dataDirectory);
    QString directory() const;
    QString path(QDate date) const;
    QList<QDate> dates() const;
    bool read(QDate date, Document *document, QString *error) const;
    bool save(QDate date, const QString &text, const Document &previous, QString *error) const;
    bool recycle(QDate date, const Document &previous, QString *error) const;

  private:
    QString root;
    bool safe(QString *error) const;
    bool unchanged(QDate date, const Document &previous, QString *error) const;
};
