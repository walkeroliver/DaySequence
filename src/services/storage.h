// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 WalkerOliver
#pragma once
#include <QString>

class StorageManager {
  public:
    static constexpr int RestartExitCode = 42;
    explicit StorageManager(QString applicationDirectory);
    QString directory(QString *error = nullptr) const;
    bool relocate(const QString &sourceDirectory, const QString &destinationDirectory,
                  QString *error = nullptr) const;
    QString configurationFile() const;

  private:
    QString applicationDirectory;
};
