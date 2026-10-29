// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "icons.h"
#include "mainwindow.h"
#include "uicomponents.h"
#include <QtWidgets>
void MainWindow::preferences() {
    QDialog dialog(this);
    dialog.setWindowTitle("设置与偏好");
    dialog.setFixedWidth(520);
    auto l = column(&dialog, 26);
    l->setSpacing(16);
    l->addWidget(label("随心设色，从容记事。", "sectionTitle"));
    auto tabs = new QTabWidget;
    tabs->setObjectName("settingsTabs");
    l->addWidget(tabs);
    auto appearance = new QWidget;
    auto al = column(appearance, 12);
    al->setSpacing(14);
    auto themeBox = new QComboBox;
    themeBox->setObjectName("themeSelector");
    themeBox->setIconSize(QSize(64, 16));
    for (const auto &preset : Theme::presets()) {
        const auto palette = Theme::named(preset.id);
        QPixmap swatch(64, 16);
        swatch.fill(Qt::transparent);
        QPainter painter(&swatch);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        const QVector<QColor> colors = {palette.bg, palette.soft, palette.accent, palette.text};
        for (int i = 0; i < colors.size(); ++i) {
            painter.setBrush(colors[i]);
            painter.drawRoundedRect(QRectF(i * 16, 1, 13, 14), 3, 3);
        }
        painter.end();
        themeBox->addItem(QIcon(swatch), preset.label, preset.id);
    }
    themeBox->setCurrentIndex(qMax(0, themeBox->findData(settings.value("theme", "forest"))));
    al->addWidget(label("界面配色", "eyebrow"));
    al->addWidget(themeBox);
    auto autoTheme = new QCheckBox("随天气自动换色");
    autoTheme->setChecked(settings.value("weatherAuto", false).toBool());
    connect(themeBox, &QComboBox::activated, &dialog,
            [autoTheme](int) { autoTheme->setChecked(false); });
    al->addWidget(autoTheme);
    auto city = new QLineEdit(settings.value("city").toString());
    city->setPlaceholderText("城市，例如：北京 / Shanghai");
    city->setClearButtonEnabled(true);
    al->addWidget(city);
    auto note = label(
        "晴暖杏，阴苔绿；雨海蓝，雪雾紫，入夜换深色。\n每 30 分钟更新一次，离线时沿用上次配色。",
        "smallMuted");
    note->setWordWrap(true);
    al->addWidget(note);
    auto service = label("天气来源：Open-Meteo。仅在启用后查询你填写的城市。", "smallMuted");
    service->setWordWrap(true);
    al->addWidget(service);
    tabs->addTab(appearance, "外观与天气");
    auto dataPage = new QWidget;
    auto data = column(dataPage, 12);
    data->setSpacing(16);
    data->addWidget(label("你的记录，保存在这台电脑。", "sectionTitle"));
    auto block = new QFrame;
    block->setObjectName("settingsBlock");
    auto bl = column(block, 18);
    bl->addWidget(label("数据位置", "eyebrow"));
    auto path = new QLineEdit(QFileInfo(model.filePath()).absolutePath());
    path->setReadOnly(true);
    path->setCursorPosition(0);
    bl->addWidget(path);
    auto open = button("打开数据文件夹", "secondary");
    bl->addWidget(open);
    connect(open, &QPushButton::clicked, this, [this] {
        QDir().mkpath(QFileInfo(model.filePath()).absolutePath());
        if (!QDesktopServices::openUrl(
                QUrl::fromLocalFile(QFileInfo(model.filePath()).absolutePath())))
            QMessageBox::warning(this, "无法打开", "请复制上方路径，在资源管理器中打开。");
    });
    data->addWidget(block);
    auto backup = button("备份全部计划…", "secondary");
    data->addWidget(backup);
    connect(backup, &QPushButton::clicked, this, [this] {
        if (!healthy) {
            QMessageBox::warning(this, "暂时无法备份",
                                 "数据读取失败。请通过数据文件夹复制原文件，以免生成不完整备份。");
            return;
        }
        auto file = QFileDialog::getSaveFileName(
            this, "备份全部计划",
            QDir::homePath() + "/日序备份-" + QDate::currentDate().toString("yyyyMMdd") + ".json",
            "JSON 文件 (*.json)");
        if (file.isEmpty())
            return;
        Planner copy(file);
        copy.tasks = model.tasks;
        if (!copy.save())
            QMessageBox::warning(this, "备份失败", copy.error);
        else
            QMessageBox::information(this, "备份完成", "已保存全部日期的计划。");
    });
    auto info = label("备份包含所有日期的目标与完成状态。\n月度表格可在「月度回顾」中单独导出。",
                      "smallMuted");
    info->setWordWrap(true);
    data->addWidget(info);
    data->addStretch();
    tabs->addTab(dataPage, "数据与备份");
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setText("保存设置");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    l->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (autoTheme->isChecked() && city->text().trimmed().isEmpty()) {
            tabs->setCurrentIndex(0);
            city->setFocus();
            city->setPlaceholderText("请先填写城市名称");
            return;
        }
        dialog.accept();
    });
    if (dialog.exec() == QDialog::Accepted) {
        cancelWeatherRequest();
        weatherError.clear();
        settings.setValue("theme", themeBox->currentData());
        settings.setValue("weatherAuto", autoTheme->isChecked());
        settings.setValue("city", city->text().trimmed());
        if (autoTheme->isChecked())
            fetchWeather();
        else {
            weatherText = themeBox->currentText().section(' ', 0, 0);
            weatherDetail = "手动配色 · 随心选择";
        }
        applyTheme();
    }
}
