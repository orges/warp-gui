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
      m_settingsBtn(new QPushButton(this)),
      m_busy(false) {
    // Don't use WA_TranslucentBackground - it interferes with the styled background
    // The frameless window with styled background will work fine

    // Make the window close when clicking outside or pressing Escape
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    // Title and settings button in horizontal layout
    auto *topLayout = new QHBoxLayout();
    topLayout->addStretch();

    QFont titleFont = m_title->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    m_title->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(m_title);

    topLayout->addStretch();

    // Settings button with wrench icon
    m_settingsBtn->setText(QStringLiteral("⚙"));
    m_settingsBtn->setFixedSize(32, 32);
    topLayout->addWidget(m_settingsBtn);

    QFont statusFont = m_status->font();
    statusFont.setPointSize(16);
    statusFont.setBold(true);
    m_status->setFont(statusFont);
    m_status->setAlignment(Qt::AlignHCenter);

    QFont subtitleFont = m_subtitle->font();
    subtitleFont.setPointSize(11);
    m_subtitle->setFont(subtitleFont);
    m_subtitle->setAlignment(Qt::AlignHCenter);
    m_subtitle->setWordWrap(true);

    layout->addLayout(topLayout);
    layout->addSpacing(6);
    layout->addWidget(m_toggle, 0, Qt::AlignHCenter);
    layout->addSpacing(6);
    layout->addWidget(m_status);
    layout->addWidget(m_subtitle);
    layout->addStretch();

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
        "  font-size: 20px; "
        "  padding: 0px;"
        "}"
        "QPushButton:hover { color: #ffffff; }"
    ));

    m_title->setStyleSheet(QStringLiteral("color: #ff6a00; background: transparent; font-weight: bold;"));
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
