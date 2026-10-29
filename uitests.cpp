// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "icons.h"
#include "mainwindow.h"
#include "tasklist.h"
#include <QTemporaryDir>
#include <QtTest>
#include <QtWidgets>

class UiTests : public QObject {
    Q_OBJECT
    void isolateSettings(const QString &path) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, path);
    }
    QPushButton *findButton(QWidget &window, const QString &text) {
        for (auto button : window.findChildren<QPushButton *>())
            if (button->text() == text)
                return button;
        return nullptr;
    }
  private slots:
    void dailyWorkflow() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-test-XXXXXX");
        isolateSettings(dir.path());
        QString file = dir.filePath("plans.json");
        Planner initial(file);
        QVERIFY(initial.save());
        MainWindow window(file);
        window.show();
        QTest::qWait(30);
        auto input = window.findChild<QLineEdit *>("taskInput");
        auto list = window.findChild<QListView *>("taskList");
        QVERIFY(input);
        QVERIFY(list);
        input->setText(QStringLiteral("界面添加测试"));
        QTest::keyClick(input, Qt::Key_Return);
        Planner saved(file);
        QVERIFY(saved.load());
        QCOMPARE(saved.tasks.size(), 1);
        QCOMPARE(saved.tasks[0].title, QStringLiteral("界面添加测试"));
        QVERIFY(input->text().isEmpty());
        QCOMPARE(list->model()->rowCount(), 1);
        auto rect = list->visualRect(list->model()->index(0, 0));
        QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                          QPoint(rect.left() + 20, rect.center().y()));
        QTRY_VERIFY(([&] { return saved.load() && saved.tasks[0].done; })());
        for (auto edit : window.findChildren<QLineEdit *>())
            if (edit->placeholderText() == QStringLiteral("搜索当天目标"))
                edit->setText(QStringLiteral("不存在"));
        QTRY_COMPARE(list->model()->rowCount(), 0);
        QVERIFY(window.findChild<QLabel *>("empty")->isVisible());
        auto review = findButton(window, QStringLiteral("月度回顾"));
        QVERIFY(review);
        review->click();
        QCOMPARE(window.findChild<QStackedWidget *>()->currentIndex(), 1);
        bool rate = false;
        for (auto l : window.findChildren<QLabel *>("metricValue"))
            if (l->text() == "100%")
                rate = true;
        QVERIFY(rate);
        auto daily = findButton(window, QStringLiteral("每日计划"));
        QVERIFY(daily);
        daily->click();
        QCOMPARE(window.findChild<QStackedWidget *>()->currentIndex(), 0);
    }
    void largeListKeepsScroll() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-large-XXXXXX");
        isolateSettings(dir.path());
        QString file = dir.filePath("plans.json");
        Planner p(file);
        for (int i = 0; i < 500; ++i)
            p.add(QDate::currentDate(), QString("Task %1").arg(i), i % 3);
        QVERIFY(p.save());
        MainWindow window(file);
        window.show();
        QTest::qWait(30);
        auto list = window.findChild<QListView *>("taskList");
        QVERIFY(list);
        QCOMPARE(list->model()->rowCount(), 500);
        QVERIFY(list->findChildren<QCheckBox *>().isEmpty());
        list->scrollTo(list->model()->index(100, 0), QAbstractItemView::PositionAtCenter);
        QTest::qWait(20);
        int position = list->verticalScrollBar()->value();
        QVERIFY(position > 0);
        auto rect = list->visualRect(list->model()->index(100, 0));
        QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                          QPoint(rect.left() + 20, rect.center().y()));
        QTRY_VERIFY(list->model()->index(100, 0).data(Qt::CheckStateRole).toInt() == Qt::Checked);
        QCOMPARE(list->verticalScrollBar()->value(), position);
        QTest::keyClick(list, Qt::Key_Space);
        QTRY_VERIFY(list->model()->index(100, 0).data(Qt::CheckStateRole).toInt() == Qt::Unchecked);
    }
    void taskModelUpdatesOnlyChangedRows() {
        TaskListModel model;
        Task a{"a", "<b>literal</b>", QDate(2026, 10, 29), false, 1, 0};
        QSignalSpy reset(&model, &QAbstractItemModel::modelReset),
            changed(&model, &QAbstractItemModel::dataChanged);
        model.setTasks({a});
        QCOMPARE(reset.count(), 1);
        model.setTasks({a});
        QCOMPARE(changed.count(), 0);
        a.done = true;
        model.setTasks({a});
        QCOMPARE(changed.count(), 1);
        QCOMPARE(reset.count(), 1);
        QCOMPARE(model.index(0, 0).data().toString(), QString("<b>literal</b>"));
        QSignalSpy requested(&model, &TaskListModel::completionRequested);
        QVERIFY(model.setData(model.index(0, 0), Qt::Unchecked, Qt::CheckStateRole));
        QCOMPARE(requested.count(), 1);
        QVERIFY(!model.setData({}, Qt::Checked, Qt::CheckStateRole));
    }
    void editTaskWithDoubleClick() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-edit-XXXXXX");
        isolateSettings(dir.path());
        QString file = dir.filePath("plans.json");
        Planner p(file);
        p.add(QDate::currentDate(), "before", 0);
        QVERIFY(p.save());
        MainWindow window(file);
        window.show();
        QTest::qWait(30);
        auto list = window.findChild<QListView *>("taskList");
        auto rect = list->visualRect(list->model()->index(0, 0));
        QPoint point(rect.left() + 100, rect.top() + 25);
        bool edited = false;
        QTimer::singleShot(50, &window, [&] {
            auto dialog = window.findChild<QDialog *>();
            if (!dialog)
                return;
            auto edit = dialog->findChild<QLineEdit *>();
            if (!edit)
                return;
            edit->setText("<b>literal</b>");
            edited = true;
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
        });
        QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier, point);
        QTest::mouseDClick(list->viewport(), Qt::LeftButton, Qt::NoModifier, point);
        QTRY_VERIFY(edited);
        QVERIFY(p.load());
        QCOMPARE(p.tasks[0].title, QString("<b>literal</b>"));
    }
    void chromeAndSettings() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-chrome-XXXXXX");
        isolateSettings(dir.path());
        QString file = dir.filePath("plans.json");
        Planner p(file);
        QVERIFY(p.save());
        MainWindow window(file);
        window.show();
        QTest::qWait(20);
        QVERIFY(window.windowFlags().testFlag(Qt::FramelessWindowHint));
        QVERIFY(window.findChild<QWidget *>("titleBar"));
        QVERIFY(window.findChildren<QPushButton *>("square").isEmpty());
        for (auto l : window.findChildren<QLabel *>())
            QVERIFY(!l->text().contains("LOCAL FIRST"));
        QVERIFY(!findButton(window, QStringLiteral("打开数据文件夹")));
        QPushButton *maximize = nullptr;
        for (auto b : window.findChildren<QPushButton *>("chromeButton"))
            if (b->accessibleName() == QStringLiteral("最大化"))
                maximize = b;
        QVERIFY(maximize);
        maximize->click();
        QTest::qWait(20);
        QVERIFY(window.isMaximized());
        window.restoreWindow();
        QVERIFY(window.isMaximized());
        maximize->click();
        QTest::qWait(20);
        QVERIFY(!window.isMaximized());
        bool inspected = false;
        QTimer::singleShot(30, &window, [&] {
            auto dialog = window.findChild<QDialog *>();
            if (!dialog)
                return;
            auto tabs = dialog->findChild<QTabWidget *>();
            if (!tabs)
                return;
            QCOMPARE(tabs->count(), 2);
            tabs->setCurrentIndex(1);
            QVERIFY(findButton(*dialog, QStringLiteral("打开数据文件夹")));
            inspected = true;
            dialog->reject();
        });
        auto settings = window.findChild<QPushButton *>("weatherAction");
        QVERIFY(settings);
        settings->click();
        QVERIFY(inspected);
    }
    void themeDialog() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-theme-XXXXXX");
        isolateSettings(dir.path());
        QString file = dir.filePath("plans.json");
        Planner p(file);
        QVERIFY(p.save());
        MainWindow window(file);
        window.show();
        bool applied = false;
        QTimer::singleShot(30, &window, [&] {
            auto dialog = window.findChild<QDialog *>();
            if (!dialog)
                return;
            dialog->findChild<QComboBox *>()->setCurrentIndex(4);
            applied = true;
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
        });
        auto settings = window.findChild<QPushButton *>("weatherAction");
        QVERIFY(settings);
        settings->click();
        QVERIFY(applied);
        QVERIFY(qApp->styleSheet().contains("#171c24", Qt::CaseInsensitive));
    }
    void vectorIconsScale() {
        QVERIFY(QFile::exists(":/check.svg"));
        auto icon = lineIcon("close");
        auto normal = icon.pixmap(QSize(24, 24), 1);
        auto large = icon.pixmap(QSize(24, 24), 2);
        QVERIFY(!normal.isNull());
        QVERIFY(!large.isNull());
        QCOMPARE(large.size(), QSize(48, 48));
        QCOMPARE(large.devicePixelRatio(), 2.0);
    }
    void simplifiedNavigationAndCalendar() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-simple-XXXXXX");
        isolateSettings(dir.path());
        QString file = dir.filePath("plans.json");
        Planner p(file);
        QVERIFY(p.save());
        MainWindow window(file);
        window.show();
        QVERIFY(window.findChildren<QPushButton *>("settingsEntry").isEmpty());
        QVERIFY(window.findChildren<QWidget *>("utilities").isEmpty());
        auto exit = window.findChild<QPushButton *>("exitEntry");
        QVERIFY(exit);
        QVERIFY(exit->text().isEmpty());
        QVERIFY(!findButton(window, QStringLiteral("设置与偏好")));
        QVERIFY(findButton(window, QStringLiteral("上个月")));
        QVERIFY(findButton(window, QStringLiteral("下个月")));
        for (auto b : window.findChildren<QPushButton *>()) {
            QVERIFY(!b->text().contains(QChar(0x2039)));
            QVERIFY(!b->text().contains(QChar(0x203A)));
        }
        CalendarView calendar;
        calendar.resize(350, 330);
        calendar.theme = Theme::named("forest");
        calendar.month = QDate(2026, 10, 1);
        calendar.selected = QDate(2026, 10, 29);
        calendar.stats.dailyTotal.fill(0, 31);
        calendar.stats.dailyDone.fill(0, 31);
        auto without = calendar.grab().toImage();
        calendar.stats.dailyTotal[28] = 4;
        calendar.stats.dailyDone[28] = 3;
        auto with = calendar.grab().toImage();
        QCOMPARE(with, without);
    }
    void calendarSelectionStaysSquare() {
        CalendarView calendar;
        calendar.theme = Theme::named("sun");
        calendar.month = QDate(2026, 10, 1);
        calendar.selected = QDate(2026, 10, 15);
        for (const auto size : {QSize(270, 230), QSize(420, 360), QSize(600, 260)}) {
            calendar.resize(size);
            const auto image = calendar.grab().toImage();
            QRect colored;
            for (int y = 0; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x)
                    if (image.pixelColor(x, y) == calendar.theme.accent)
                        colored = colored.united(QRect(x, y, 1, 1));
            QVERIFY(!colored.isEmpty());
            QVERIFY(qAbs(colored.width() - colored.height()) <= 1);
            QVERIFY(colored.width() > 24);
            QDate clicked;
            calendar.onSelect = [&](QDate date) { clicked = date; };
            QTest::mouseClick(&calendar, Qt::LeftButton, Qt::NoModifier, colored.center());
            QCOMPARE(clicked, calendar.selected);
        }
    }
    void culturalThemesSaveAndApply() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-cultural-XXXXXX");
        isolateSettings(dir.path());
        QString file = dir.filePath("plans.json");
        Planner model(file);
        QVERIFY(model.save());
        for (const auto &id : QStringList{"chinese", "european", "american"}) {
            MainWindow window(file);
            window.show();
            bool applied = false;
            QTimer::singleShot(30, &window, [&] {
                auto dialog = window.findChild<QDialog *>();
                if (!dialog)
                    return;
                auto selector = dialog->findChild<QComboBox *>("themeSelector");
                if (!selector)
                    return;
                int index = selector->findData(id);
                QVERIFY(index >= 0);
                QVERIFY(!selector->itemIcon(index).isNull());
                selector->setCurrentIndex(index);
                auto automatic = dialog->findChild<QCheckBox *>();
                automatic->setChecked(false);
                applied = true;
                dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
            });
            auto settings = window.findChild<QPushButton *>("weatherAction");
            QVERIFY(settings);
            settings->click();
            QVERIFY(applied);
            const auto palette = Theme::named(id);
            QVERIFY(qApp->styleSheet().contains(palette.accent.name(), Qt::CaseInsensitive));
            auto text = window.findChild<QLabel *>("weatherText");
            QVERIFY(text);
            QCOMPARE(text->text(), Theme::displayName(id));
            QSettings stored(QSettings::IniFormat, QSettings::UserScope, "DaySequence",
                             "DaySequence");
            QCOMPARE(stored.value("theme").toString(), id);
        }
        const auto a = Theme::named("chinese"), b = Theme::named("european"),
                   c = Theme::named("american");
        QVERIFY(a.accent != b.accent);
        QVERIFY(b.accent != c.accent);
        QCOMPARE(Theme::presets().size(), 8);
    }
};
QTEST_MAIN(UiTests)
#include "uitests.moc"
