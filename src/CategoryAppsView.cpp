#include "CategoryAppsView.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

CategoryAppsView::CategoryAppsView(QWidget *parent)
    : QWidget(parent), m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(16, 16, 16, 16);
    m_layout->setSpacing(12);
    m_layout->setAlignment(Qt::AlignTop);
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
        if (QWidget *widget = child->widget()) {
            delete widget;
        }
        delete child;
    }

    if (m_apps.isEmpty()) {
        auto *placeholder = new QLabel(tr("Aucune application disponible dans cette catégorie pour le moment."), this);
        placeholder->setAlignment(Qt::AlignCenter);
        m_layout->addWidget(placeholder);
        m_layout->addStretch();
        return;
    }

    for (const auto &app : m_apps) {
        auto *button = new QPushButton(app.name, this);
        button->setFixedHeight(60);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, [this, app]() {
            emit applicationSelected(app);
        });

        m_layout->addWidget(button);
    }

    m_layout->addStretch();
}
