// SPDX-License-Identifier: GPL-3.0-only
#include "journalpage.h"
#include "uicomponents.h"
#include <QtWidgets>

namespace {
class SafeDocument : public QTextDocument {
  public:
    using QTextDocument::QTextDocument;

  protected:
    QVariant loadResource(int, const QUrl &) override {
        // A valid, empty result prevents Qt's local-file fallback.
        return QByteArray();
    }
};
class SafePreview : public QTextBrowser {
  public:
    explicit SafePreview(QWidget *parent = nullptr) : QTextBrowser(parent) {
        setDocument(new SafeDocument(this));
    }

  protected:
    QVariant loadResource(int, const QUrl &) override {
        return QByteArray();
    }
};
} // namespace
JournalPage::JournalPage(const QString &dataDirectory, QWidget *parent)
    : QWidget(parent), store(dataDirectory) {
    setObjectName("journalPage");
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);
    auto archive = card();
    archive->setFixedWidth(220);
    auto left = column(archive, 16);
    left->addWidget(label("往日书页", "sectionTitle"));
    left->addWidget(label("一日一记，留住每一步。", "smallMuted"));
    search = new QLineEdit;
    search->setObjectName("journalSearch");
    search->setPlaceholderText("搜索日期或正文");
    search->setClearButtonEnabled(true);
    left->addWidget(search);
    history = new QListWidget;
    history->setObjectName("journalHistory");
    history->setWordWrap(true);
    history->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    left->addWidget(history, 1);
    counter = label("", "smallMuted");
    counter->setWordWrap(true);
    left->addWidget(counter);
    auto folder = button("打开文档文件夹", "secondary");
    left->addWidget(folder);
    connect(folder, &QPushButton::clicked, this, [this] {
        const auto info = QFileInfo(store.directory());
        if (info.isSymLink() || !QDir().mkpath(store.directory()) ||
            !QDesktopServices::openUrl(QUrl::fromLocalFile(store.directory())))
            updateStatus("无法打开文档目录，请检查保存位置。");
    });
    layout->addWidget(archive);
    auto writing = card();
    auto right = column(writing, 22);
    right->setSpacing(12);
    auto top = new QHBoxLayout;
    heading = label("", "sectionTitle");
    top->addWidget(heading, 1);
    datePicker = new QDateEdit(QDate::currentDate());
    datePicker->setObjectName("journalDate");
    datePicker->setDisplayFormat("yyyy-MM-dd");
    datePicker->setCalendarPopup(true);
    datePicker->setDateRange(QDate(1, 1, 1), QDate(9999, 12, 31));
    top->addWidget(datePicker);
    auto today = button("今天", "secondary");
    top->addWidget(today);
    right->addLayout(top);
    auto toolbar = new QHBoxLayout;
    auto outline = button("插入提纲", "secondary");
    auto save = button("保存", "primary");
    save->setObjectName("journalSave");
    auto reloadButton = button("重新读取", "secondary");
    auto exportButton = button("另存副本", "secondary");
    auto remove = button("删除", "secondary");
    remove->setObjectName("journalDelete");
    toolbar->addWidget(outline);
    toolbar->addStretch();
    for (auto b : {reloadButton, exportButton, remove, save})
        toolbar->addWidget(b);
    right->addLayout(toolbar);
    auto tabs = new QTabWidget;
    tabs->setObjectName("journalTabs");
    editor = new QPlainTextEdit;
    editor->setObjectName("journalEditor");
    editor->setAccessibleName("每日总结 Markdown 编辑器");
    editor->setPlaceholderText("今天完成了什么？\n有哪些收获，或想留给明天的提醒？\n\n直接书写 "
                               "Markdown，或点击「插入提纲」。\n未填写内容时，不会创建空文件。");
    editor->setTabStopDistance(28);
    editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    preview = new SafePreview;
    preview->setObjectName("journalPreview");
    preview->setOpenExternalLinks(false);
    preview->setOpenLinks(false);
    tabs->addTab(editor, "编辑 Markdown");
    tabs->addTab(preview, "阅读预览");
    right->addWidget(tabs, 1);
    status = label("", "journalStatus");
    status->setWordWrap(true);
    status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    right->addWidget(status);
    right->addWidget(label("自动保存 · Ctrl+S 手动保存 · 每日一个 .md 文件", "smallMuted"));
    layout->addWidget(writing, 1);
    saveTimer = new QTimer(this);
    saveTimer->setSingleShot(true);
    saveTimer->setInterval(1000);
    searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(180);
    connect(searchTimer, &QTimer::timeout, this, &JournalPage::rebuildHistory);
    connect(search, &QLineEdit::textChanged, this, [this] { searchTimer->start(); });
    connect(saveTimer, &QTimer::timeout, this, &JournalPage::flush);
    connect(editor, &QPlainTextEdit::textChanged, this, [this] {
        dirty = true;
        updateStatus("尚未保存 · 停止输入后自动保存");
        saveTimer->start();
    });
    connect(save, &QPushButton::clicked, this, &JournalPage::flush);
    auto shortcut = new QShortcut(QKeySequence::Save, this);
    connect(shortcut, &QShortcut::activated, this, &JournalPage::flush);
    connect(tabs, &QTabWidget::currentChanged, this, [this] { showPreview(); });
    connect(outline, &QPushButton::clicked, this, [this] {
        if (!editor->isReadOnly())
            editor->insertPlainText(
                "## 今日完成\n\n- \n\n## 收获与思考\n\n\n## 明日的方向\n\n- \n");
    });
    connect(today, &QPushButton::clicked, this, [this] { openDate(QDate::currentDate()); });
    connect(datePicker, &QDateEdit::dateChanged, this, [this](QDate date) { openDate(date); });
    connect(history, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!rebuilding)
            openDate(item->data(Qt::UserRole).toDate());
    });
    connect(reloadButton, &QPushButton::clicked, this, &JournalPage::reload);
    connect(exportButton, &QPushButton::clicked, this, [this] {
        const auto destination = QFileDialog::getSaveFileName(
            this, "另存总结副本",
            QDir::homePath() + "/" + current.toString("yyyy-MM-dd") + "-副本.md",
            "Markdown (*.md)");
        if (destination.isEmpty())
            return;
        if (QFileInfo(destination)
                .absoluteFilePath()
                .compare(QFileInfo(store.path(current)).absoluteFilePath(), Qt::CaseInsensitive) ==
            0) {
            updateStatus("副本请选择其他位置，当前总结请使用「保存」。");
            return;
        }
        QSaveFile file(destination);
        const auto bytes = editor->toPlainText().toUtf8();
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
            updateStatus("副本保存失败：" + file.errorString());
        else
            updateStatus("已另存副本 · 当前文档仍保留原来的保存状态");
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        if (!readable || (!snapshot.exists && !dirty))
            return;
        if (QMessageBox::question(
                this, "删除这一天的总结？",
                "已保存的文档会移入 document/.trash，可手动恢复。尚未保存的编辑将丢弃。",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
            return;
        QString error;
        if (!store.recycle(current, snapshot, &error)) {
            updateStatus("删除失败：" + error);
            return;
        }
        saveTimer->stop();
        dirty = false;
        snapshot = {};
        QSignalBlocker blocker(editor);
        editor->clear();
        showPreview();
        rebuildHistory();
        updateStatus("已移入回收目录 · 当前日期尚未创建新文档");
    });
    openDate(QDate::currentDate());
}
void JournalPage::updateStatus(const QString &message) {
    status->setText(message);
}
bool JournalPage::flush() {
    saveTimer->stop();
    if (!dirty)
        return true;
    QString error;
    const auto text = editor->toPlainText();
    if (!readable || !store.save(current, text, snapshot, &error)) {
        updateStatus("保存失败，编辑内容已保留：" + error);
        return false;
    }
    // Use the bytes just committed as our revision, never accept a later external edit as ours.
    if (snapshot.exists || !text.trimmed().isEmpty()) {
        snapshot.exists = true;
        snapshot.bytes = text.toUtf8();
        snapshot.text = text;
    }
    dirty = false;
    updateStatus(snapshot.exists ? "已保存 · " + current.toString("yyyy-MM-dd") + ".md · " +
                                       QTime::currentTime().toString("HH:mm:ss")
                                 : "尚未创建文档 · 有内容后自动保存");
    rebuildHistory();
    return true;
}
bool JournalPage::openDate(QDate date) {
    if (!date.isValid())
        return false;
    const QSignalBlocker blocker(datePicker);
    if (!flush()) {
        datePicker->setDate(current);
        rebuildHistory();
        return false;
    }
    JournalStore::Document loaded;
    QString error;
    if (!store.read(date, &loaded, &error)) {
        if (current.isValid()) {
            datePicker->setDate(current);
            updateStatus("读取失败：" + error);
            rebuildHistory();
            return false;
        }
        readable = false;
        editor->setReadOnly(true);
        current = date;
        datePicker->setDate(date);
        updateStatus("读取失败：" + error);
        return false;
    }
    current = date;
    snapshot = loaded;
    readable = true;
    editor->setReadOnly(false);
    datePicker->setDate(date);
    heading->setText(date.toString("M 月 d 日") + " · 每日总结");
    {
        QSignalBlocker blocked(editor);
        editor->setPlainText(loaded.text);
    }
    dirty = false;
    updateStatus(loaded.exists ? "已载入 · " + date.toString("yyyy-MM-dd") + ".md"
                               : "尚未创建文档 · 有内容后自动保存");
    showPreview();
    rebuildHistory();
    return true;
}
void JournalPage::showPreview() {
    preview->setMarkdown(editor->toPlainText());
}
void JournalPage::reload() {
    if (dirty && QMessageBox::question(
                     this, "重新读取文档？", "将丢弃当前尚未保存的编辑，读取磁盘上的版本。",
                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    JournalStore::Document loaded;
    QString error;
    if (!store.read(current, &loaded, &error)) {
        updateStatus("读取失败：" + error);
        return;
    }
    saveTimer->stop();
    dirty = false;
    openDate(current);
}
void JournalPage::rebuildHistory() {
    rebuilding = true;
    QSignalBlocker blocker(history);
    history->clear();
    const auto query = search->text().trimmed();
    const auto dates = store.dates();
    for (const auto date : dates) {
        JournalStore::Document document;
        QString error;
        const auto valid = store.read(date, &document, &error);
        if (!query.isEmpty() && !date.toString("yyyy-MM-dd").contains(query) &&
            (!valid || !document.text.contains(query, Qt::CaseInsensitive)))
            continue;
        QString title =
            valid ? document.text.section('\n', 0, 0).trimmed().left(36) : "文件暂不可读";
        if (title.isEmpty())
            title = "空白文档";
        auto item = new QListWidgetItem(date.toString("yyyy-MM-dd") + "\n" + title, history);
        item->setData(Qt::UserRole, date);
        item->setToolTip(store.path(date));
        if (date == current)
            history->setCurrentItem(item);
    }
    counter->setText(
        dates.isEmpty()
            ? "还没有总结。\n写下今天的第一行吧。"
            : QString("%1 篇总结 · 当前显示 %2 篇").arg(dates.size()).arg(history->count()));
    rebuilding = false;
}
