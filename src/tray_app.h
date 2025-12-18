#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QString>

class QAction;
class QMenu;
class QTimer;
class QWidgetAction;
class QWidget;

class WarpPopup;
class SettingsMenu;

#include "warp_cli.h"

class TrayApp : public QObject {
    Q_OBJECT

public:
    explicit TrayApp(QObject *parent = nullptr);
    void start();

private slots:
    void refreshStatus();
    void refreshSettings();
    void updateZeroTrustStatus();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void showPopup();
    void hidePopup();

private:
    void connectWarp();
    void disconnectWarp();
    void registerClient();
    void enrollOrg();
    void attachLicense();

    void onWarpFinished(const QString &requestId, const WarpResult &result);

    void updateFromStatusJson(const QByteArray &jsonBytes);
    void updateFromSettingsText(const QString &settingsText);
    void setBusy(bool busy);
    void applyUiState();

    static QString normalizeStatus(const QString &status);

    WarpCli m_warp;

    QSystemTrayIcon *m_tray;
    QMenu *m_menu;

    QAction *m_statusAction;
    QAction *m_connectAction;
    QAction *m_disconnectAction;
    QAction *m_registerAction;
    QAction *m_enrollAction;
    QAction *m_licenseAction;
    QAction *m_refreshAction;
    QAction *m_quitAction;

    QTimer *m_poll;

    WarpPopup *m_popup;
    SettingsMenu *m_settingsMenu;

    QString m_currentStatus;
    QString m_currentReason;
    QString m_currentMode;
    bool m_busy;
    bool m_isZeroTrust;
    QPoint m_lastCursorPos; // Store cursor position when tray is clicked
    QPoint m_popupOffset; // User's custom popup position offset
    
    void savePopupOffset(const QPoint &offset);
    QPoint loadPopupOffset();
};
