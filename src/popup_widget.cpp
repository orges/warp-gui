#include "popup_widget.h"
#include "toggle_switch.h"
#include "wayland_popup_helper.h"

#include <QApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QtMath>

WarpPopup::WarpPopup(QWidget *parent)
    : QWidget(parent),
      m_title(new QLabel(QStringLiteral("WARP"), this)),
      m_toggle(new ToggleSwitch(this)),
      m_status(new QLabel(QStringLiteral("…"), this)),
      m_subtitle(new QLabel(QStringLiteral(""), this)),
      m_bottomBar(new QWidget(this)),
      m_brandingLabel(new QLabel(QStringLiteral("WARP\nby Cloudflare"), m_bottomBar)),
      m_settingsBtn(new QPushButton(m_bottomBar)),
      m_busy(false),
      m_currentMode(QStringLiteral("warp")),
      m_isZeroTrust(false),
      m_anchorBottom(true), // Default to bottom panel
      m_currentPosition(0, 0),
      m_dragging(false),
      m_dragStartPos(0, 0),
      m_windowStartPos(0, 0) {
    // Enable transparency for rounded corners
    setAttribute(Qt::WA_TranslucentBackground);
    // Use FramelessWindowHint for custom styling, WindowStaysOnTopHint for overlay behavior
    // Qt::Popup tells Qt this should close when clicking outside
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Popup);

    // Make the window close when clicking outside or pressing Escape
    setAttribute(Qt::WA_DeleteOnClose, false);

    // Set focus policy to receive focus events
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Main content area (rounded box)
    auto *contentWidget = new QWidget(this);
    contentWidget->setObjectName(QStringLiteral("contentWidget"));
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(18, 18, 18, 18);
    contentLayout->setSpacing(12);

    // Drag handle indicator (three dots)
    auto *dragHandle = new QLabel(QStringLiteral("⋮⋮⋮"), this);
    QFont handleFont = dragHandle->font();
    handleFont.setPointSize(10);
    dragHandle->setFont(handleFont);
    dragHandle->setAlignment(Qt::AlignCenter);
    dragHandle->setStyleSheet(QStringLiteral("color: #666666; background: transparent;"));
    dragHandle->setToolTip(QStringLiteral("Drag to reposition popup"));
    dragHandle->setCursor(Qt::OpenHandCursor);
    contentLayout->addWidget(dragHandle);
    
    contentLayout->addSpacing(-8); // Reduce space after drag handle

    // Title centered at top with drag hint
    QFont titleFont = m_title->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    m_title->setAlignment(Qt::AlignCenter);
    m_title->setToolTip(QStringLiteral("Click and drag to reposition"));
    m_title->setCursor(Qt::OpenHandCursor);
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

    setFixedSize(260, 332);
}

void WarpPopup::applyStyle() {
    setStyleSheet(QStringLiteral(
        "WarpPopup { "
        "  background-color: transparent; " // Transparent for arrow
        "}"
        "#contentWidget { "
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

    // Title color will be set dynamically in updateTitle()
    updateTitleColor();

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
    QString normalizedMode = m_currentMode.toLower();
    bool isDnsOnlyMode = normalizedMode.contains(QStringLiteral("dnsover")) ||
                         normalizedMode == QStringLiteral("doh") ||
                         normalizedMode == QStringLiteral("dot");

    if (lower == QStringLiteral("connected")) {
        // Zero Trust and WARP modes always say "Internet", DNS-only says "DNS queries"
        if (isDnsOnlyMode && !m_isZeroTrust) {
            m_subtitle->setText(QStringLiteral("Your DNS queries are <span style='color:#ff6a00;font-weight:600'>private</span>."));
        } else {
            m_subtitle->setText(QStringLiteral("Your Internet is <span style='color:#ff6a00;font-weight:600'>private</span>."));
        }
        m_toggle->blockSignals(true);
        m_toggle->setChecked(true);
        m_toggle->blockSignals(false);
    } else if (lower == QStringLiteral("connecting")) {
        m_subtitle->setText(QStringLiteral("Connecting…"));
        m_toggle->blockSignals(true);
        m_toggle->setChecked(true);
        m_toggle->blockSignals(false);
    } else {
        // Zero Trust and WARP modes always say "Internet", DNS-only says "DNS queries"
        if (isDnsOnlyMode && !m_isZeroTrust) {
            m_subtitle->setText(QStringLiteral("Your DNS queries are <span style='color:#ff6a00;font-weight:600'>not private</span>."));
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

    if (m_dragging) {
        return;
    }

    QWidget *focusWidget = qApp->focusWidget();
    if (focusWidget) {
        QWidget *topLevel = focusWidget->window();

        // Don't close if focus is still within the popup itself
        if (topLevel == this) {
            return;
        }

        QString className = topLevel->metaObject()->className();

        // Keep popup open for settings menu and preferences dialog
        if (className == QStringLiteral("SettingsMenu") ||
            className == QStringLiteral("PreferencesDialog")) {
            return;
        }

        emit requestClose();
    } else {
        // Focus went to desktop/other apps
        emit requestClose();
    }
}

void WarpPopup::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    qApp->installEventFilter(this);
}

void WarpPopup::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    qApp->removeEventFilter(this);
}

bool WarpPopup::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QWidget *clickedWidget = qApp->widgetAt(mouseEvent->globalPosition().toPoint());

        // Click inside popup
        if (clickedWidget && (this->isAncestorOf(clickedWidget) || clickedWidget == this)) {
            return QWidget::eventFilter(watched, event);
        }

        // Check if clicked on settings menu or preferences dialog
        if (clickedWidget) {
            QWidgetList topLevelWidgets = qApp->topLevelWidgets();
            for (QWidget *widget : topLevelWidgets) {
                if (widget == this) {
                    continue;
                }

                if (widget->isVisible() && (widget->isAncestorOf(clickedWidget) || widget == clickedWidget)) {
                    QString className = widget->metaObject()->className();
                    if (className == QStringLiteral("SettingsMenu") ||
                        className == QStringLiteral("PreferencesDialog")) {
                        return QWidget::eventFilter(watched, event);
                    }
                }
            }
        }

        // Clicked outside - close the popup
        emit requestClose();
        return false;
    }

    return QWidget::eventFilter(watched, event);
}

void WarpPopup::setMode(const QString &mode) {
    m_currentMode = mode;
    updateTitle();
}

void WarpPopup::setZeroTrust(bool isZeroTrust) {
    m_isZeroTrust = isZeroTrust;
    updateTitle();
}

void WarpPopup::updateTitle() {
    // Zero Trust takes priority
    if (m_isZeroTrust) {
        m_title->setText(QStringLiteral("Zero Trust"));
        updateTitleColor();
        return;
    }

    QString normalizedMode = m_currentMode.toLower();

    // DNS-only modes: DnsOverHttps, DnsOverTls, doh, dot
    if (normalizedMode.contains(QStringLiteral("dnsover")) ||
        normalizedMode == QStringLiteral("doh") ||
        normalizedMode == QStringLiteral("dot")) {
        m_title->setText(QStringLiteral("1.1.1.1"));
    } else {
        // WARP modes: Warp, WarpPlusDoh, WarpPlusDot, warp, warp+doh, warp+dot, proxy, tunnel_only
        m_title->setText(QStringLiteral("WARP"));
    }
    updateTitleColor();
}

void WarpPopup::updateTitleColor() {
    // Zero Trust uses blue, WARP and 1.1.1.1 use orange
    if (m_isZeroTrust) {
        m_title->setStyleSheet(QStringLiteral("color: #0A64BC; background: transparent; font-weight: bold;"));
    } else {
        m_title->setStyleSheet(QStringLiteral("color: #ff6a00; background: transparent; font-weight: bold;"));
    }
}

void WarpPopup::setAnchorBottom(bool anchorBottom) {
    m_anchorBottom = anchorBottom;
}

void WarpPopup::setCurrentPosition(const QPoint &pos) {
    m_currentPosition = pos;
}

void WarpPopup::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // Check if clicking on title area (top 50px) for dragging
        if (event->pos().y() < 50) {
            m_dragging = true;
            m_dragStartPos = event->globalPosition().toPoint();
            // Use stored LayerShell position (mapToGlobal returns 0,0 for LayerShell windows)
            m_windowStartPos = m_currentPosition;

            // Disable LayerShell for smooth dragging on Wayland
            WaylandPopupHelper::disableLayerShell(this);

            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void WarpPopup::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging) {
        // Don't update position during drag - LayerShell can't move in real-time
        // Position will update when drag ends (in mouseReleaseEvent)
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void WarpPopup::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);

        // Calculate the delta from drag
        QPoint currentGlobalPos = event->globalPosition().toPoint();
        QPoint dragDelta = currentGlobalPos - m_dragStartPos;

        // Only update position if actually dragged (more than 5 pixels)
        if (qAbs(dragDelta.x()) > 5 || qAbs(dragDelta.y()) > 5) {
            QPoint finalPos = m_windowStartPos + dragDelta;
            WaylandPopupHelper::enableLayerShell(this, finalPos, m_anchorBottom);
            emit positionChanged(dragDelta);
        } else {
            WaylandPopupHelper::enableLayerShell(this, m_windowStartPos, m_anchorBottom);
        }

        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
