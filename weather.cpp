// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "icons.h"
#include "mainwindow.h"
#include "uicomponents.h"
#include "weatherdata.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QtWidgets>
#include <cmath>
void MainWindow::updateWeatherCard() {
    bool automatic = settings.value("weatherAuto", false).toBool();
    QString city = settings.value("city").toString();
    weatherHeading->setText(automatic ? city + " · 天气" : "天气与配色");
    QString primary = weatherText;
    if (!automatic) {
        primary = Theme::displayName(settings.value("theme", "forest").toString());
        weatherDetail = "手动配色 · 随心选择";
    }
    weatherLabel->setText(
        QFontMetrics(weatherLabel->font()).elidedText(primary, Qt::ElideRight, 206));
    weatherNote->setText(
        QFontMetrics(weatherNote->font()).elidedText(weatherDetail, Qt::ElideRight, 206));
    weatherAction->setIcon(lineIcon("settings", theme.accent));
    weatherAction->setEnabled(true);
    weatherAction->setToolTip(automatic ? "外观与天气设置 · 右键刷新天气" : "外观与天气设置");
    weatherLabel->setToolTip(primary + (weatherError.isEmpty() ? "" : "\n" + weatherError));
    weatherNote->setToolTip(weatherError.isEmpty() ? weatherDetail : weatherError);
}
void MainWindow::weatherFailed(const QString &reason, bool cityNotFound) {
    weatherBusy = false;
    weatherError = reason;
    bool cached = settings.value("weatherCity").toString() == settings.value("city").toString() &&
                  !settings.value("weatherText").toString().isEmpty();
    weatherText = cached ? settings.value("weatherText").toString()
                         : (cityNotFound ? "换个城市名称试试" : "天气稍后更新");
    weatherDetail =
        cached ? "上次获取的天气"
               : (cityNotFound ? "在设置中填写城市或英文名" : "暂时离线 · 当前配色照常使用");
    updateWeatherCard();
}
void MainWindow::fetchWeather() {
    if (weatherBusy || !settings.value("weatherAuto", false).toBool())
        return;
    QString city = settings.value("city").toString().trimmed();
    if (city.isEmpty())
        return;
    int id = ++weatherRequest;
    weatherBusy = true;
    weatherError.clear();
    if (settings.value("weatherCity").toString() != city)
        weatherText = "正在获取天气";
    weatherDetail = "正在获取天气…";
    updateWeatherCard();
    if (settings.value("coordinatesCity").toString() == city && settings.contains("latitude") &&
        settings.contains("longitude") && std::isfinite(settings.value("latitude").toDouble()) &&
        std::isfinite(settings.value("longitude").toDouble())) {
        fetchForecast(settings.value("latitude").toDouble(), settings.value("longitude").toDouble(),
                      city, id);
        return;
    }
    QUrl url("https://geocoding-api.open-meteo.com/v1/search");
    QUrlQuery query;
    query.addQueryItem("name", city);
    query.addQueryItem("count", "1");
    query.addQueryItem("language", "zh");
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    auto reply = network.get(request);
    weatherReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, city, id] {
        auto error = reply->error();
        auto message = reply->errorString();
        auto doc = QJsonDocument::fromJson(reply->readAll());
        reply->deleteLater();
        if (weatherReply == reply)
            weatherReply.clear();
        if (id != weatherRequest || !settings.value("weatherAuto").toBool() ||
            settings.value("city").toString() != city)
            return;
        if (error != QNetworkReply::NoError) {
            weatherFailed("城市查询失败：" + message);
            return;
        }
        auto results = doc.object()["results"].toArray();
        if (results.isEmpty()) {
            weatherFailed("没有找到匹配的城市，请尝试完整城市名或英文名。", true);
            return;
        }
        auto location = results[0].toObject();
        if (!location["latitude"].isDouble() || !location["longitude"].isDouble()) {
            weatherFailed("城市坐标格式无效，请稍后重试。");
            return;
        }
        double lat = location["latitude"].toDouble(), lon = location["longitude"].toDouble();
        if (!std::isfinite(lat) || !std::isfinite(lon) || qAbs(lat) > 90 || qAbs(lon) > 180) {
            weatherFailed("城市坐标无效，请稍后重试。");
            return;
        }
        settings.setValue("coordinatesCity", city);
        settings.setValue("latitude", lat);
        settings.setValue("longitude", lon);
        fetchForecast(lat, lon, city, id);
    });
}
void MainWindow::fetchForecast(double lat, double lon, const QString &city, int id) {
    const auto url = forecastUrl(lat, lon);
    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    auto reply = network.get(request);
    weatherReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, city, id] {
        auto error = reply->error();
        auto message = reply->errorString();
        const auto bytes = reply->readAll();
        reply->deleteLater();
        if (weatherReply == reply)
            weatherReply.clear();
        if (id != weatherRequest || !settings.value("weatherAuto").toBool() ||
            settings.value("city").toString() != city)
            return;
        if (error != QNetworkReply::NoError) {
            weatherFailed("天气查询失败：" + message);
            return;
        }
        QString parseError;
        auto reading = WeatherReading::parse(bytes, &parseError);
        if (!reading) {
            weatherFailed(parseError);
            return;
        }
        weatherTheme = reading->themeName();
        weatherBusy = false;
        weatherError.clear();
        weatherText =
            QString("%1°C  %2")
                .arg(QString::number(reading->temperature, 'f', 0), reading->description());
        weatherDetail = "更新于 " + QTime::currentTime().toString("HH:mm") + " · 随天气换色";
        settings.setValue("weatherTheme", weatherTheme);
        settings.setValue("weatherCity", city);
        settings.setValue("weatherText", weatherText);
        settings.setValue("weatherDetail", weatherDetail);
        applyTheme();
    });
}

void MainWindow::cancelWeatherRequest() {
    ++weatherRequest;
    weatherBusy = false;
    if (weatherReply) {
        auto reply = weatherReply;
        weatherReply.clear();
        reply->abort();
    }
}