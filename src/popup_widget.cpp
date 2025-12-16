#include "popup_widget.h"

#include <QCheckBox>
#include <QFont>
#include <QKeyEvent>
#include <QLabel>
#include <QVBoxLayout>

WarpPopup::WarpPopup(QWidget *parent)
    : QWidget(parent),
      m_title(new QLabel(QStringLiteral("WARP"), this)),
      m_toggle(new QCheckBox(this)),
      m_status(new QLabel(QStringLiteral("…"), this)),
      m_subtitle(new QLabel(QStringLiteral(""), this)),
      m_busy(false) {
    // Don't use WA_TranslucentBackground - it interferes with the styled background
    // The frameless window with styled background will work fine

    // Make the window close when clicking outside or pressing Escape
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    QFont titleFont = m_title->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    m_title->setAlignment(Qt::AlignHCenter);

    m_toggle->setTristate(false);
    m_toggle->setText(QString());

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

    layout->addWidget(m_title);
    layout->addSpacing(6);
    layout->addWidget(m_toggle, 0, Qt::AlignHCenter);
    layout->addSpacing(6);
    layout->addWidget(m_status);
    layout->addWidget(m_subtitle);

    applyStyle();

    connect(m_toggle, &QCheckBox::toggled, this, &WarpPopup::onToggleChanged);

    setFixedSize(260, 320);
}

void WarpPopup::applyStyle() {
    setStyleSheet(QStringLiteral(
        "WarpPopup { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a1a1c, stop:1 #0f0f10); "
        "  border: 2px solid rgba(255,255,255,0.15); "
        "  border-radius: 16px; "
        "}"
        "QLabel { color: #ffffff; background: transparent; }"
        "QCheckBox { background: transparent; }"
        "QCheckBox::indicator { width: 86px; height: 44px; }"
        "QCheckBox::indicator:unchecked {"
        "  border-radius: 22px; background: #2c2c2f; border: 1px solid rgba(255,255,255,0.12);"
        "  image: none;"
        "}"
        "QCheckBox::indicator:checked {"
        "  border-radius: 22px; background: #ff6a00; border: 1px solid rgba(255,255,255,0.08);"
        "  image: none;"
        "}"
    ));

    m_title->setStyleSheet(QStringLiteral("color: #ff3b30; background: transparent;"));
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
        if (!reason.trimmed().isEmpty()) {
            m_subtitle->setText(reason.trimmed());
        } else {
            m_subtitle->setText(QStringLiteral("Your Internet is <span style='color:#ff6a00;font-weight:600'>not private</span>."));
        }
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
