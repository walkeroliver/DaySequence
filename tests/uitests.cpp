// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "choicebox.h"
#include "focusalarm.h"
#include "focusclock.h"
#include "icons.h"
#include "mainwindow.h"
#include "storage.h"
#include "tasklist.h"
#include <QAudioDevice>
#include <QMediaDevices>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>
#include <QtWidgets>

class UiTests : public QObject {
    Q_OBJECT
    QPushButton *findButton(QWidget &window, const QString &text) {
        for (auto button : window.findChildren<QPushButton *>())
            if (button->text() == text)
                return button;
        return nullptr;
    }
  private slots:
    void unavailableWeatherUsesCenteredTipAndKeepsManualTheme() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-weather-unavailable-XXXXXX");
        {
            QSettings previous(dir.filePath("settings.ini"), QSettings::IniFormat);
            previous.setValue("weatherAuto", true);
            previous.setValue("theme", "chinese");
            previous.setValue("weatherTheme", "ocean");
            previous.setValue("city", "Beijing");
            previous.sync();
        }
        MainWindow window(dir.filePath("tasks.json"));
        window.show();
        QTest::qWait(550);
        QCOMPARE(qApp->property("daysequenceTheme").toString(), QString("chinese"));
        QTimer::singleShot(30, &window, [&] {
            auto dialog = window.findChild<QDialog *>();
            QVERIFY(dialog);
            auto toggle = dialog->findChild<QCheckBox *>("weatherAutoToggle");
            auto city = dialog->findChild<QLineEdit *>("weatherCityInput");
            QVERIFY(toggle && city);
            QVERIFY(!toggle->isChecked());
            QVERIFY(!city->isEnabled());
            toggle->click();
            QVERIFY(!toggle->isChecked());
            auto tip = dialog->findChild<QLabel *>("centerTip");
            QVERIFY(tip && tip->isVisible());
            QVERIFY(tip->text().contains(QStringLiteral("暂不可用")));
            QVERIFY((tip->geometry().center() - dialog->rect().center()).manhattanLength() <= 2);
            QVERIFY(!window.findChild<QMessageBox *>());
            QTest::qWait(2850);
            QVERIFY(!tip->isVisible());
            toggle->click();
            QCOMPARE(dialog->findChildren<QLabel *>("centerTip").size(), 1);
            QVERIFY(tip->isVisible());
            QVERIFY(!toggle->isChecked());
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
        });
        window.findChild<QPushButton *>("weatherAction")->click();
        QSettings saved(dir.filePath("settings.ini"), QSettings::IniFormat);
        QVERIFY(!saved.value("weatherAuto").toBool());
        QCOMPARE(saved.value("theme").toString(), QString("chinese"));
    }
    void focusClockRunsAcrossPagesAndInBackground() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-focus-XXXXXX");
        MainWindow window(dir.filePath("tasks.json"));
        window.resize(1024, 700);
        window.show();
        auto clock = window.findChild<FocusClock *>();
        QVERIFY(clock);
        auto nav = findButton(window, QStringLiteral("专注时钟"));
        QVERIFY(nav);
        nav->click();
        auto hours = clock->findChild<QSpinBox *>("clockHours");
        auto minutes = clock->findChild<QSpinBox *>("clockMinutes");
        auto seconds = clock->findChild<QSpinBox *>("clockSeconds");
        auto start = clock->findChild<QPushButton *>("clockStart");
        auto reset = clock->findChild<QPushButton *>("clockReset");
        QVERIFY(hours && minutes && seconds && start && reset);
        clock->findChild<QCheckBox *>("clockSound")->setChecked(false);
        hours->setValue(0);
        minutes->setValue(0);
        seconds->setValue(0);
        QVERIFY(!start->isEnabled());
        seconds->setValue(1);
        QVERIFY(start->isEnabled());
        QSignalSpy completed(clock, &FocusClock::completed);
        start->click();
        QCOMPARE(clock->countdown()->state(), Countdown::State::Running);
        QVERIFY(!minutes->isEnabled());
        start->click();
        QCOMPARE(clock->countdown()->state(), Countdown::State::Paused);
        auto remaining = clock->countdown()->remaining();
        QTest::qWait(130);
        QCOMPARE(clock->countdown()->remaining(), remaining);
        start->click();
        findButton(window, QStringLiteral("每日计划"))->click();
        window.hide();
        QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 2500);
        QCOMPARE(clock->countdown()->state(), Countdown::State::Finished);
        window.restoreWindow();
        nav->click();
        reset->click();
        QCOMPARE(clock->countdown()->state(), Countdown::State::Ready);
        QCOMPARE(clock->countdown()->remaining(), qint64(1000));
        QVERIFY(minutes->isEnabled());
        auto dial = clock->findChild<QWidget *>("focusDial");
        QVERIFY(dial);
        QVERIFY(window.rect().contains(dial->mapTo(&window, dial->rect().bottomRight())));
    }
    void focusDurationAndSoundPreferencesPersist() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-focus-prefs-XXXXXX");
        {
            QSettings preferences(dir.filePath("settings.ini"), QSettings::IniFormat);
            FocusClock clock(preferences);
            for (auto preset : clock.findChildren<QPushButton *>("clockPreset"))
                if (preset->text() == QStringLiteral("45 分"))
                    preset->click();
            QCOMPARE(clock.countdown()->duration(), qint64(45 * 60 * 1000));
            clock.findChild<QCheckBox *>("clockSound")->setChecked(false);
            preferences.sync();
        }
        QSettings preferences(dir.filePath("settings.ini"), QSettings::IniFormat);
        FocusClock reopened(preferences);
        QCOMPARE(reopened.countdown()->duration(), qint64(45 * 60 * 1000));
        QVERIFY(!reopened.findChild<QCheckBox *>("clockSound")->isChecked());
    }
    void focusAudioImportAndSettingsPersist() {
        QTemporaryDir dir;
        const auto source = dir.filePath("source.wav");
        QFile builtin(":/sounds/chime.wav");
        QVERIFY(builtin.open(QIODevice::ReadOnly));
        {
            QFile out(source);
            QVERIFY(out.open(QIODevice::WriteOnly));
            out.write(builtin.readAll());
        }
        QString imported;
        {
            QSettings prefs(dir.filePath("settings.ini"), QSettings::IniFormat);
            FocusClock clock(prefs);
            QString error;
            QVERIFY2(clock.importAudio(source, &error), qPrintable(error));
            imported = prefs.value("focusAudioFile").toString();
            QVERIFY(QFile::exists(dir.filePath(imported)));
            QVERIFY(QFile::remove(source));
            QVERIFY(QFile::exists(dir.filePath(imported)));
            clock.findChild<QCheckBox *>("clockLoopSound")->setChecked(true);
            clock.findChild<QSlider *>("clockVolume")->setValue(35);
            QVERIFY(!clock.importAudio(dir.filePath("absent.mp3"), &error));
            QCOMPARE(prefs.value("focusAudioFile").toString(), imported);
            prefs.sync();
        }
        QSettings prefs(dir.filePath("settings.ini"), QSettings::IniFormat);
        FocusClock reopened(prefs);
        QCOMPARE(reopened.findChild<QComboBox *>("clockAudioChoice")->currentData().toString(),
                 imported);
        QVERIFY(reopened.findChild<QCheckBox *>("clockLoopSound")->isChecked());
        QCOMPARE(reopened.findChild<QSlider *>("clockVolume")->value(), 35);
    }
    void focusMusicLibraryKeepsMultipleTracksAndDiscoversFiles() {
        QTemporaryDir dir;
        QSettings prefs(dir.filePath("settings.ini"), QSettings::IniFormat);
        FocusClock clock(prefs);
        auto choices = clock.findChild<QComboBox *>("clockAudioChoice");
        QCOMPARE(choices->count(), 3);
        QVERIFY(QFile::exists(dir.filePath("music/chime.wav")));
        auto write = [](const QString &path, const QByteArray &bytes) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
        };
        QVERIFY(QDir().mkpath(dir.filePath("one")));
        QVERIFY(QDir().mkpath(dir.filePath("two")));
        QVERIFY(write(dir.filePath("one/favorite.wav"), "first track"));
        QVERIFY(write(dir.filePath("two/favorite.wav"), "second track"));
        QString error;
        QVERIFY(clock.importAudio(dir.filePath("one/favorite.wav"), &error));
        QVERIFY(clock.importAudio(dir.filePath("two/favorite.wav"), &error));
        QVERIFY(QFile::exists(dir.filePath("music/favorite.wav")));
        QVERIFY(QFile::exists(dir.filePath("music/favorite (2).wav")));
        QCOMPARE(choices->count(), 5);
        QVERIFY(clock.importAudio(dir.filePath("two/favorite.wav"), &error));
        QCOMPARE(choices->count(), 5); // Identical re-import does not duplicate.
        QVERIFY(write(dir.filePath("music/自己喜欢的音乐.MP3"), "manual track"));
        clock.countdown()->start();
        clock.reloadMusic();
        QCOMPARE(clock.countdown()->state(), Countdown::State::Running);
        QVERIFY(choices->findData("music/自己喜欢的音乐.MP3") >= 0);
        QCOMPARE(choices->currentData().toString(), QString("music/favorite (2).wav"));
        clock.countdown()->reset();
    }
    void focusAlarmLoopsAndStopsWithoutResettingTimer() {
        if (QMediaDevices::defaultAudioOutput().isNull())
            QSKIP("Audio output unavailable");
        QTemporaryDir dir;
        QSettings prefs(dir.filePath("settings.ini"), QSettings::IniFormat);
        prefs.setValue("focusSeconds", 1);
        prefs.setValue("focusLoopSound", true);
        FocusClock clock(prefs);
        clock.findChild<QAudioOutput *>()->setMuted(true);
        auto start = clock.findChild<QPushButton *>("clockStart");
        start->click();
        QTRY_VERIFY_WITH_TIMEOUT(clock.ringing(), 2500);
        auto player = clock.findChild<QMediaPlayer *>();
        QCOMPARE(player->loops(), int(QMediaPlayer::Infinite));
        QTRY_COMPARE_WITH_TIMEOUT(player->playbackState(), QMediaPlayer::PlayingState, 5000);
        clock.hide();
        QTest::qWait(3800);
        QVERIFY(clock.ringing());
        clock.stopAlarm();
        QVERIFY(!clock.ringing());
        QCOMPARE(clock.countdown()->state(), Countdown::State::Finished);
        QCOMPARE(player->playbackState(), QMediaPlayer::StoppedState);
        clock.findChild<QCheckBox *>("clockLoopSound")->setChecked(false);
        start->click();
        QTRY_VERIFY_WITH_TIMEOUT(clock.ringing(), 2500);
        QCOMPARE(player->loops(), int(QMediaPlayer::Once));
        QTRY_VERIFY_WITH_TIMEOUT(!clock.ringing(), 6500);
    }
    void focusMissingAudioFallsBackAndPreviewNeverLoops() {
        if (QMediaDevices::defaultAudioOutput().isNull())
            QSKIP("Audio output unavailable");
        FocusAlarm alarm;
        alarm.setVolume(0);
        QSignalSpy errors(&alarm, &FocusAlarm::failed);
        alarm.play(QUrl::fromLocalFile("Z:/daysequence-missing-sound.wav"), true, true);
        auto player = alarm.findChild<QMediaPlayer *>();
        QVERIFY(alarm.previewing());
        QCOMPARE(player->loops(), int(QMediaPlayer::Once));
        QCOMPARE(player->source(), FocusAlarm::preset("chime"));
        QCOMPARE(errors.count(), 1);
        alarm.stop();
        QVERIFY(!alarm.active());
    }
    void dailyWorkflow() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-test-XXXXXX");
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
        QVERIFY(!findButton(window, QStringLiteral("打开文件夹")));
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
            QVERIFY(findButton(*dialog, QStringLiteral("打开文件夹")));
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
            QSettings stored(dir.filePath("settings.ini"), QSettings::IniFormat);
            QCOMPARE(stored.value("theme").toString(), id);
        }
        const auto a = Theme::named("chinese"), b = Theme::named("european"),
                   c = Theme::named("american");
        QVERIFY(a.accent != b.accent);
        QVERIFY(b.accent != c.accent);
        QCOMPARE(Theme::presets().size(), 8);
    }
    void startupNeverRestoresOldData() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-no-legacy-XXXXXX");
        const bool hadLocal = qEnvironmentVariableIsSet("LOCALAPPDATA");
        const auto previous = qgetenv("LOCALAPPDATA");
        auto restore = qScopeGuard([&] {
            if (hadLocal)
                qputenv("LOCALAPPDATA", previous);
            else
                qunsetenv("LOCALAPPDATA");
        });
        const auto oldRoot = dir.filePath("old-user");
        QVERIFY(QDir().mkpath(oldRoot + "/Daymark"));
        qputenv("LOCALAPPDATA", oldRoot.toUtf8());
        QFile old(oldRoot + "/Daymark/plans.json");
        QVERIFY(old.open(QIODevice::WriteOnly));
        const QByteArray original = R"({"2026-10-29":[{"text":"legacy record","done":false}]})";
        QCOMPARE(old.write(original), original.size());
        old.close();
        const auto data = dir.filePath("portable/data");
        QVERIFY(QDir().mkpath(data));
        const auto file = data + "/tasks.json";
        {
            MainWindow window(file);
            window.show();
            QTest::qWait(20);
            auto list = window.findChild<QListView *>("taskList");
            QVERIFY(list);
            QCOMPARE(list->model()->rowCount(), 0);
            QVERIFY(!QFile::exists(file));
            auto input = window.findChild<QLineEdit *>("taskInput");
            input->setText("new record");
            QTest::keyClick(input, Qt::Key_Return);
            QVERIFY(QFile::exists(file));
            QCOMPARE(list->model()->rowCount(), 1);
        }
        QVERIFY(QFile::remove(file));
        {
            MainWindow window(file);
            window.show();
            QTest::qWait(20);
            QCOMPARE(window.findChild<QListView *>("taskList")->model()->rowCount(), 0);
            QVERIFY(!QFile::exists(file));
        }
        QVERIFY(old.open(QIODevice::ReadOnly));
        QCOMPARE(old.readAll(), original);
    }
    void portableTasksAndPreferencesMoveTogether() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-portable-XXXXXX");
        const auto a = dir.filePath("first/data"), b = dir.filePath("second/data");
        QVERIFY(QDir().mkpath(a));
        QVERIFY(QDir().mkpath(b));
        Planner plan(a + "/tasks.json");
        plan.add(QDate::currentDate(), "portable task", 1);
        QVERIFY(plan.save());
        {
            QSettings settings(a + "/settings.ini", QSettings::IniFormat);
            settings.setValue("theme", "chinese");
            settings.setValue("sort", 2);
            settings.setValue("weatherAuto", false);
            settings.sync();
            QCOMPARE(settings.status(), QSettings::NoError);
        }
        {
            MainWindow window(a + "/tasks.json");
            window.show();
            QCOMPARE(window.findChild<QLabel *>("weatherText")->text(), QStringLiteral("中式"));
            QCOMPARE(window.findChild<QListView *>("taskList")->model()->rowCount(), 1);
        }
        QVERIFY(QFile::copy(a + "/tasks.json", b + "/tasks.json"));
        QVERIFY(QFile::copy(a + "/settings.ini", b + "/settings.ini"));
        {
            QSettings settings(a + "/settings.ini", QSettings::IniFormat);
            settings.setValue("theme", "american");
            settings.sync();
        }
        {
            MainWindow window(b + "/tasks.json");
            window.show();
            QCOMPARE(window.findChild<QLabel *>("weatherText")->text(), QStringLiteral("中式"));
            QCOMPARE(window.findChild<QListView *>("taskList")->model()->rowCount(), 1);
        }
        QSettings moved(b + "/settings.ini", QSettings::IniFormat);
        QCOMPARE(moved.value("sort").toInt(), 2);
        QCOMPARE(QFileInfo(moved.fileName()).absolutePath(), QFileInfo(b).absoluteFilePath());
    }
    void choiceBoxMouseKeyboardAndWheel() {
        ChoiceBox box;
        box.addItems({"手动排序", "未完成优先", "优先级排序"});
        box.resize(220, 36);
        box.show();
        QTest::qWait(20);
        QSignalSpy activated(&box, &QComboBox::activated);
        QTest::mouseClick(&box, Qt::LeftButton);
        auto popup = box.findChild<QFrame *>("choicePopup");
        auto list = box.findChild<QListView *>("choiceList");
        QVERIFY(popup);
        QVERIFY(list);
        QVERIFY(popup->isVisible());
        const auto popupImage = popup->grab().toImage();
        QVERIFY(popupImage.pixelColor(4, popupImage.height() / 2).alpha() > 240);
        QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                          list->visualRect(list->model()->index(2, 0)).center());
        QCOMPARE(box.currentIndex(), 2);
        QCOMPARE(activated.count(), 1);
        QVERIFY(!popup->isVisible());
        box.showPopup();
        QTest::keyClick(list, Qt::Key_Up);
        QTest::keyClick(list, Qt::Key_Escape);
        QCOMPARE(box.currentIndex(), 2);
        QTest::keyClick(&box, Qt::Key_Down);
        QVERIFY(popup->isVisible());
        QTest::keyClick(list, Qt::Key_Up);
        QTest::keyClick(list, Qt::Key_Return);
        QCOMPARE(box.currentIndex(), 1);
        QCOMPARE(activated.count(), 2);
        QWheelEvent wheel(QPointF(12, 12), box.mapToGlobal(QPoint(12, 12)), QPoint(),
                          QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(&box, &wheel);
        QCOMPARE(box.currentIndex(), 1);
        QVERIFY(box.screen()->availableGeometry().contains(popup->geometry()));
    }
    void settingsDataLocationMigration() {
        QTemporaryDir dir(QDir::currentPath() + "/ui-relocate-XXXXXX");
        const auto source = dir.filePath("data"),
                   destination =
                       dir.filePath("a-very-long-directory-name-for-the-storage-location-"
                                    "regression-test/another-long-directory-for-user-plans");
        QVERIFY(QDir().mkpath(source));
        QVERIFY(QDir().mkpath(destination));
        Planner plan(source + "/tasks.json");
        plan.add(QDate::currentDate(), "keep during move", 1);
        QVERIFY(plan.save());
        StorageManager manager(dir.path());
        MainWindow window(source + "/tasks.json", nullptr, &manager);
        window.show();
        QSignalSpy restart(&window, &MainWindow::restartRequested);
        const bool previous = qApp->testAttribute(Qt::AA_DontUseNativeDialogs);
        qApp->setAttribute(Qt::AA_DontUseNativeDialogs);
        auto restore =
            qScopeGuard([&] { qApp->setAttribute(Qt::AA_DontUseNativeDialogs, previous); });
        QTimer::singleShot(30, &window, [&] {
            auto dialog = window.findChild<QDialog *>();
            if (!dialog)
                return;
            auto tabs = dialog->findChild<QTabWidget *>();
            tabs->setCurrentIndex(1);
            auto choose = dialog->findChild<QPushButton *>("chooseStorage");
            QVERIFY(choose);
            QVERIFY(choose->isEnabled());
            QTimer::singleShot(30, &window, [&] {
                auto picker = window.findChild<QFileDialog *>();
                if (picker) {
                    picker->setDirectory(destination);
                    picker->selectFile(destination);
                    QMetaObject::invokeMethod(picker, "accept", Qt::DirectConnection);
                }
            });
            choose->click();
            auto location = dialog->findChild<QLabel *>("storageLocation");
            QVERIFY(location);
            QCOMPARE(location->text(), QDir::toNativeSeparators(destination));
            QCOMPARE(location->toolTip(), QDir::toNativeSeparators(destination));
            QVERIFY(location->fontMetrics().horizontalAdvance(location->text()) >
                    location->width());
            QVERIFY(location->height() <= location->fontMetrics().height() + 8);
            QVERIFY(
                dialog->rect().contains(location->mapTo(dialog, location->rect().bottomRight())));
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
            QTimer::singleShot(30, &window, [&] {
                auto question = window.findChild<QMessageBox *>();
                if (question)
                    question->button(QMessageBox::Yes)->click();
            });
        });
        window.findChild<QPushButton *>("weatherAction")->click();
        QCOMPARE(restart.count(), 1);
        QCOMPARE(manager.directory(), QFileInfo(destination).canonicalFilePath());
        Planner copied(destination + "/tasks.json");
        QVERIFY(copied.load());
        QCOMPARE(copied.tasks[0].title, QString("keep during move"));
        QVERIFY(QFile::exists(source + "/tasks.json"));
        QVERIFY(QFile::exists(destination + "/settings.ini"));
    }
};
QTEST_MAIN(UiTests)
#include "uitests.moc"
