// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "mainwindow.h"
#include "choicebox.h"
#include "focusclock.h"
#include "storage.h"
#include "tasklist.h"
#include <QtWidgets>

#include <QNetworkProxyFactory>
#include <QSaveFile>
#include <QWindow>

#include "icons.h"
#include "uicomponents.h"
MainWindow::MainWindow(const QString &file, QWidget *parent, StorageManager *storage)
    : QMainWindow(parent), storageManager(storage), model(file),
      settings(QFileInfo(file).dir().filePath("settings.ini"), QSettings::IniFormat) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setWindowTitle("日序");
    setWindowIcon(appIcon());
    const auto screenArea = QGuiApplication::primaryScreen()->availableGeometry();
    resize(qMin(1260, screenArea.width() - 32), qMin(894, screenArea.height() - 32));
    setMinimumSize(980, 640);
    QNetworkProxyFactory::setUseSystemConfiguration(true);
    healthy = model.load();
    QString loadError = model.error;
    build();
    setupTray();
    applyTheme();
    refresh();
    if (settings.contains("windowGeometry"))
        restoreGeometry(settings.value("windowGeometry").toByteArray());
    auto dateTimer = new QTimer(this);
    dateTimer->setInterval(60000);
    connect(dateTimer, &QTimer::timeout, this, &MainWindow::updateChromeDate);
    dateTimer->start();
    if (!healthy)
        QTimer::singleShot(0, this, [this, loadError] {
            QMessageBox::warning(this, "数据读取失败",
                                 loadError + "\n为保护文件，暂时停止编辑。数据位置：\n" +
                                     model.filePath());
        });
    if (settings.value("weatherAuto", false).toBool())
        QTimer::singleShot(500, this, &MainWindow::fetchWeather);
    auto timer = new QTimer(this);
    timer->setInterval(30 * 60 * 1000);
    connect(timer, &QTimer::timeout, this, [this] {
        if (settings.value("weatherAuto", false).toBool())
            fetchWeather();
    });
    timer->start();
}
void MainWindow::build() {
    root = new QWidget;
    root->setObjectName("root");
    setCentralWidget(root);
    auto frame = new QVBoxLayout(root);
    frame->setContentsMargins(1, 1, 1, 1);
    frame->setSpacing(0);
    titleBar = new QFrame;
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(48);
    auto chrome = new QHBoxLayout(titleBar);
    chrome->setContentsMargins(18, 0, 8, 0);
    chrome->setSpacing(10);
    auto seal = label("序", "seal");
    seal->setFixedSize(27, 27);
    seal->setAlignment(Qt::AlignCenter);
    chrome->addWidget(seal);
    auto appName = label("日序", "chromeName");
    chrome->addWidget(appName);
    auto motto = label("一日一序  ·  从容有章", "chromeMotto");
    chrome->addWidget(motto);
    chrome->addStretch();
    auto dateText = label(QDate::currentDate().toString("yyyy 年 M 月 d 日"), "chromeDate");
    dateText->setProperty("date", QDate::currentDate());
    chrome->addWidget(dateText);
    chrome->addSpacing(18);
    auto minimize = button("", "chromeButton");
    minimize->setIcon(lineIcon("min"));
    minimize->setToolTip("最小化");
    minimize->setAccessibleName("最小化");
    maximizeButton = button("", "chromeButton");
    maximizeButton->setIcon(lineIcon("max"));
    maximizeButton->setToolTip("最大化");
    maximizeButton->setAccessibleName("最大化");
    auto closeButton = button("", "closeButton");
    closeButton->setIcon(lineIcon("close"));
    closeButton->setToolTip("关闭窗口，留在托盘");
    closeButton->setAccessibleName("关闭窗口");
    for (auto b : {minimize, maximizeButton, closeButton}) {
        b->setFixedSize(36, 32);
        b->setIconSize(QSize(24, 24));
        chrome->addWidget(b);
    }
    connect(minimize, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(maximizeButton, &QPushButton::clicked, this,
            [this] { isMaximized() ? showNormal() : showMaximized(); });
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
    frame->addWidget(titleBar);
    titleBar->installEventFilter(this);
    for (auto l : titleBar->findChildren<QLabel *>()) {
        l->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    auto mainArea = new QWidget;
    auto shell = new QHBoxLayout(mainArea);
    shell->setContentsMargins(0, 0, 0, 0);
    shell->setSpacing(0);
    frame->addWidget(mainArea, 1);
    qApp->installEventFilter(this);
    root->setMouseTracking(true);
    auto sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(180);
    auto side = column(sidebar, 16);
    side->setSpacing(10);
    side->addSpacing(16);
    auto brand = label("日序", "brand");
    side->addWidget(brand);
    side->addWidget(label("DAYSEQUENCE", "eyebrow"));
    side->addSpacing(40);
    dailyNav = button("每日计划", "nav");
    reviewNav = button("月度回顾", "nav");
    clockNav = button("专注时钟", "nav");
    clockNav->setAccessibleName("专注时钟");
    clockNav->setCheckable(true);
    dailyNav->setCheckable(true);
    reviewNav->setCheckable(true);
    side->addWidget(dailyNav);
    side->addWidget(reviewNav);
    side->addWidget(clockNav);
    side->addStretch();
    auto quote = label("让每一个小目标\n都有完成的回响。", "sideQuote");
    side->addWidget(quote);
    side->addSpacing(18);
    auto quit = button("", "exitEntry");
    quit->setIcon(lineIcon("power"));
    quit->setIconSize(QSize(24, 24));
    quit->setFixedSize(44, 44);
    quit->setToolTip("完全退出日序");
    quit->setAccessibleName("退出日序");
    side->addWidget(quit, 0, Qt::AlignLeft);
    connect(dailyNav, &QPushButton::clicked, this, [this] { selectPage(0); });
    connect(reviewNav, &QPushButton::clicked, this, [this] { selectPage(1); });
    connect(clockNav, &QPushButton::clicked, this, [this] { selectPage(2); });
    connect(quit, &QPushButton::clicked, this, &MainWindow::requestExit);
    shell->addWidget(sidebar);
    auto content = new QWidget;
    auto body = column(content, 24);
    body->setSpacing(22);
    shell->addWidget(content, 1);
    auto header = new QHBoxLayout;
    auto titles = new QVBoxLayout;
    titles->setSpacing(6);
    pageTitle = label("把今天，过得有条理。", "pageTitle");
    pageSubtitle = label("先安排重要的事，再一步一步完成。", "muted");
    titles->addWidget(pageTitle);
    titles->addWidget(pageSubtitle);
    header->addLayout(titles, 1);
    auto weather = card();
    weather->setObjectName("weather");
    weather->setFixedWidth(238);
    auto wl = column(weather, 14);
    wl->setSpacing(5);
    auto wh = new QHBoxLayout;
    weatherHeading = label("天气与配色", "eyebrow");
    wh->addWidget(weatherHeading, 1);
    weatherAction = button("", "weatherAction");
    weatherAction->setFixedSize(26, 24);
    weatherAction->setIconSize(QSize(18, 18));
    wh->addWidget(weatherAction);
    wl->addLayout(wh);
    weatherLabel = label("", "weatherText");
    weatherNote = label("", "weatherNote");
    wl->addWidget(weatherLabel);
    wl->addWidget(weatherNote);
    header->addWidget(weather);
    body->addLayout(header);
    weatherAction->setAccessibleName("外观与天气设置");
    connect(weatherAction, &QPushButton::clicked, this, &MainWindow::preferences);
    weatherAction->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(weatherAction, &QPushButton::customContextMenuRequested, this,
            [this](const QPoint &point) {
                if (!settings.value("weatherAuto", false).toBool())
                    return;
                QMenu menu;
                auto refresh = menu.addAction("刷新天气");
                refresh->setEnabled(!weatherBusy);
                if (menu.exec(weatherAction->mapToGlobal(point)) == refresh)
                    fetchWeather();
            });
    pages = new QStackedWidget;
    body->addWidget(pages, 1);
    auto daily = new QWidget;
    auto dl = new QHBoxLayout(daily);
    dl->setContentsMargins(0, 0, 0, 0);
    dl->setSpacing(20);
    auto calCard = card();
    calCard->setMinimumWidth(330);
    auto cal = column(calCard, 20);
    auto calHead = new QHBoxLayout;
    monthLabel = label("", "sectionTitle");
    calHead->addWidget(monthLabel, 1);
    auto chooseMonth = button("选择月份", "monthSelector");
    calHead->addWidget(chooseMonth);
    cal->addLayout(calHead);
    connect(chooseMonth, &QPushButton::clicked, this, [this] {
        QDialog dialog(this);
        dialog.setWindowTitle("选择月份");
        auto l = column(&dialog);
        l->addWidget(label("前往哪一个月？", "sectionTitle"));
        auto date = new QDateEdit(month);
        date->setDisplayFormat("yyyy 年 M 月");
        date->setButtonSymbols(QAbstractSpinBox::NoButtons);
        l->addWidget(date);
        auto box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        box->button(QDialogButtonBox::Ok)->setText("前往");
        box->button(QDialogButtonBox::Cancel)->setText("取消");
        l->addWidget(box);
        connect(box, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() == QDialog::Accepted) {
            month = QDate(date->date().year(), date->date().month(), 1);
            selected =
                QDate(month.year(), month.month(), qMin(selected.day(), month.daysInMonth()));
            refresh();
        }
    });
    cal->addWidget(label("选择日期，提前安排每一天", "muted"));
    calendar = new CalendarView;
    cal->addWidget(calendar, 1);
    auto divider = new QFrame;
    divider->setObjectName("divider");
    divider->setFixedHeight(1);
    cal->addWidget(divider);
    monthCaption = label("", "muted");
    cal->addWidget(monthCaption);
    auto calFoot = new QHBoxLayout;
    auto today = button("回到今天", "secondary"), summary = button("查看月度回顾  ↗", "secondary");
    calFoot->addWidget(today);
    calFoot->addWidget(summary);
    cal->addLayout(calFoot);
    dl->addWidget(calCard, 4);

    connect(today, &QPushButton::clicked, this, [this] {
        selected = QDate::currentDate();
        month = QDate(selected.year(), selected.month(), 1);
        refresh();
    });
    connect(summary, &QPushButton::clicked, this, [this] { selectPage(1); });
    calendar->onSelect = [this](QDate date) {
        selected = date;
        refresh();
    };
    auto tasks = card();
    tasks->setMinimumWidth(380);
    auto tl = column(tasks, 24);
    tl->setSpacing(12);
    auto taskHead = new QHBoxLayout;
    dayLabel = label("", "sectionTitle");
    taskHead->addWidget(dayLabel, 1);
    dayCaption = label("", "pill");
    taskHead->addWidget(dayCaption);
    tl->addLayout(taskHead);
    progressCaption = label("", "muted");
    tl->addWidget(progressCaption);
    dayProgress = new QProgressBar;
    dayProgress->setTextVisible(false);
    dayProgress->setFixedHeight(6);
    dayProgress->setRange(0, 100);
    tl->addWidget(dayProgress);
    auto entry = new QHBoxLayout;
    taskInput = new QLineEdit;
    taskInput->setPlaceholderText("写下一个想完成的目标…");
    taskInput->setMaxLength(200);
    taskInput->setObjectName("taskInput");
    auto add = button("＋ 添加", "primary");
    entry->addWidget(taskInput, 1);
    entry->addWidget(add);
    tl->addLayout(entry);
    auto controls = new QHBoxLayout;
    priority = new ChoiceBox;
    priority->setObjectName("prioritySelector");
    priority->setAccessibleName("新目标优先级");
    priority->addItems({"普通优先级", "重要", "紧急"});
    priority->setToolTip("新目标的优先级");
    sort = new ChoiceBox;
    sort->setObjectName("sortSelector");
    sort->setAccessibleName("目标排序");
    sort->addItems({"手动排序", "未完成优先", "优先级排序", "按名称排序"});
    sort->setCurrentIndex(qBound(0, settings.value("sort", 0).toInt(), 3));
    controls->addWidget(priority);
    controls->addStretch();
    controls->addWidget(sort);
    tl->addLayout(controls);
    search = new QLineEdit;
    search->setPlaceholderText("搜索当天目标");
    search->setClearButtonEnabled(true);
    tl->addWidget(search);
    taskList = new QListView;
    taskList->setObjectName("taskList");
    taskList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    taskList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    taskList->setUniformItemSizes(true);
    taskList->setMouseTracking(true);
    taskList->setContextMenuPolicy(Qt::CustomContextMenu);
    taskList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    taskModel = new TaskListModel(taskList);
    taskDelegate = new TaskDelegate(taskList);
    taskList->setModel(taskModel);
    taskList->setItemDelegate(taskDelegate);
    emptyTasks = label("", "empty");
    emptyTasks->setParent(taskList->viewport());
    emptyTasks->setAlignment(Qt::AlignCenter);
    emptyTasks->setWordWrap(true);
    emptyTasks->setAttribute(Qt::WA_TransparentForMouseEvents);
    taskList->viewport()->installEventFilter(this);
    tl->addWidget(taskList, 1);
    connect(
        taskModel, &TaskListModel::completionRequested, this,
        [this](const QString &id, bool done) {
            mutate([&] {
                if (auto task = model.find(id))
                    task->done = done;
            });
        },
        Qt::QueuedConnection);
    connect(taskDelegate, &TaskDelegate::menuRequested, this,
            [this](const QString &id, const QPoint &position) {
                QTimer::singleShot(0, this, [this, id, position] { showTaskMenu(id, position); });
            });
    connect(taskList, &QListView::doubleClicked, this, [this](const QModelIndex &index) {
        editTask(index.data(TaskListModel::IdRole).toString());
    });
    connect(taskList, &QListView::customContextMenuRequested, this, [this](const QPoint &point) {
        auto index = taskList->indexAt(point);
        if (index.isValid())
            showTaskMenu(index.data(TaskListModel::IdRole).toString(),
                         taskList->viewport()->mapToGlobal(point));
    });
    tl->addWidget(label("双击文字编辑  ·  手动排序时可用 ↑ ↓ 调整顺序", "smallMuted"));
    dl->addWidget(tasks, 5);
    pages->addWidget(scrollPage(daily));
    connect(add, &QPushButton::clicked, this, &MainWindow::addTask);
    connect(taskInput, &QLineEdit::returnPressed, this, &MainWindow::addTask);
    connect(sort, &QComboBox::currentIndexChanged, this, [this](int i) {
        settings.setValue("sort", i);
        refreshTasks();
    });
    searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(100);
    connect(searchTimer, &QTimer::timeout, this, &MainWindow::refreshTasks);
    connect(search, &QLineEdit::textChanged, this, [this] { searchTimer->start(); });
    auto review = new QWidget;
    auto rl = new QVBoxLayout(review);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(18);
    auto reviewHead = new QHBoxLayout;
    reviewMonth = label("", "sectionTitle");
    reviewHead->addWidget(reviewMonth, 1);
    auto rp = button("上个月", "secondary"), rn = button("下个月", "secondary"),
         exportButton = button("导出本月 CSV", "secondary");
    reviewHead->addWidget(rp);
    reviewHead->addWidget(rn);
    reviewHead->addWidget(exportButton);
    rl->addLayout(reviewHead);
    connect(rp, &QPushButton::clicked, this, [this] { changeMonth(-1); });
    connect(rn, &QPushButton::clicked, this, [this] { changeMonth(1); });
    connect(exportButton, &QPushButton::clicked, this, [this] {
        auto file = QFileDialog::getSaveFileName(
            this, "导出月度计划", QDir::homePath() + "/日序-" + month.toString("yyyy-MM") + ".csv",
            "CSV 文件 (*.csv)");
        if (file.isEmpty())
            return;
        QSaveFile f(file);
        auto data = model.monthCsv(month);
        if (!f.open(QIODevice::WriteOnly) || f.write(data) != data.size() || !f.commit())
            QMessageBox::warning(this, "导出失败", f.errorString());
        else
            saveStatus->setText("本月计划已导出");
    });
    auto metrics = new QHBoxLayout;
    metrics->setSpacing(14);
    QStringList metricNames = {"目标总数", "已完成目标", "目标完成率", "全部完成天数"};
    for (const auto &name : metricNames) {
        auto c = card();
        auto cl = column(c, 18);
        cl->setSpacing(8);
        cl->addWidget(label(name, "muted"));
        auto value = label("", "metricValue"), note = label("", "smallMuted");
        metricValues.append(value);
        metricNotes.append(note);
        cl->addWidget(value);
        cl->addWidget(note);
        metrics->addWidget(c, 1);
    }
    rl->addLayout(metrics);
    auto charts = new QHBoxLayout;
    charts->setSpacing(18);
    auto barCard = card(), pieCard = card();
    auto bl = column(barCard, 20), pl = column(pieCard, 20);
    bl->addWidget(label("每天，前进一步", "sectionTitle"));
    bl->addWidget(label("每日目标数量  ·  深色已完成 / 浅色未完成", "smallMuted"));
    bars = new ChartView(false);
    bl->addWidget(bars, 1);
    pl->addWidget(label("完成的每一份努力", "sectionTitle"));
    pl->addWidget(label("整月目标完成占比", "smallMuted"));
    donut = new ChartView(true);
    pl->addWidget(donut, 1);
    charts->addWidget(barCard, 3);
    charts->addWidget(pieCard, 2);
    rl->addLayout(charts, 1);
    auto insightCard = card();
    auto il = column(insightCard, 18);
    il->addWidget(label("这个月的小结", "eyebrow"));
    insight = label("", "insight");
    insight->setWordWrap(true);
    il->addWidget(insight);
    rl->addWidget(insightCard);
    pages->addWidget(scrollPage(review));
    focusClock = new FocusClock(settings);
    pages->addWidget(scrollPage(focusClock));
    connect(focusClock, &FocusClock::completed, this, [this](bool sound) {
        if (sound)
            QApplication::beep();
        if (tray && tray->isVisible())
            tray->showMessage("专注完成", "这一段专注已完成，起身活动一下吧。",
                              QSystemTrayIcon::Information, 5000);
        QApplication::alert(this, 5000);
    });
    auto foot = new QHBoxLayout;
    saveStatus = label("●  所有更改自动保存在本机", "smallMuted");
    foot->addWidget(saveStatus);
    foot->addStretch();
    body->addLayout(foot);
    auto shortcut = new QShortcut(QKeySequence("Ctrl+N"), this);
    connect(shortcut, &QShortcut::activated, this, [this] {
        selectPage(0);
        taskInput->setFocus();
    });
    selectPage(0);
}
void MainWindow::addTask() {
    if (taskInput->text().trimmed().isEmpty()) {
        taskInput->setFocus();
        return;
    }
    auto title = taskInput->text();
    if (mutate([&] { model.add(selected, title, priority->currentIndex()); })) {
        taskInput->clear();
        search->clear();
    }
    taskInput->setFocus();
}
bool MainWindow::mutate(const std::function<void()> &action) {
    if (!healthy) {
        QMessageBox::warning(this, "暂时无法编辑",
                             "请先修复数据文件或恢复备份，然后重新启动日序。");
        return false;
    }
    auto backup = model.tasks;
    action();
    if (!model.save()) {
        model.tasks = backup;
        saveStatus->setProperty("ok", false);
        saveStatus->setText("保存失败 · 本次更改已撤回");
        QMessageBox::warning(this, "保存失败", model.error);
    } else {
        saveStatus->setProperty("ok", true);
        saveStatus->setText("●  已保存到本机 · " + QTime::currentTime().toString("HH:mm"));
    }
    refresh();
    return saveStatus->property("ok").toBool();
}
void MainWindow::refresh() {
    monthLabel->setText(month.toString("yyyy年 M月"));
    reviewMonth->setText(month.toString("yyyy年 M月") + " · 月度回顾");
    dayLabel->setText(
        selected.toString("M月d日") + "  " +
        QLocale(QLocale::Chinese).dayName(selected.dayOfWeek(), QLocale::ShortFormat));
    dayCaption->setText(selected == QDate::currentDate()  ? "今天"
                        : selected > QDate::currentDate() ? "提前计划"
                                                          : "回顾这一天");
    auto stats = model.stats(month);
    calendar->month = month;
    calendar->selected = selected;
    calendar->stats = stats;
    calendar->theme = theme;
    calendar->update();
    monthCaption->setText(
        QString("本月已完成 %1 项目标  ·  %2 天有计划").arg(stats.done).arg(stats.plannedDays));
    bars->stats = stats;
    bars->month = month;
    bars->theme = theme;
    bars->update();
    donut->stats = stats;
    donut->month = month;
    donut->theme = theme;
    donut->update();
    QStringList values = {QString::number(stats.total), QString::number(stats.done),
                          QString::number(stats.rate()) + "%", QString::number(stats.completeDays)};
    QStringList notes = {
        QString("%1 天安排了目标").arg(stats.plannedDays),
        QString("还有 %1 项待完成").arg(stats.total - stats.done), "按目标数量计算",
        QString("占有计划天数的 %1%")
            .arg(stats.plannedDays ? qRound(stats.completeDays * 100.0 / stats.plannedDays) : 0)};
    for (int i = 0; i < 4; i++) {
        metricValues[i]->setText(values[i]);
        metricNotes[i]->setText(notes[i]);
    }
    if (!stats.total)
        insight->setText("本月还没有计划。挑选一天，写下一个小目标，回顾会从这里开始。");
    else {
        int best = 0;
        for (int i = 1; i < stats.dailyDone.size(); i++)
            if (stats.dailyDone[i] > stats.dailyDone[best])
                best = i;
        insight->setText(
            stats.done
                ? QString("这个月，你在 %1 天里安排了 %2 项目标，完成了 %3 "
                          "项。%4月%5日完成最多，共 %6 项。每一步都算数。")
                      .arg(stats.plannedDays)
                      .arg(stats.total)
                      .arg(stats.done)
                      .arg(month.month())
                      .arg(best + 1)
                      .arg(stats.dailyDone[best])
                : QString("这个月已经安排了 %1 项目标。完成第一项后，你的进展就会出现在这里。")
                      .arg(stats.total));
    }
    refreshTasks();
}
void MainWindow::refreshTasks() {
    const auto all = model.forDate(selected);
    int done = 0;
    for (const auto &task : all)
        if (task.done)
            ++done;
    progressCaption->setText(all.isEmpty() ? "今天想做些什么？从一个小目标开始。"
                                           : QString("已完成 %1 / %2 项  ·  %3% 的计划已实现")
                                                 .arg(done)
                                                 .arg(all.size())
                                                 .arg(qRound(100.0 * done / all.size())));
    dayProgress->setValue(all.isEmpty() ? 0 : qRound(done * 100.0 / all.size()));
    const auto filter = search->text();
    const int sortMode = sort->currentIndex();
    const auto filtered = model.forDate(selected, sortMode, filter);
    bool preserve =
        taskViewDate == selected && taskViewFilter == filter && taskViewSort == sortMode;
    const int scroll = preserve ? taskList->verticalScrollBar()->value() : 0;
    const QString id =
        preserve ? taskList->currentIndex().data(TaskListModel::IdRole).toString() : QString();
    taskModel->setTasks(filtered);
    taskList->setCurrentIndex(taskModel->indexForId(id));
    taskList->doItemsLayout();
    taskList->verticalScrollBar()->setValue(scroll);
    emptyTasks->setText(all.isEmpty() ? "给这一天留一个小目标\n从上方输入框开始"
                                      : "没有找到匹配的目标");
    emptyTasks->setGeometry(taskList->viewport()->rect());
    emptyTasks->setVisible(filtered.isEmpty());
    taskViewDate = selected;
    taskViewFilter = filter;
    taskViewSort = sortMode;
}
void MainWindow::showTaskMenu(const QString &id, const QPoint &position) {
    if (!model.find(id))
        return;
    QMenu menu;
    auto edit = menu.addAction("编辑目标");
    auto up = menu.addAction("↑ 上移"), down = menu.addAction("↓ 下移");
    bool manual = sort->currentIndex() == 0 && search->text().isEmpty();
    auto rows = model.forDate(selected);
    int row = -1;
    for (int i = 0; i < rows.size(); ++i)
        if (rows[i].id == id) {
            row = i;
            break;
        }
    up->setEnabled(manual && row > 0);
    down->setEnabled(manual && row >= 0 && row + 1 < rows.size());
    menu.addSeparator();
    auto remove = menu.addAction("删除目标");
    auto chosen = menu.exec(position);
    if (!chosen)
        return;
    if (chosen == edit)
        editTask(id);
    else if (chosen == up || chosen == down)
        mutate([&] { model.move(id, chosen == up ? -1 : 1); });
    else if (chosen == remove &&
             QMessageBox::question(this, "删除目标", "确定删除这个目标吗？") == QMessageBox::Yes)
        mutate([&] { model.remove(id); });
}
void MainWindow::editTask(const QString &id) {
    auto t = model.find(id);
    if (!t)
        return;
    QDialog dialog(this);
    dialog.setWindowTitle("编辑目标");
    dialog.setMinimumWidth(420);
    auto l = column(&dialog);
    l->addWidget(label("让目标更清晰", "sectionTitle"));
    auto input = new QLineEdit(t->title);
    input->setMaxLength(200);
    auto pr = new ChoiceBox;
    pr->addItems({"普通", "重要", "紧急"});
    pr->setCurrentIndex(t->priority);
    auto date = new QDateEdit(t->date);
    date->setCalendarPopup(true);
    date->setDisplayFormat("yyyy年M月d日");
    l->addWidget(input);
    l->addWidget(pr);
    l->addWidget(date);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setText("保存");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    l->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (!input->text().trimmed().isEmpty())
            dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted)
        mutate([&] {
            if (auto p = model.find(id)) {
                auto target = date->date();
                model.reschedule(id, target);
                p->title = input->text().trimmed();
                p->priority = pr->currentIndex();
            }
        });
}
void MainWindow::selectPage(int page) {
    pages->setCurrentIndex(page);
    dailyNav->setChecked(page == 0);
    reviewNav->setChecked(page == 1);
    clockNav->setChecked(page == 2);
    const QStringList titles = {"把今天，过得有条理。", "每一份努力，都看得见。",
                                "把此刻，留给一件事。"};
    const QStringList subtitles = {"先安排重要的事，再一步一步完成。",
                                   "回看一个月的节奏，为下个月留一点灵感。",
                                   "设定一段时间，让注意力慢慢沉淀。"};
    pageTitle->setText(titles.value(page));
    pageSubtitle->setText(subtitles.value(page));
}
void MainWindow::changeMonth(int delta) {
    month = month.addMonths(delta);
    selected = QDate(month.year(), month.month(), qMin(selected.day(), month.daysInMonth()));
    refresh();
}
void MainWindow::setupTray() {
    tray = new QSystemTrayIcon(windowIcon(), this);
    tray->setToolTip("日序 · 计划每一天");
    auto menu = new QMenu(this);
    menu->addAction("打开日序", this, [this] { restoreWindow(); });
    menu->addAction("退出日序", this, &MainWindow::requestExit);
    tray->setContextMenu(menu);
    connect(tray, &QSystemTrayIcon::messageClicked, this, [this] {
        selectPage(2);
        restoreWindow();
    });
    connect(tray, &QSystemTrayIcon::activated, this, [this](auto reason) {
        if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
            restoreWindow();
        }
    });
    if (QSystemTrayIcon::isSystemTrayAvailable())
        tray->show();
    weatherTheme = settings.value("weatherTheme", "forest").toString();
    if (settings.value("weatherAuto", false).toBool() &&
        settings.value("weatherCity").toString() == settings.value("city").toString()) {
        weatherText = settings.value("weatherText", "天气稍后更新").toString();
        weatherDetail = settings.value("weatherDetail", "正在等待天气更新").toString();
    }
}
void MainWindow::requestExit() {
    if (quitting)
        return;
    quitting = true;
    settings.setValue("windowGeometry", saveGeometry());
    settings.sync();
    cancelWeatherRequest();
    tray->hide();
    hide();
    emit exitRequested();
}
void MainWindow::closeEvent(QCloseEvent *event) {
    if (!quitting && tray && tray->isVisible()) {
        settings.setValue("windowGeometry", saveGeometry());
        settings.sync();
        hide();
        event->ignore();
        return;
    }
    requestExit();
    event->accept();
}
void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        if (maximizeButton) {
            maximizeButton->setIcon(lineIcon(isMaximized() ? "restore" : "max", theme.muted));
            maximizeButton->setToolTip(isMaximized() ? "还原窗口" : "最大化");
        }
    }
}
bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (taskList && watched == taskList->viewport() && event->type() == QEvent::Resize)
        emptyTasks->setGeometry(taskList->viewport()->rect());
    auto widget = qobject_cast<QWidget *>(watched);
    if (widget && widget->window() == this &&
        (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove ||
         event->type() == QEvent::MouseButtonDblClick)) {
        auto mouse = static_cast<QMouseEvent *>(event);
        QPoint pos = mapFromGlobal(mouse->globalPosition().toPoint());
        Qt::Edges edges;
        if (!isMaximized()) {
            if (pos.x() < 6)
                edges |= Qt::LeftEdge;
            else if (pos.x() >= width() - 6)
                edges |= Qt::RightEdge;
            if (pos.y() < 6)
                edges |= Qt::TopEdge;
            else if (pos.y() >= height() - 6)
                edges |= Qt::BottomEdge;
        }
        if (event->type() == QEvent::MouseButtonPress && mouse->button() == Qt::LeftButton &&
            edges && windowHandle()) {
            windowHandle()->startSystemResize(edges);
            return true;
        }
        if (watched == titleBar) {
            if (event->type() == QEvent::MouseButtonDblClick && mouse->button() == Qt::LeftButton) {
                isMaximized() ? showNormal() : showMaximized();
                return true;
            }
            if (event->type() == QEvent::MouseButtonPress && mouse->button() == Qt::LeftButton &&
                windowHandle()) {
                windowHandle()->startSystemMove();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
void MainWindow::restoreWindow() {
    if (isMinimized())
        setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
}
void MainWindow::updateChromeDate() {
    const QDate today = QDate::currentDate();
    for (auto date : findChildren<QLabel *>("chromeDate"))
        if (date->property("date").toDate() != today) {
            date->setText(today.toString("yyyy 年 M 月 d 日"));
            date->setProperty("date", today);
            refresh();
        }
}
void MainWindow::preview(const QString &folder) {
    QDir().mkpath(folder);
    model.tasks.clear();
    month = QDate::currentDate();
    month = QDate(month.year(), month.month(), 1);
    selected = QDate(month.year(), month.month(), qMin(18, month.daysInMonth()));
    for (int day = 1; day <= selected.day(); day++) {
        if (day % 7 == 0)
            continue;
        for (int j = 0; j < 2 + day % 4; j++) {
            auto id =
                model.add(QDate(month.year(), month.month(), day),
                          QStringList{"梳理今天的工作重点", "完成产品设计方案", "阅读 20 分钟",
                                      "整理本周工作笔记", "运动半小时"}[j % 5],
                          j == 1 ? 1 : 0);
            model.find(id)->done = (j < (day % 5 == 0 ? 1 : 3));
        }
    }
    refresh();
    selectPage(0);
    resize(1260, 894);
    show();
    QApplication::processEvents();
    grab().save(folder + "/daily.png");
    selectPage(1);
    QApplication::processEvents();
    grab().save(folder + "/monthly.png");
    selectPage(0);
    resize(1024, 700);
    QApplication::processEvents();
    grab().save(folder + "/compact.png");
    selectPage(2);
    QApplication::processEvents();
    grab().save(folder + "/clock-compact.png");
    resize(1260, 894);
    selectPage(2);
    QApplication::processEvents();
    grab().save(folder + "/clock.png");
    for (const auto &id : QStringList{"chinese", "european", "american"}) {
        settings.setValue("theme", id);
        settings.setValue("weatherAuto", false);
        applyTheme();
        selectPage(0);
        QApplication::processEvents();
        grab().save(folder + "/theme-" + id + ".png");
        selectPage(1);
        QApplication::processEvents();
        grab().save(folder + "/theme-" + id + "-monthly.png");
        selectPage(2);
        QApplication::processEvents();
        grab().save(folder + "/clock-" + id + ".png");
    }
    settings.setValue("theme", "forest");
    settings.setValue("weatherAuto", false);
    applyTheme();
    selectPage(0);
    QApplication::processEvents();
    if (auto selector = qobject_cast<ChoiceBox *>(sort)) {
        selector->showPopup();
        QApplication::processEvents();
        auto popup = selector->findChild<QFrame *>("choicePopup");
        auto image = grab();
        if (popup) {
            popup->grab().save(folder + "/sort-choices.png");
            QPainter painter(&image);
            painter.drawPixmap(mapFromGlobal(popup->pos()), popup->grab());
        }
        image.save(folder + "/sort-popup.png");
        selector->hidePopup();
    }
    QTimer::singleShot(0, this, [this, folder] {
        if (auto dialog = findChild<QDialog *>()) {
            dialog->grab().save(folder + "/appearance-settings.png");
            if (auto tabs = dialog->findChild<QTabWidget *>()) {
                tabs->setCurrentIndex(1);
                QApplication::processEvents();
                dialog->grab().save(folder + "/storage-settings.png");
            }
            dialog->reject();
        }
    });
    StorageManager previewStorage(QFileInfo(model.filePath()).absolutePath());
    auto originalStorage = storageManager;
    storageManager = &previewStorage;
    preferences();
    storageManager = originalStorage;
}
