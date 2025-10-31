#pragma once

#include <QString>
#include <QList>

struct ApplicationInfo {
    QString id;
    QString name;
    QString description;
};

struct Category {
    QString id;
    QString name;
    QList<ApplicationInfo> applications;
};
