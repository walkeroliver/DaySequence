// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <optional>
struct WeatherReading {
    double temperature = 0;
    int code = 0;
    bool isDay = true;
    QString themeName() const;
    QString description() const;
    static std::optional<WeatherReading> parse(const QByteArray &json, QString *error = nullptr);
};
QUrl forecastUrl(double latitude, double longitude);
