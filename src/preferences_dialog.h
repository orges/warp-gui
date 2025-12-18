#pragma once

#include <QDialog>
#include <QStackedWidget>
#include <QString>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class QTextEdit;
class QCheckBox;
class QWidget;

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

signals:
    void settingsChanged();

private slots:
    void onCategoryChanged(int index);
    void onFamiliesModeConnectionChanged(int index);
    void onAddNetwork();
    void onRemoveNetwork();
    void onDisableWifiChanged(bool checked);
    void onDisableEthernetChanged(bool checked);
    void onGatewayDohChanged();
    void refreshSettings();

private:
    void setupUi();
    void createGeneralPage();
    void createConnectivityPage();
    void createConnectionPage();
    void createAccountPage();
    void createAdvancedPage();
    void applyStyles();
    void loadCurrentSettings(const QString &settingsText);
    void updateAccountStatus(const QString &statusText);
    void updateConnectionPageVisibility();
    void updateConnectivityStatus();
    bool isZeroTrustEnrolled();

    QListWidget *m_sidebar;
    QStackedWidget *m_contentStack;

    // General page widgets
    QLabel *m_statusLabel;
    QLabel *m_dnsProtocolLabel;
    QLabel *m_coloLabel;
    QLabel *m_connectionTypeLabel;
    QLabel *m_publicIpLabel;
    QLabel *m_deviceIdLabel;

    // Connectivity page widgets
    QLabel *m_apiConnectivityLabel;
    QLabel *m_dnsConnectivityLabel;
    QLabel *m_warpConnectivityLabel;
    QLabel *m_coloConnectivityLabel;


    // Connection page - Network exclusion (consumer only)
    QWidget *m_networkExclusionWidget;
    QListWidget *m_excludedNetworksList;
    QPushButton *m_addNetworkBtn;
    QPushButton *m_removeNetworkBtn;
    QCheckBox *m_disableWifiCheck;
    QCheckBox *m_disableEthernetCheck;

    // Connection page - Consumer only
    QWidget *m_consumerDnsWidget;
    QComboBox *m_familiesModeComboConnection;

    // Connection page - Zero Trust only
    QWidget *m_zeroTrustDnsWidget;
    QLineEdit *m_gatewayDohInput;

    // Split Tunnel page widgets
    QTextEdit *m_excludedHostsText;
    QTextEdit *m_excludedIpsText;
    QPushButton *m_viewSplitTunnelBtn;

    // Advanced page widgets
    QCheckBox *m_autoConnectCheck;
    QLabel *m_advancedInfoLabel;

    QString m_currentSettings;
    bool m_isZeroTrust;
};
