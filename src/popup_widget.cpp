#include "popup_widget.h"
#include "toggle_switch.h"

#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

WarpPopup::WarpPopup(QWidget *parent)
    : QWidget(parent),
      m_title(new QLabel(QStringLiteral("WARP"), this)),
      m_toggle(new ToggleSwitch(this)),
      m_status(new QLabel(QStringLiteral("…"), this)),
      m_subtitle(new QLabel(QStringLiteral(""), this)),
      m_bottomBar(new QWidget(this)),
      m_brandingLabel(new QLabel(QStringLiteral("WARP\nby Cloudflare"), m_bottomBar)),
      m_settingsBtn(new QPushButton(m_bottomBar)),
      m_busy(false) {
    // Don't use WA_TranslucentBackground - it interferes with the styled background
    // The frameless window with styled background will work fine

    // Make the window close when clicking outside or pressing Escape
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Main content area
    auto *contentWidget = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(18, 18, 18, 18);
    contentLayout->setSpacing(12);

    // Title centered at top
    QFont titleFont = m_title->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    m_title->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(m_title);

    contentLayout->addSpacing(6);
    contentLayout->addWidget(m_toggle, 0, Qt::AlignHCenter);
    contentLayout->addSpacing(6);

    QFont statusFont = m_status->font();
    statusFont.setPointSize(16);
    statusFont.setBold(true);
    m_status->setFont(statusFont);
    m_status->setAlignment(Qt::AlignHCenter);
    contentLayout->addWidget(m_status);

    QFont subtitleFont = m_subtitle->font();
    subtitleFont.setPointSize(11);
    m_subtitle->setFont(subtitleFont);
    m_subtitle->setAlignment(Qt::AlignHCenter);
    m_subtitle->setWordWrap(true);
    contentLayout->addWidget(m_subtitle);

    contentLayout->addStretch();

    layout->addWidget(contentWidget);

    // Bottom bar
    auto *bottomLayout = new QHBoxLayout(m_bottomBar);
    bottomLayout->setContentsMargins(12, 8, 12, 8);
    bottomLayout->setSpacing(0);

    QFont brandingFont = m_brandingLabel->font();
    brandingFont.setPointSize(9);
    brandingFont.setBold(true);
    m_brandingLabel->setFont(brandingFont);
    bottomLayout->addWidget(m_brandingLabel);

    bottomLayout->addStretch();

    // Settings button with gear icon
    m_settingsBtn->setText(QStringLiteral("⚙"));
    m_settingsBtn->setFixedSize(28, 28);
    bottomLayout->addWidget(m_settingsBtn);

    m_bottomBar->setFixedHeight(50);
    layout->addWidget(m_bottomBar);

    applyStyle();

    connect(m_toggle, &ToggleSwitch::toggled, this, &WarpPopup::onToggleChanged);
    connect(m_settingsBtn, &QPushButton::clicked, this, &WarpPopup::requestSettings);

    setFixedSize(260, 320);
}

void WarpPopup::applyStyle() {
    setStyleSheet(QStringLiteral(
        "WarpPopup { "
        "  background-color: #1e1e1e; "
        "  border: 1px solid #3a3a3a; "
        "  border-radius: 12px; "
        "}"
        "QLabel { color: #ffffff; background: transparent; }"
        "QPushButton {"
        "  background: transparent; "
        "  border: none; "
        "  color: #999999; "
        "  font-size: 18px; "
        "  padding: 0px;"
        "}"
        "QPushButton:hover { color: #ffffff; }"
    ));

    m_title->setStyleSheet(QStringLiteral("color: #ff6a00; background: transparent; font-weight: bold;"));

    // Bottom bar styling
    m_bottomBar->setStyleSheet(QStringLiteral(
        "QWidget { "
        "  background-color: #2a2a2a; "
        "  border-top: 1px solid #3a3a3a; "
        "  border-bottom-left-radius: 12px; "
        "  border-bottom-right-radius: 12px; "
        "}"
    ));

    m_brandingLabel->setStyleSheet(QStringLiteral(
        "color: #888888; "
        "background: transparent; "
        "font-weight: bold;"
    ));
}

void WarpPopup::setStatusText(const QString &status, const QString &reason) {
    const QString s = status.trimmed();
    m_status->setText(s.isEmpty() ? QStringLiteral("Unknown") : s);

    const QString lower = s.toLower();
    if (lower == QStringLiteral("connected")) {
        m_subtitle->setText(QStringLiteral("Your Internet is <span style='color:#ff6a00;font-weight:600'>private</span>."));
        m_toggle->blockSignals(true);
        m_toggle->setChecked(true);
        m_toggle->blockSignals(false);
    } else if (lower == QStringLiteral("connecting")) {
        m_subtitle->setText(QStringLiteral("Connecting…"));
        m_toggle->blockSignals(true);
        m_toggle->setChecked(true);
        m_toggle->blockSignals(false);
    } else {
        // Always show "not private" message when disconnected, ignore reason
        m_subtitle->setText(QStringLiteral("Your Internet is <span style='color:#ff6a00;font-weight:600'>not private</span>."));
        m_toggle->blockSignals(true);
        m_toggle->setChecked(false);
        m_toggle->blockSignals(false);
    }
}

void WarpPopup::setBusy(bool busy) {
    m_busy = busy;
    m_toggle->setEnabled(!busy);
    if (busy) {
        m_subtitle->setText(QStringLiteral("Working…"));
    }
}

void WarpPopup::onToggleChanged(bool checked) {
    if (m_busy) {
        return;
    }

    if (checked) {
        emit requestConnect();
    } else {
        emit requestDisconnect();
    }
}

void WarpPopup::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        emit requestClose();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void WarpPopup::focusOutEvent(QFocusEvent *event) {
    QWidget::focusOutEvent(event);
    // Close when focus is lost (user clicked outside)
    emit requestClose();
}
