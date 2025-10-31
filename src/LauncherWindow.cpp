#include "LauncherWindow.h"

#include "CategoryAppsView.h"
#include "ModulePlaceholder.h"

#include <QDockWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>

LauncherWindow::LauncherWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupSampleData();
}

void LauncherWindow::setupUi()
{
    resize(960, 600);
    setWindowTitle(tr("Colony Launcher"));

    auto *central = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    m_tabBar = new QTabBar(this);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setMovable(true);
    m_tabBar->setTabsClosable(true);
    centralLayout->addWidget(m_tabBar);

    m_stack = new QStackedWidget(this);
    centralLayout->addWidget(m_stack);

    m_appsView = new CategoryAppsView(this);
    m_stack->addWidget(m_appsView);

    setCentralWidget(central);

    m_categoryList = new QListWidget(this);
    m_categoryList->setObjectName("categoryList");
    m_categoryList->setFixedWidth(220);

    auto *dock = new QDockWidget(tr("Catégories"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setWidget(m_categoryList);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    connect(m_categoryList, &QListWidget::currentRowChanged, this, &LauncherWindow::handleCategorySelection);
    connect(m_appsView, &CategoryAppsView::applicationSelected, this, &LauncherWindow::openApplication);
    connect(m_tabBar, &QTabBar::currentChanged, this, &LauncherWindow::handleTabChanged);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &LauncherWindow::handleTabCloseRequested);
}

void LauncherWindow::setupSampleData()
{
    m_categories = {
        {"informatics", tr("Informatique"), {
             {"text_editor", tr("Éditeur de texte"), tr("Rédiger et organiser vos notes.")},
             {"terminal", tr("Terminal"), tr("Accéder à la console système." )}
         }},
        {"music", tr("Musique"), {
             {"player", tr("Lecteur audio"), tr("Écouter vos morceaux favoris.")},
             {"synth", tr("Synthétiseur"), tr("Créer des sonorités expérimentales.")}
         }},
        {"category3", tr("Catégorie 3"), {}},
        {"category4", tr("Catégorie 4"), {}},
        {"category5", tr("Catégorie 5"), {}},
        {"category6", tr("Catégorie 6"), {}},
        {"category7", tr("Catégorie 7"), {}}
    };

    for (const auto &category : m_categories) {
        m_categoryList->addItem(category.name);
    }

    if (!m_categories.isEmpty()) {
        m_categoryList->setCurrentRow(0);
    }
}

void LauncherWindow::handleCategorySelection(int row)
{
    if (row < 0 || row >= m_categories.size()) {
        return;
    }

    updateAppsView(row);
    m_stack->setCurrentWidget(m_appsView);
}

void LauncherWindow::updateAppsView(int categoryIndex)
{
    if (categoryIndex < 0 || categoryIndex >= m_categories.size()) {
        return;
    }

    m_appsView->setApplications(m_categories[categoryIndex].applications);
}

void LauncherWindow::openApplication(const ApplicationInfo &app)
{
    int existingIndex = findTabForApp(app.id);
    if (existingIndex != -1) {
        m_tabBar->setCurrentIndex(existingIndex);
        return;
    }

    auto *page = new ModulePlaceholder(app.name, this);
    m_openPages.insert(app.id, page);
    m_stack->addWidget(page);

    int tabIndex = m_tabBar->addTab(app.name);
    m_tabBar->setTabData(tabIndex, app.id);
    m_tabBar->setCurrentIndex(tabIndex);

    m_stack->setCurrentWidget(page);
}

int LauncherWindow::findTabForApp(const QString &appId) const
{
    for (int i = 0; i < m_tabBar->count(); ++i) {
        if (m_tabBar->tabData(i).toString() == appId) {
            return i;
        }
    }
    return -1;
}

void LauncherWindow::handleTabChanged(int index)
{
    if (index < 0) {
        m_stack->setCurrentWidget(m_appsView);
        return;
    }

    const QString appId = m_tabBar->tabData(index).toString();
    QWidget *page = m_openPages.value(appId, nullptr);
    if (page) {
        m_stack->setCurrentWidget(page);
    }
}

void LauncherWindow::handleTabCloseRequested(int index)
{
    if (index < 0) {
        return;
    }

    const QString appId = m_tabBar->tabData(index).toString();
    QWidget *page = m_openPages.take(appId);
    if (page) {
        m_stack->removeWidget(page);
        page->deleteLater();
    }

    m_tabBar->removeTab(index);

    if (m_tabBar->count() == 0) {
        m_stack->setCurrentWidget(m_appsView);
    }
}
