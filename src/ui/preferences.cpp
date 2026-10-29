// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "choicebox.h"
#include "icons.h"
#include "mainwindow.h"
#include "pathlabel.h"
#include "storage.h"
#include "uicomponents.h"
#include <QtWidgets>
void MainWindow::preferences() {
    const auto currentDirectory = QFileInfo(model.filePath()).absolutePath();
    QString pendingDirectory = currentDirectory;
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
    auto themeBox = new ChoiceBox;
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
    data->addWidget(label("让记录保存在你选择的位置。", "sectionTitle"));
    auto block = new QFrame;
    block->setObjectName("settingsBlock");
    auto bl = column(block, 18);
    bl->addWidget(label("保存文件夹", "eyebrow"));
    auto path = new PathLabel(QDir::toNativeSeparators(currentDirectory));
    path->setToolTip(path->text());
    bl->addWidget(path);
    auto directoryButtons = new QHBoxLayout;
    auto choose = button("更改位置…", "secondary");
    choose->setObjectName("chooseStorage");
    choose->setEnabled(storageManager && healthy);
    auto open = button("打开文件夹", "secondary");
    auto copy = button("复制路径", "secondary");
    directoryButtons->addWidget(choose);
    directoryButtons->addWidget(open);
    directoryButtons->addWidget(copy);
    bl->addLayout(directoryButtons);
    auto locationNote =
        label("修改保存位置后，会复制当前任务和偏好并重启生效。原文件保留。", "smallMuted");
    locationNote->setWordWrap(true);
    bl->addWidget(locationNote);
    connect(choose, &QPushButton::clicked, &dialog, [&] {
        const auto selected = QFileDialog::getExistingDirectory(
            &dialog, "选择新的保存文件夹", pendingDirectory, QFileDialog::ShowDirsOnly);
        if (selected.isEmpty())
            return;
        pendingDirectory = QDir::cleanPath(selected);
        path->setText(QDir::toNativeSeparators(pendingDirectory));
        path->setToolTip(path->text());
    });
    connect(copy, &QPushButton::clicked, &dialog, [&] {
        QApplication::clipboard()->setText(QDir::toNativeSeparators(pendingDirectory));
    });
    connect(open, &QPushButton::clicked, &dialog, [&] {
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(pendingDirectory)))
            QMessageBox::warning(&dialog, "无法打开", "请复制路径，在资源管理器中打开。");
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
        settings.sync();
        if (settings.status() != QSettings::NoError) {
            QMessageBox::warning(this, "设置未能保存", "保存文件夹不可写入，请检查权限后重试。");
            return;
        }
        applyTheme();
        if (pendingDirectory.compare(currentDirectory, Qt::CaseInsensitive) != 0) {
            if (!storageManager || !healthy)
                return;
            const auto answer = QMessageBox::question(
                this, "更改保存位置", "将任务和偏好复制到所选文件夹，并重新打开日序。继续吗？",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes)
                return;
            if (!model.save()) {
                QMessageBox::warning(this, "无法迁移数据", model.error);
                return;
            }
            settings.setValue("windowGeometry", saveGeometry());
            settings.sync();
            if (settings.status() != QSettings::NoError) {
                QMessageBox::warning(this, "无法迁移数据", "当前偏好未能保存，请检查文件夹权限。");
                return;
            }
            QString error;
            if (!storageManager->relocate(currentDirectory, pendingDirectory, &error)) {
                QMessageBox::warning(this, "保存位置未更改", error);
                return;
            }
            cancelWeatherRequest();
            quitting = true;
            tray->hide();
            hide();
            emit restartRequested();
        }
    }
}
