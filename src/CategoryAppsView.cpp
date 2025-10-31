#include "CategoryAppsView.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

CategoryAppsView::CategoryAppsView(QWidget *parent)
    : QWidget(parent), m_layout(new QGridLayout(this))
{
    m_layout->setContentsMargins(16, 16, 16, 16);
    m_layout->setSpacing(12);
}

void CategoryAppsView::setApplications(const QList<ApplicationInfo> &apps)
{
    m_apps = apps;
    rebuildGrid();
}

void CategoryAppsView::rebuildGrid()
{
    QLayoutItem *child;
    while ((child = m_layout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    if (m_apps.isEmpty()) {
        auto *placeholder = new QLabel(tr("Aucune application disponible dans cette catégorie pour le moment."), this);
        placeholder->setAlignment(Qt::AlignCenter);
        m_layout->addWidget(placeholder, 0, 0);
        return;
    }

    constexpr int columns = 2;
    int row = 0;
    int column = 0;

    for (const auto &app : m_apps) {
        auto *button = new QPushButton(app.name, this);
        button->setMinimumHeight(60);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, [this, app]() {
            emit applicationSelected(app);
        });

        m_layout->addWidget(button, row, column);

        if (++column >= columns) {
            column = 0;
            ++row;
        }
    }
}
