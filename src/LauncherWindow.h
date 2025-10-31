#pragma once

#include <QMainWindow>
#include <QHash>

#include "LauncherModels.h"

class QListWidget;
class QStackedWidget;
class QTabBar;
class CategoryAppsView;

class LauncherWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit LauncherWindow(QWidget *parent = nullptr);

private slots:
    void handleCategorySelection(int row);
    void openApplication(const ApplicationInfo &app);
    void handleTabChanged(int index);
    void handleTabCloseRequested(int index);

private:
    void setupUi();
    void setupSampleData();
    void updateAppsView(int categoryIndex);
    int findTabForApp(const QString &appId) const;

    QListWidget *m_categoryList = nullptr;
    QStackedWidget *m_stack = nullptr;
    QTabBar *m_tabBar = nullptr;
    CategoryAppsView *m_appsView = nullptr;

    QList<Category> m_categories;
    QHash<QString, QWidget *> m_openPages;
};
