#include "./syncthinginfoaction.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>

SyncthingInfoWidget::SyncthingInfoWidget(const SyncthingInfoAction *action, QWidget *parent)
    : QWidget(parent)
    , m_textLabel(new QLabel(parent))
    , m_iconLabel(new QLabel(parent))
{
    auto *const layout = new QHBoxLayout(parent);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(5);
    m_iconLabel->setFixedWidth(16);
    m_iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_textLabel);
    setLayout(layout);
    updateFromAction(action);
    connect(action, &QAction::changed, this, &SyncthingInfoWidget::updateFromSender);
}

void SyncthingInfoWidget::updateFromSender()
{
    updateFromAction(qobject_cast<const SyncthingInfoAction *>(QObject::sender()));
}

void SyncthingInfoWidget::updateFromAction(const SyncthingInfoAction *action)
{
    auto text(action->text());
    text.replace(QChar('&'), QString()); // FIXME: find a better way to get rid of '&' in QAction::text()
    m_textLabel->setText(std::move(text));
    m_iconLabel->setPixmap(action->icon().pixmap(16));
    setVisible(action->isVisible());
}

SyncthingInfoAction::SyncthingInfoAction(QObject *parent)
    : QWidgetAction(parent)
{
}

QWidget *SyncthingInfoAction::createWidget(QWidget *parent)
{
    return new SyncthingInfoWidget(this, parent);
}
