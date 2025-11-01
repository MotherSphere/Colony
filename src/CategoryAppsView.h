#pragma once

#include <QWidget>
#include <QList>

#include "LauncherModels.h"

class QVBoxLayout;

class CategoryAppsView : public QWidget {
    Q_OBJECT

public:
    explicit CategoryAppsView(QWidget *parent = nullptr);

    void setApplications(const QList<ApplicationInfo> &apps);

signals:
    void applicationSelected(const ApplicationInfo &app);

private:
    void rebuildGrid();

    QList<ApplicationInfo> m_apps;
    QVBoxLayout *m_layout = nullptr;
};
