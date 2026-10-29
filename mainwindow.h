// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include "planner.h"

#include "visuals.h"
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QSettings>
#include <functional>
class QLabel;
class QLineEdit;
class QComboBox;
class QListView;
class QStackedWidget;
class QProgressBar;
class QPushButton;
class QSystemTrayIcon;
class QCloseEvent;
class QNetworkReply;
class QTimer;
class TaskListModel;
class TaskDelegate;
class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(const QString &dataFile, QWidget *parent = nullptr);
    void preview(const QString &folder);
    void restoreWindow();

  protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    Planner model;
    QSettings settings;
    QNetworkAccessManager network;
    QDate selected = QDate::currentDate(),
          month = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);
    Theme theme;
    QString weatherTheme = "forest", weatherText = "设置城市，感知天气", activeTheme;
    bool quitting = false, healthy = true, weatherBusy = false;
    int weatherRequest = 0;
    QString weatherDetail, weatherError;
    QWidget *titleBar;
    QPushButton *maximizeButton = nullptr, *weatherAction = nullptr;
    QLabel *weatherHeading, *weatherNote;
    QStackedWidget *pages;
    QWidget *root;
    CalendarView *calendar;
    ChartView *bars, *donut;
    QListView *taskList = nullptr;
    TaskListModel *taskModel;
    TaskDelegate *taskDelegate;
    QLabel *emptyTasks;
    QDate taskViewDate;
    QString taskViewFilter;
    int taskViewSort = -1;
    QPointer<QNetworkReply> weatherReply;
    QTimer *searchTimer;
    QLineEdit *taskInput, *search;
    QComboBox *priority, *sort;
    QProgressBar *dayProgress;
    QLabel *pageTitle, *pageSubtitle, *weatherLabel, *monthLabel, *dayLabel, *dayCaption,
        *progressCaption, *monthCaption, *reviewMonth, *insight, *saveStatus;
    QVector<QLabel *> metricValues, metricNotes;
    QPushButton *dailyNav, *reviewNav;
    QSystemTrayIcon *tray = nullptr;
    void build();
    void refresh();
    void refreshTasks();
    void showTaskMenu(const QString &id, const QPoint &position);
    void updateChromeDate();

    void applyTheme();
    void selectPage(int page);
    void changeMonth(int delta);
    void editTask(const QString &id);
    bool mutate(const std::function<void()> &action);
    void preferences();
    void fetchWeather();
    void cancelWeatherRequest();
    void updateWeatherCard();
    void weatherFailed(const QString &reason, bool cityNotFound = false);
    void fetchForecast(double latitude, double longitude, const QString &city, int requestId);
    void setupTray();
    void addTask();
};
