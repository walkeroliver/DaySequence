// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "journalstore.h"
#include <QWidget>
class QPlainTextEdit;
class QTextBrowser;
class QDateEdit;
class QLineEdit;
class QListWidget;
class QLabel;
class QTimer;
class JournalPage : public QWidget {
    Q_OBJECT
  public:
    explicit JournalPage(const QString &dataDirectory, QWidget *parent = nullptr);
    bool flush();
    bool openDate(QDate date);
    QDate date() const {
        return current;
    }

  private:
    JournalStore store;
    JournalStore::Document snapshot;
    QDate current;
    bool dirty = false, readable = true, rebuilding = false;
    QPlainTextEdit *editor;
    QTextBrowser *preview;
    QDateEdit *datePicker;
    QLineEdit *search;
    QListWidget *history;
    QLabel *status, *heading, *counter;
    QTimer *saveTimer, *searchTimer;
    void rebuildHistory();
    void showPreview();
    void reload();
    void updateStatus(const QString &message);
};
