// SPDX-License-Identifier: GPL-3.0-only
#include "journalpage.h"
#include "journalstore.h"
#include "mainwindow.h"
#include "storage.h"
#include <QTemporaryDir>
#include <QtTest>
#include <QtWidgets>

class JournalTests : public QObject {
    Q_OBJECT
    void write(const QString &path, const QByteArray &bytes) {
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(bytes), bytes.size());
    }
    QByteArray read(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return file.readAll();
    }
    QPushButton *button(QWidget &widget, const QString &text) {
        for (auto b : widget.findChildren<QPushButton *>())
            if (b->text() == text)
                return b;
        return nullptr;
    }
  private slots:
    void blankDaysCreateNothingAndMarkdownRoundTrips() {
        QTemporaryDir dir;
        JournalStore store(dir.path());
        JournalStore::Document document;
        QString error;
        const QDate day(2026, 11, 2);
        QVERIFY(store.read(day, &document, &error));
        QVERIFY(!document.exists);
        QVERIFY(store.save(day, " \n\t", document, &error));
        QVERIFY(!QFileInfo::exists(store.directory()));
        const auto markdown =
            QString::fromUtf8("# 每日总结\n\n- [x] 完成设计 🌿\n\n**有收获的一天**\n");
        QVERIFY(store.save(day, markdown, document, &error));
        QCOMPARE(read(store.path(day)), markdown.toUtf8());
        QVERIFY(store.read(day, &document, &error));
        QCOMPARE(document.text, markdown);
        QVERIFY(store.save(day, "", document, &error));
        QVERIFY(QFileInfo::exists(store.path(day)));
        QCOMPARE(read(store.path(day)), QByteArray());
        QVERIFY(!store.read(QDate(), &document, &error));
        QVERIFY(!error.isEmpty());
    }
    void refusesConflictingWritesAndRecyclesWithoutLosingOldCopies() {
        QTemporaryDir dir;
        JournalStore store(dir.path());
        const QDate day(2026, 11, 2);
        JournalStore::Document document;
        QString error;
        QVERIFY(store.read(day, &document, &error));
        write(store.path(day), "external");
        QVERIFY(!store.save(day, "overwrite", document, &error));
        QVERIFY(!store.recycle(day, document, &error));
        QCOMPARE(read(store.path(day)), QByteArray("external"));
        QVERIFY(store.read(day, &document, &error));
        QVERIFY(store.recycle(day, document, &error));
        QVERIFY(!QFileInfo::exists(store.path(day)));
        auto trash = QDir(store.directory() + "/.trash");
        QCOMPARE(trash.entryList({"*.md"}, QDir::Files).size(), 1);
        QCOMPARE(read(trash.filePath(trash.entryList({"*.md"}, QDir::Files).first())),
                 QByteArray("external"));
        write(store.path(day), "another copy");
        QVERIFY(store.read(day, &document, &error));
        QVERIFY(store.recycle(day, document, &error));
        QCOMPARE(trash.entryList({"*.md"}, QDir::Files).size(), 2);
        QVERIFY(store.dates().isEmpty());
    }
    void rejectsInvalidUtf8OversizeAndFolderInPlaceOfFile() {
        QTemporaryDir dir;
        JournalStore store(dir.path());
        const QDate day(2026, 11, 2);
        JournalStore::Document document;
        QString error;
        write(store.path(day), QByteArray::fromHex("ffff"));
        QVERIFY(!store.read(day, &document, &error));
        write(store.path(day), QByteArray::fromHex("e4b8"));
        QVERIFY(!store.read(day, &document, &error));
        write(store.path(day), QByteArray(JournalStore::MaximumBytes + 1, 'x'));
        QVERIFY(!store.read(day, &document, &error));
        QVERIFY(QFile::remove(store.path(day)));
        QVERIFY(QDir().mkpath(store.path(day)));
        QVERIFY(!store.read(day, &document, &error));
        QVERIFY(!store.save(day, "text", document, &error));
    }
    void historyDatesAreStrictAndDescending() {
        QTemporaryDir dir;
        JournalStore store(dir.path());
        write(store.directory() + "/2026-02-30.md", "bad date");
        write(store.directory() + "/2026-2-02.md", "bad format");
        write(store.directory() + "/notes.md", "ignored");
        write(store.directory() + "/2026-11-01.md", "old");
        write(store.directory() + "/2026-11-02.md", "new");
        QCOMPARE(store.dates(), QList<QDate>({QDate(2026, 11, 2), QDate(2026, 11, 1)}));
    }
    void editorAutosavesSearchesAndKeepsDateAcrossSessions() {
        QTemporaryDir dir;
        JournalStore store(dir.path());
        const QDate yesterday = QDate::currentDate().addDays(-1);
        {
            JournalPage page(dir.path());
            QVERIFY(!QFileInfo::exists(store.directory()));
            QVERIFY(page.openDate(yesterday));
            auto editor = page.findChild<QPlainTextEdit *>("journalEditor");
            QVERIFY(editor);
            editor->setPlainText(QStringLiteral("# 回顾\n\n完成设计"));
            QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(store.path(yesterday)), 1800);
            QCOMPARE(read(store.path(yesterday)), QStringLiteral("# 回顾\n\n完成设计").toUtf8());
            editor->insertPlainText("\nmore");
            QVERIFY(page.openDate(QDate::currentDate()));
            QVERIFY(read(store.path(yesterday)).contains("more"));
            QVERIFY(!QFileInfo::exists(store.path(QDate::currentDate())));
            auto history = page.findChild<QListWidget *>("journalHistory");
            auto search = page.findChild<QLineEdit *>("journalSearch");
            QVERIFY(history && search);
            QCOMPARE(history->count(), 1);
            search->setText("not in text");
            QTRY_COMPARE(history->count(), 0);
            search->setText(QStringLiteral("设计"));
            QTRY_COMPARE(history->count(), 1);
            QVERIFY(page.openDate(yesterday));
            auto tabs = page.findChild<QTabWidget *>("journalTabs");
            tabs->setCurrentIndex(1);
            QVERIFY(page.findChild<QTextBrowser *>("journalPreview")
                        ->toPlainText()
                        .contains(QStringLiteral("完成设计")));
        }
        JournalPage restored(dir.path());
        QVERIFY(restored.openDate(yesterday));
        QVERIFY(
            restored.findChild<QPlainTextEdit *>("journalEditor")->toPlainText().contains("more"));
    }
    void conflictKeepsEditsAndBlocksNavigationUntilExplicitReload() {
        QTemporaryDir dir;
        JournalStore store(dir.path());
        JournalPage page(dir.path());
        const auto day = page.date();
        auto editor = page.findChild<QPlainTextEdit *>("journalEditor");
        editor->setPlainText("local text");
        QVERIFY(page.flush());
        write(store.path(day), "external text");
        editor->setPlainText("unsaved local edits");
        QVERIFY(!page.flush());
        QVERIFY(!page.openDate(day.addDays(-1)));
        QCOMPARE(page.date(), day);
        QCOMPARE(editor->toPlainText(), QString("unsaved local edits"));
        QCOMPARE(read(store.path(day)), QByteArray("external text"));
        QTimer::singleShot(0, [] {
            if (auto message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                message->button(QMessageBox::Yes)->click();
        });
        button(page, QStringLiteral("重新读取"))->click();
        QCOMPARE(editor->toPlainText(), QString("external text"));
        QVERIFY(page.openDate(day.addDays(-1)));
    }
    void deletionRequiresConfirmationAndKeepsRecoverableMarkdown() {
        QTemporaryDir dir;
        JournalStore store(dir.path());
        JournalPage page(dir.path());
        auto editor = page.findChild<QPlainTextEdit *>("journalEditor");
        editor->setPlainText("keep in trash");
        QVERIFY(page.flush());
        QTimer::singleShot(0, [] {
            if (auto message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                message->button(QMessageBox::No)->click();
        });
        page.findChild<QPushButton *>("journalDelete")->click();
        QVERIFY(QFileInfo::exists(store.path(page.date())));
        QTimer::singleShot(0, [] {
            if (auto message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                message->button(QMessageBox::Yes)->click();
        });
        page.findChild<QPushButton *>("journalDelete")->click();
        QVERIFY(editor->toPlainText().isEmpty());
        QVERIFY(!QFileInfo::exists(store.path(page.date())));
        QCOMPARE(QDir(store.directory() + "/.trash").entryList({"*.md"}, QDir::Files).size(), 1);
    }
    void windowSavesBeforePageChangesHideAndExit() {
        QTemporaryDir dir;
        MainWindow window(dir.filePath("tasks.json"));
        window.show();
        auto page = window.findChild<JournalPage *>();
        QVERIFY(page);
        button(window, QStringLiteral("每日总结"))->click();
        auto editor = page->findChild<QPlainTextEdit *>("journalEditor");
        editor->setPlainText("save before switching");
        button(window, QStringLiteral("每日计划"))->click();
        JournalStore store(dir.path());
        QCOMPARE(read(store.path(page->date())), QByteArray("save before switching"));
        editor->setPlainText("save before exit");
        QSignalSpy exits(&window, &MainWindow::exitRequested);
        window.findChild<QPushButton *>("exitEntry")->click();
        QCOMPARE(exits.count(), 1);
        QCOMPARE(read(store.path(page->date())), QByteArray("save before exit"));
    }
    void unavailableDirectoryKeepsEditsAndBlocksWindowExit() {
        QTemporaryDir dir;
        MainWindow window(dir.filePath("tasks.json"));
        auto page = window.findChild<JournalPage *>();
        auto editor = page->findChild<QPlainTextEdit *>("journalEditor");
        write(dir.filePath("document"), "directory blocked by file");
        editor->setPlainText("must not be lost");
        QVERIFY(!page->flush());
        QSignalSpy exits(&window, &MainWindow::exitRequested);
        window.findChild<QPushButton *>("exitEntry")->click();
        QCOMPARE(exits.count(), 0);
        QCOMPARE(editor->toPlainText(), QString("must not be lost"));
        QVERIFY(page->isVisible());
        QVERIFY(QFile::remove(dir.filePath("document")));
        QVERIFY(page->flush());
        window.findChild<QPushButton *>("exitEntry")->click();
        QCOMPARE(exits.count(), 1);
    }
    void previewBlocksLocalAndRemoteImageLoading() {
        QTemporaryDir dir;
        write(dir.filePath("private.txt"), "should not load");
        JournalPage page(dir.path());
        auto editor = page.findChild<QPlainTextEdit *>("journalEditor");
        const auto local = QUrl::fromLocalFile(dir.filePath("private.txt"));
        editor->setPlainText("![local](" + local.toString() +
                             ")\n![remote](https://example.invalid/image.png)");
        page.findChild<QTabWidget *>("journalTabs")->setCurrentIndex(1);
        auto preview = page.findChild<QTextBrowser *>("journalPreview");
        QVERIFY(!preview->openExternalLinks());
        QVERIFY(!preview->openLinks());
        QVERIFY(preview->document()
                    ->resource(QTextDocument::ImageResource, local)
                    .toByteArray()
                    .isEmpty());
        QVERIFY(
            preview->document()
                ->resource(QTextDocument::ImageResource, QUrl("https://example.invalid/image.png"))
                .toByteArray()
                .isEmpty());
    }
    void relocationIncludesDocumentsAndTrashAndRejectsConflicts() {
        QTemporaryDir dir;
        StorageManager manager(dir.path());
        const auto source = dir.filePath("data"), target = dir.filePath("next");
        write(source + "/document/2026-11-02.md", "# journal");
        write(source + "/document/.trash/old.md", "recycled");
        QString error;
        QVERIFY2(manager.relocate(source, target, &error), qPrintable(error));
        QCOMPARE(read(target + "/document/2026-11-02.md"), QByteArray("# journal"));
        QCOMPARE(read(target + "/document/.trash/old.md"), QByteArray("recycled"));
        write(target + "/document/2026-11-02.md", "other version");
        QVERIFY(!manager.relocate(source, target, &error));
        QCOMPARE(read(target + "/document/2026-11-02.md"), QByteArray("other version"));
        QCOMPARE(read(source + "/document/2026-11-02.md"), QByteArray("# journal"));
    }
};
QTEST_MAIN(JournalTests)
#include "journal_tests.moc"
