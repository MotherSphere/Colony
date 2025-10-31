#include "ModulePlaceholder.h"

#include <QLabel>
#include <QVBoxLayout>

ModulePlaceholder::ModulePlaceholder(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    m_titleLabel = new QLabel(title, this);
    m_titleLabel->setObjectName("placeholderTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);

    auto *subtitle = new QLabel(tr("Cette application sera développée prochainement."), this);
    subtitle->setAlignment(Qt::AlignCenter);

    layout->addWidget(m_titleLabel);
    layout->addWidget(subtitle);
}
