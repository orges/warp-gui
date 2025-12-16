#include "settings_menu.h"

#include <QKeyEvent>
#include <QPushButton>
#include <QVBoxLayout>

SettingsMenu::SettingsMenu(QWidget *parent)
    : QWidget(parent),
      m_registerBtn(new QPushButton(QStringLiteral("Register"), this)),
      m_enrollBtn(new QPushButton(QStringLiteral("Enroll Zero Trust Organization…"), this)),
      m_licenseBtn(new QPushButton(QStringLiteral("Attach License Key…"), this)),
      m_refreshBtn(new QPushButton(QStringLiteral("Refresh"), this)),
      m_quitBtn(new QPushButton(QStringLiteral("Quit"), this)) {

    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(m_registerBtn);
    layout->addWidget(m_enrollBtn);
    layout->addWidget(m_licenseBtn);
    layout->addWidget(m_refreshBtn);
    layout->addWidget(m_quitBtn);

    // Style the menu
    setStyleSheet(QStringLiteral(
        "SettingsMenu {"
        "  background-color: #2a2a2a;"
        "  border: 1px solid #3a3a3a;"
        "  border-radius: 8px;"
        "}"
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: #ffffff;"
        "  padding: 10px 20px;"
        "  text-align: left;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #3a3a3a;"
        "}"
        "QPushButton:disabled {"
        "  color: #666666;"
        "}"
        "QPushButton:first-child {"
        "  border-top-left-radius: 8px;"
        "  border-top-right-radius: 8px;"
        "}"
        "QPushButton:last-child {"
        "  border-bottom-left-radius: 8px;"
        "  border-bottom-right-radius: 8px;"
        "}"
    ));

    connect(m_registerBtn, &QPushButton::clicked, this, [this]() {
        emit registerRequested();
        hide();
    });
    connect(m_enrollBtn, &QPushButton::clicked, this, [this]() {
        emit enrollRequested();
        hide();
    });
    connect(m_licenseBtn, &QPushButton::clicked, this, [this]() {
        emit licenseRequested();
        hide();
    });
    connect(m_refreshBtn, &QPushButton::clicked, this, [this]() {
        emit refreshRequested();
        hide();
    });
    connect(m_quitBtn, &QPushButton::clicked, this, [this]() {
        emit quitRequested();
        hide();
    });

    setFixedWidth(220);
}

void SettingsMenu::setActionsEnabled(bool enabled) {
    m_registerBtn->setEnabled(enabled);
    m_enrollBtn->setEnabled(enabled);
    m_licenseBtn->setEnabled(enabled);
    m_refreshBtn->setEnabled(enabled);
}

void SettingsMenu::focusOutEvent(QFocusEvent *event) {
    QWidget::focusOutEvent(event);
    hide();
}

void SettingsMenu::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        hide();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}
