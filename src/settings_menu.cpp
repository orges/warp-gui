#include "settings_menu.h"

#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SettingsMenu::SettingsMenu(QWidget *parent)
    : QWidget(parent),
      m_warpBtn(new QPushButton(QStringLiteral("    1.1.1.1 with WARP"), this)),
      m_dnsOnlyBtn(new QPushButton(QStringLiteral("    1.1.1.1"), this)),
      m_preferencesBtn(new QPushButton(QStringLiteral("Preferences"), this)),
      m_aboutBtn(new QPushButton(QStringLiteral("About Cloudflare WARP"), this)),
      m_exitBtn(new QPushButton(QStringLiteral("Exit"), this)),
      m_currentMode(QStringLiteral("warp")) {

    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Mode options - both visible, one with checkmark
    layout->addWidget(m_warpBtn);
    layout->addWidget(m_dnsOnlyBtn);

    // Add separator
    m_separator = new QWidget(this);
    m_separator->setFixedHeight(1);
    m_separator->setStyleSheet(QStringLiteral("background-color: #3a3a3a;"));
    layout->addWidget(m_separator);

    layout->addWidget(m_preferencesBtn);
    layout->addWidget(m_aboutBtn);
    layout->addWidget(m_exitBtn);

    // Style the menu
    setStyleSheet(QStringLiteral(
        "SettingsMenu {"
        "  background-color: #2a2a2a;"
        "  border: 1px solid #3a3a3a;"
        "  border-radius: 8px;"
        "}"
        "QLabel {"
        "  background: transparent;"
        "  color: #ffffff;"
        "  font-size: 13px;"
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
        "QPushButton:last-child {"
        "  border-bottom-left-radius: 8px;"
        "  border-bottom-right-radius: 8px;"
        "}"
    ));

    connect(m_warpBtn, &QPushButton::clicked, this, [this]() {
        emit modeChangeRequested(QStringLiteral("warp"));
        hide();
    });
    connect(m_dnsOnlyBtn, &QPushButton::clicked, this, [this]() {
        emit modeChangeRequested(QStringLiteral("doh"));
        hide();
    });
    connect(m_preferencesBtn, &QPushButton::clicked, this, [this]() {
        emit preferencesRequested();
        hide();
    });
    connect(m_aboutBtn, &QPushButton::clicked, this, [this]() {
        emit aboutRequested();
        hide();
    });
    connect(m_exitBtn, &QPushButton::clicked, this, [this]() {
        emit exitRequested();
        hide();
    });

    setFixedWidth(240);
}

void SettingsMenu::setActionsEnabled(bool enabled) {
    m_warpBtn->setEnabled(enabled);
    m_dnsOnlyBtn->setEnabled(enabled);
    m_preferencesBtn->setEnabled(enabled);
    m_aboutBtn->setEnabled(enabled);
}

void SettingsMenu::setCurrentMode(const QString &mode) {
    m_currentMode = mode.toLower();

    // Update button text with checkmark for active mode
    if (m_currentMode == QStringLiteral("warp")) {
        m_warpBtn->setText(QStringLiteral("✓  1.1.1.1 with WARP"));
        m_dnsOnlyBtn->setText(QStringLiteral("    1.1.1.1"));
    } else {
        m_warpBtn->setText(QStringLiteral("    1.1.1.1 with WARP"));
        m_dnsOnlyBtn->setText(QStringLiteral("✓  1.1.1.1"));
    }
}

void SettingsMenu::setZeroTrustMode(bool isZeroTrust) {
    // Hide mode switching options when enrolled in Zero Trust
    // Organizations control the mode through device profiles, not client UI
    m_warpBtn->setVisible(!isZeroTrust);
    m_dnsOnlyBtn->setVisible(!isZeroTrust);
    m_separator->setVisible(!isZeroTrust);
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
