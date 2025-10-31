#pragma once

#include <QWidget>
#include <QString>

class QLabel;

class ModulePlaceholder : public QWidget {
    Q_OBJECT

public:
    explicit ModulePlaceholder(const QString &title, QWidget *parent = nullptr);

private:
    QLabel *m_titleLabel = nullptr;
};
