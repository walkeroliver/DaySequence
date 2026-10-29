// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "choicebox.h"
#include "focusclock.h"
#include "icons.h"
#include "mainwindow.h"
#include "tasklist.h"
#include "uicomponents.h"
#include <QtWidgets>
void MainWindow::applyTheme() {
    const auto nextTheme = settings.value("weatherAuto", false).toBool()
                               ? weatherTheme
                               : settings.value("theme", "forest").toString();
    if (nextTheme == activeTheme) {
        updateWeatherCard();
        return;
    }
    activeTheme = nextTheme;
    theme = Theme::named(activeTheme);
    auto c = [](QColor color) { return color.name(); };
    QString css = QString(R"(
        QWidget { color:%1; font-family:'Microsoft YaHei UI'; font-size:13px; }
        QMainWindow, #root, QDialog { background:%2; }
        QScrollArea, QScrollArea > QWidget, QScrollArea > QWidget > QWidget { background:transparent; border:0; }
        #sidebar { background:%3; border-right:1px solid %5; }
        #brand { font-size:30px; font-weight:700; letter-spacing:5px; }
        #eyebrow { font-size:10px; color:%4; letter-spacing:1px; }
        #sideQuote { color:%4; font-size:13px;  }
        #pageTitle { font-size:27px; font-weight:700; }
        #sectionTitle { font-size:18px; font-weight:600; }
        #muted { color:%4; font-size:12px; } #smallMuted { color:%4; font-size:11px; }
        #card { background:%3; border:1px solid %5; border-radius:18px; }
        #weather { background:%7; border-radius:14px; }
        #weatherText { font-size:16px; font-weight:600; color:%1; }
        #weatherNote { font-size:10px; color:%4; }
        #weatherAction { background:transparent; padding:0; border-radius:6px; } #weatherAction:hover { background:%5; }
        #exitEntry { background:%7; border:1px solid %5; padding:0; border-radius:12px; } #exitEntry:hover { background:%5; }
        #titleBar { background:%3; border-bottom:1px solid %5; }
        #seal { background:%6; color:#FFF9EF; border-radius:4px; font-family:'SimSun'; font-size:18px; }
        #chromeName { font-family:'SimSun'; font-size:19px; font-weight:700; }
        #chromeMotto { font-size:11px; color:%4; padding-left:10px; } #chromeDate { font-size:11px; color:%4; }
        #chromeButton, #closeButton { background:transparent; padding:0; border-radius:6px; }
        #chromeButton:hover { background:%7; } #closeButton:hover { background:#F2DFDB; }
        QTabWidget::pane { border:0; } QTabBar::tab { background:transparent; color:%4; padding:10px 22px; border-bottom:2px solid transparent; }
        QTabBar::tab:selected { color:%6; border-bottom:2px solid %6; } QTabBar::tab:hover { background:%7; }
        #settingsBlock { background:%3; border:1px solid %5; border-radius:12px; }
        #pill { color:%6; background:%7; padding:5px 10px; border-radius:8px; font-size:11px; }
        QPushButton { background:%7; border:0; border-radius:8px; padding:9px 13px; }
        QPushButton:hover { background:%5; } QPushButton:pressed { color:%6; }
        QPushButton:disabled { color:%4; }
        #clockFinishTime { color:%4; font-size:11px; }
        #clockStatus { color:%6; background:%7; padding:7px 12px; border-radius:9px; font-size:11px; }
        #clockPreset { background:%7; color:%4; font-size:11px; padding:9px 4px; border:1px solid transparent; }
        #clockPreset:checked { border:1px solid %6; color:%6; font-weight:600; }
        #clockHours, #clockMinutes, #clockSeconds { background:%7; color:%1; border:1px solid %5; border-radius:10px; font-family:'Segoe UI'; font-size:26px; padding:4px; }
        #clockHours:focus, #clockMinutes:focus, #clockSeconds:focus { border:1px solid %6; }
        #clockStart { background:%6; color:%3; font-weight:600; border-radius:10px; }
        #clockStart:hover { border:1px solid %6; }
        #clockReset { background:transparent; color:%4; } #clockReset:hover { background:%7; color:%6; }
        #nav { text-align:left; padding:13px 12px; background:transparent; color:%4; font-size:14px; }
        #nav:checked { background:%7; color:%6; font-weight:600; } #nav:hover { background:%7; }
        #primary { background:%6; color:white; font-weight:600; } #primary:hover { padding:9px 13px; border:1px solid %6; }
        #secondary { background:%7; color:%6; font-size:12px; }
        #monthSelector { font-size:11px; color:%4; background:transparent; padding:6px 4px; } #monthSelector:hover { color:%6; }

        QLineEdit,QDateEdit { background:%3; border:1px solid %5; border-radius:8px; padding:8px; selection-background-color:%6; }
        QLineEdit:focus { border:1px solid %6; }
        #taskList { background:transparent; border:0; outline:none; } #taskList::item { background:transparent; border:0; }
        QCheckBox::indicator { width:19px; height:19px; border:1px solid %4; border-radius:6px; background:%3; }
        QCheckBox::indicator:checked { background:%6; border:1px solid %6; image:url(:/check.svg); }
        #empty { color:%4; font-size:13px; } #metricValue { font-size:32px; font-weight:700; color:%6; }
        #insight { font-size:13px; } #divider { background:%5; }
        QProgressBar { border:0; border-radius:3px; background:%5; } QProgressBar::chunk { background:%6; border-radius:3px; }
        QMenu { background:%3; border:1px solid %5; padding:6px; } QMenu::item { padding:8px 20px; } QMenu::item:selected { background:%7; border-radius:5px; }
        QScrollBar:vertical { width:6px; background:transparent; } QScrollBar::handle:vertical { background:%5; border-radius:3px; min-height:24px; } QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
        QToolTip { background:%3; color:%1; border:1px solid %5; padding:6px; }
    )")
                      .arg(c(theme.text), c(theme.bg), c(theme.panel), c(theme.muted),
                           c(theme.line), c(theme.accent), c(theme.soft));
    qApp->setProperty("daysequenceTheme", activeTheme);
    qApp->setStyleSheet(css);
    for (auto box : findChildren<ChoiceBox *>())
        box->setTheme(theme);
    for (auto b : findChildren<QPushButton *>("chromeButton"))
        b->setIcon(lineIcon(b == maximizeButton ? (isMaximized() ? "restore" : "max") : "min",
                            theme.muted));
    for (auto b : findChildren<QPushButton *>("closeButton"))
        b->setIcon(lineIcon("close", theme.muted));
    for (auto b : findChildren<QPushButton *>("exitEntry"))
        b->setIcon(lineIcon("power", theme.accent));
    updateWeatherCard();
    focusClock->setTheme(theme);
    calendar->theme = theme;
    calendar->update();
    bars->theme = theme;
    bars->update();
    donut->theme = theme;
    donut->update();
    taskDelegate->setTheme(theme);
    taskList->viewport()->update();
}
