// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#include "weatherdata.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <cmath>
namespace {
const QSet<int> validCodes = {0,  1,  2,  3,  45, 48, 51, 53, 55, 56, 57, 61, 63, 65,
                              66, 67, 71, 73, 75, 77, 80, 81, 82, 85, 86, 95, 96, 99};
bool snow(int code) {
    return (code >= 71 && code <= 77) || code == 85 || code == 86;
}
} // namespace
QString WeatherReading::themeName() const {
    if (!isDay)
        return "night";
    if (code == 0)
        return "sun";
    if (code <= 48)
        return "forest";
    return snow(code) ? "lilac" : "ocean";
}
QString WeatherReading::description() const {
    if (code == 0)
        return QStringLiteral("晴");
    if (code == 1)
        return QStringLiteral("晴间多云");
    if (code == 2)
        return QStringLiteral("多云");
    if (code == 3)
        return QStringLiteral("阴");
    if (code == 45 || code == 48)
        return QStringLiteral("雾");
    if (snow(code))
        return QStringLiteral("雪");
    if (code >= 95)
        return QStringLiteral("雷雨");
    if (code == 56 || code == 57 || code == 66 || code == 67)
        return QStringLiteral("冻雨");
    if (code < 60)
        return QStringLiteral("毛毛雨");
    if (code >= 80)
        return QStringLiteral("阵雨");
    return QStringLiteral("雨");
}
std::optional<WeatherReading> WeatherReading::parse(const QByteArray &json, QString *error) {
    if (error)
        error->clear();
    QJsonParseError parse;
    auto doc = QJsonDocument::fromJson(json, &parse);
    auto current = doc.object()["current"].toObject();
    const auto temp = current["temperature_2m"], code = current["weather_code"],
               day = current["is_day"];
    if (parse.error != QJsonParseError::NoError || !temp.isDouble() ||
        !std::isfinite(temp.toDouble()) || !code.isDouble() ||
        code.toDouble() != std::floor(code.toDouble()) || !validCodes.contains(code.toInt(-1)) ||
        !day.isDouble() || (day.toDouble() != 0 && day.toDouble() != 1)) {
        if (error)
            *error = QStringLiteral("天气服务返回的数据不完整或格式无效，请稍后刷新。");
        return std::nullopt;
    }
    return WeatherReading{temp.toDouble(), code.toInt(), day.toInt() == 1};
}
QUrl forecastUrl(double latitude, double longitude) {
    const auto base =
        QString("https://api.open-meteo.com/v1/forecast?latitude=%1&longitude=%2")
            .arg(QString::number(latitude, 'f', 5), QString::number(longitude, 'f', 5));
    return QUrl::fromEncoded(
        (base + "&current=temperature_2m%2Cweather_code%2Cis_day&timezone=auto").toUtf8());
}
