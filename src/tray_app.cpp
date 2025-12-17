#include "tray_app.h"

#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QGuiApplication>
#include <QIcon>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QWindow>
#include <QWidgetAction>

#include "popup_widget.h"
#include "preferences_dialog.h"
#include "settings_menu.h"
#include "wayland_popup_helper.h"

TrayApp::TrayApp(QObject *parent)
    : QObject(parent),
      m_warp(this),
      m_tray(new QSystemTrayIcon(this)),
      m_menu(new QMenu()),
      m_statusAction(new QAction(QStringLiteral("Status: …"), m_menu)),
      m_connectAction(new QAction(QStringLiteral("Connect"), m_menu)),
      m_disconnectAction(new QAction(QStringLiteral("Disconnect"), m_menu)),
      m_registerAction(new QAction(QStringLiteral("Register"), m_menu)),
      m_enrollAction(new QAction(QStringLiteral("Enroll Zero Trust Organization…"), m_menu)),
      m_licenseAction(new QAction(QStringLiteral("Attach License Key…"), m_menu)),
      m_refreshAction(new QAction(QStringLiteral("Refresh"), m_menu)),
      m_quitAction(new QAction(QStringLiteral("Quit"), m_menu)),
      m_poll(new QTimer(this)),
      m_popup(new WarpPopup()),
      m_settingsMenu(new SettingsMenu()),
      m_currentStatus(QStringLiteral("…")),
      m_currentMode(QStringLiteral("warp")),
      m_busy(false) {
    connect(&m_warp, &WarpCli::finished, this, &TrayApp::onWarpFinished);

    m_tray->setIcon(QIcon::fromTheme(QStringLiteral("network-vpn")));
    m_tray->setToolTip(QStringLiteral("WARP"));

    connect(m_tray, &QSystemTrayIcon::activated, this, &TrayApp::onTrayActivated);

    m_statusAction->setEnabled(false);

    m_menu->addAction(m_statusAction);
    m_menu->addSeparator();
    m_menu->addAction(m_connectAction);
    m_menu->addAction(m_disconnectAction);
    m_menu->addSeparator();
    m_menu->addAction(m_registerAction);
    m_menu->addAction(m_enrollAction);
    m_menu->addAction(m_licenseAction);
    m_menu->addSeparator();
    m_menu->addAction(m_refreshAction);
    m_menu->addSeparator();
    m_menu->addAction(m_quitAction);

    m_tray->setContextMenu(m_menu);

    connect(m_connectAction, &QAction::triggered, this, &TrayApp::connectWarp);
    connect(m_disconnectAction, &QAction::triggered, this, &TrayApp::disconnectWarp);
    connect(m_registerAction, &QAction::triggered, this, &TrayApp::registerClient);
    connect(m_enrollAction, &QAction::triggered, this, &TrayApp::enrollOrg);
    connect(m_licenseAction, &QAction::triggered, this, &TrayApp::attachLicense);
    connect(m_refreshAction, &QAction::triggered, this, &TrayApp::refreshStatus);
    connect(m_quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_poll->setInterval(5000);
    connect(m_poll, &QTimer::timeout, this, &TrayApp::refreshStatus);

    connect(m_popup, &WarpPopup::requestConnect, this, &TrayApp::connectWarp);
    connect(m_popup, &WarpPopup::requestDisconnect, this, &TrayApp::disconnectWarp);
    connect(m_popup, &WarpPopup::requestClose, this, &TrayApp::hidePopup);
    connect(m_popup, &WarpPopup::requestSettings, this, [this]() {
        // Show custom settings menu
        // Position it below the settings button or at bottom-left of popup
        QPoint menuPos = m_popup->mapToGlobal(QPoint(8, m_popup->height() - 8));
        m_settingsMenu->move(menuPos);
        m_settingsMenu->show();
        m_settingsMenu->raise();
        m_settingsMenu->activateWindow();
    });

    // Connect settings menu signals
    connect(m_settingsMenu, &SettingsMenu::preferencesRequested, this, [this]() {
        auto *prefs = new PreferencesDialog();
        connect(prefs, &PreferencesDialog::settingsChanged, this, &TrayApp::refreshSettings);
        prefs->setAttribute(Qt::WA_DeleteOnClose);
        prefs->show();
    });
    connect(m_settingsMenu, &SettingsMenu::aboutRequested, this, [this]() {
        QMessageBox::about(nullptr, QStringLiteral("About Cloudflare WARP"),
                          QStringLiteral("Cloudflare WARP GUI\nUnofficial Qt-based GUI for warp-cli"));
    });
    connect(m_settingsMenu, &SettingsMenu::exitRequested, qApp, &QApplication::quit);
    connect(m_settingsMenu, &SettingsMenu::modeChangeRequested, this, [this](const QString &targetMode) {
        m_warp.run(QStringLiteral("set_mode"), QStringList{QStringLiteral("mode"), targetMode});
        setBusy(true);
    });

    // Setup popup window
    m_popup->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    m_popup->hide();

    applyUiState();
}

void TrayApp::start() {
    m_tray->show();
    refreshStatus();
    refreshSettings();
    m_poll->start();
}

void TrayApp::refreshStatus() {
    m_warp.runJson(QStringLiteral("status"), QStringList{QStringLiteral("status")});
}

void TrayApp::refreshSettings() {
    m_warp.run(QStringLiteral("settings"), QStringList{QStringLiteral("settings")});
}

void TrayApp::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        showPopup();
    }
}

void TrayApp::showPopup() {
    if (!m_popup) {
        return;
    }

    applyUiState();

    if (m_popup->isVisible()) {
        m_popup->hide();
        return;
    }

    // On Wayland, tray geometry is not available and cursor position doesn't work
    // Position near system tray which is typically in a panel
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        qDebug() << "No primary screen available!";
        return;
    }

    QRect screenGeom = screen->geometry();
    QRect availGeom = screen->availableGeometry();

    // On Wayland, panel detection often doesn't work (especially with auto-hide panels)
    // For now, hardcode to bottom-right which is most common for system tray
    const bool panelAtBottom = true;
    const int estimatedPanelHeight = 48;
    const int rightPadding = 16;
    const int popupSpacing = 8; // Space between panel and popup

    QPoint popupPos;
    if (panelAtBottom) {
        // Panel at bottom - position popup just above the panel
        popupPos = QPoint(
            screenGeom.right() - m_popup->width() - rightPadding,
            screenGeom.bottom() - m_popup->height() - estimatedPanelHeight - popupSpacing
        );
    } else {
        // Panel at top - position popup just below the panel
        popupPos = QPoint(
            screenGeom.right() - m_popup->width() - rightPadding,
            screenGeom.top() + estimatedPanelHeight + popupSpacing
        );
    }

    // Create native window if not already created
    if (!m_popup->windowHandle()) {
        m_popup->winId();
    }

    // Use Wayland helper to set up positioning BEFORE showing
    WaylandPopupHelper::setupPopupWindow(m_popup, popupPos, panelAtBottom);

    m_popup->show();
    m_popup->raise();
    m_popup->activateWindow();
}

void TrayApp::hidePopup() {
    if (m_popup) {
        m_popup->hide();
    }
}

void TrayApp::connectWarp() {
    m_warp.run(QStringLiteral("connect"), QStringList{QStringLiteral("connect")});
    setBusy(true);
}

void TrayApp::disconnectWarp() {
    m_warp.run(QStringLiteral("disconnect"), QStringList{QStringLiteral("disconnect")});
    setBusy(true);
}

void TrayApp::registerClient() {
    m_warp.run(QStringLiteral("registration_new"), QStringList{QStringLiteral("registration"), QStringLiteral("new")});
    setBusy(true);
}

void TrayApp::enrollOrg() {
    bool ok = false;
    const QString org = QInputDialog::getText(nullptr,
                                              QStringLiteral("Zero Trust Enrollment"),
                                              QStringLiteral("Organization:"),
                                              QLineEdit::Normal,
                                              QString(),
                                              &ok)
                            .trimmed();
    if (!ok || org.isEmpty()) {
        return;
    }

    m_warp.run(QStringLiteral("registration_new"),
               QStringList{QStringLiteral("registration"), QStringLiteral("new"), org});
    setBusy(true);
}

void TrayApp::attachLicense() {
    bool ok = false;
    const QString key = QInputDialog::getText(nullptr,
                                              QStringLiteral("Attach License"),
                                              QStringLiteral("License key:"),
                                              QLineEdit::Password,
                                              QString(),
                                              &ok)
                            .trimmed();
    if (!ok || key.isEmpty()) {
        return;
    }

    m_warp.run(QStringLiteral("license"),
               QStringList{QStringLiteral("registration"), QStringLiteral("license"), key});
    setBusy(true);
}

void TrayApp::onWarpFinished(const QString &requestId, const WarpResult &result) {
    if (requestId == QStringLiteral("status")) {
        const QByteArray jsonBytes = result.stdoutText.toUtf8();
        updateFromStatusJson(jsonBytes);
        applyUiState();
        return;
    }

    if (requestId == QStringLiteral("settings")) {
        if (result.exitCode == 0) {
            updateFromSettingsText(result.stdoutText);
            applyUiState();
        }
        return;
    }

    // Only show error dialogs for registration/license commands
    // Connect/disconnect/set-mode should be silent and show status in popup
    if (result.exitCode != 0 &&
        (requestId == QStringLiteral("registration_new") ||
         requestId == QStringLiteral("license"))) {
        const QString msg = !result.stderrText.trimmed().isEmpty()
                                ? result.stderrText.trimmed()
                                : (!result.stdoutText.trimmed().isEmpty() ? result.stdoutText.trimmed()
                                                                          : QStringLiteral("warp-cli failed"));
        QMessageBox::critical(nullptr, QStringLiteral("WARP"), msg);
    } else if (result.exitCode == 0 &&
               (requestId == QStringLiteral("registration_new") ||
                requestId == QStringLiteral("license"))) {
        const QString msg = !result.stdoutText.trimmed().isEmpty() ? result.stdoutText.trimmed()
                                                                   : QStringLiteral("Command completed");
        QMessageBox::information(nullptr, QStringLiteral("WARP"), msg);
    }

    setBusy(false);
    refreshStatus();

    // Refresh settings after mode change to update the checkmark
    if (requestId == QStringLiteral("set_mode")) {
        refreshSettings();
    }
}

void TrayApp::updateFromStatusJson(const QByteArray &jsonBytes) {
    const auto doc = QJsonDocument::fromJson(jsonBytes);
    if (!doc.isObject()) {
        m_currentStatus = QStringLiteral("Error");
        m_currentReason = QStringLiteral("Invalid JSON from warp-cli");
        return;
    }

    const QJsonObject obj = doc.object();
    m_currentStatus = obj.value(QStringLiteral("status")).toString(QStringLiteral("Unknown"));

    const auto reasonVal = obj.value(QStringLiteral("reason"));
    if (reasonVal.isObject()) {
        const QJsonObject r = reasonVal.toObject();
        if (!r.isEmpty()) {
            const QString key = r.keys().first();
            m_currentReason = key + QStringLiteral(": ") + r.value(key).toString();
        } else {
            m_currentReason.clear();
        }
    } else if (reasonVal.isString()) {
        m_currentReason = reasonVal.toString();
    } else {
        m_currentReason.clear();
    }
}

void TrayApp::updateFromSettingsText(const QString &settingsText) {
    // Parse the mode from settings output
    // Looking for lines like "(default)	Mode: Warp" or "(override)	Mode: Doh"
    const QStringList lines = settingsText.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (line.contains(QStringLiteral("Mode:"), Qt::CaseInsensitive)) {
            // Extract the mode value after "Mode: "
            const int modeIndex = line.indexOf(QStringLiteral("Mode:"), 0, Qt::CaseInsensitive);
            if (modeIndex >= 0) {
                QString mode = line.mid(modeIndex + 5).trimmed();
                m_currentMode = mode.toLower();
                break;
            }
        }
    }
}

void TrayApp::setBusy(bool busy) {
    m_busy = busy;
    applyUiState();
}

QString TrayApp::normalizeStatus(const QString &status) {
    return status.trimmed().toLower();
}

void TrayApp::applyUiState() {
    QString tooltip = QStringLiteral("WARP\nStatus: ") + m_currentStatus;
    if (!m_currentReason.isEmpty()) {
        tooltip += QStringLiteral("\n") + m_currentReason;
    }

    m_tray->setToolTip(tooltip);

    QString statusLine = QStringLiteral("Status: ") + m_currentStatus;
    if (!m_currentReason.isEmpty()) {
        statusLine += QStringLiteral(" (") + m_currentReason + QStringLiteral(")");
    }
    m_statusAction->setText(statusLine);

    const QString normalized = normalizeStatus(m_currentStatus);
    const bool connected = (normalized == QStringLiteral("connected"));
    const bool connecting = (normalized == QStringLiteral("connecting"));

    const bool canConnect = !connected && !connecting;
    const bool canDisconnect = connected || connecting;

    m_connectAction->setEnabled(!m_busy && canConnect);
    m_disconnectAction->setEnabled(!m_busy && canDisconnect);
    m_registerAction->setEnabled(!m_busy);
    m_enrollAction->setEnabled(!m_busy);
    m_licenseAction->setEnabled(!m_busy);
    m_refreshAction->setEnabled(!m_busy);

    if (m_popup) {
        m_popup->setBusy(m_busy);
        m_popup->setStatusText(m_currentStatus, m_currentReason);
        m_popup->setMode(m_currentMode);
    }

    if (m_settingsMenu) {
        m_settingsMenu->setActionsEnabled(!m_busy);
        m_settingsMenu->setCurrentMode(m_currentMode);
    }

    if (connected) {
        m_tray->setIcon(QIcon::fromTheme(QStringLiteral("network-vpn")));
    } else if (connecting) {
        m_tray->setIcon(QIcon::fromTheme(QStringLiteral("network-idle")));
    } else {
        QIcon icon = QIcon::fromTheme(QStringLiteral("network-vpn-disconnected"));
        if (icon.isNull()) {
            icon = QIcon::fromTheme(QStringLiteral("network-offline"));
        }
        m_tray->setIcon(icon);
    }
}
