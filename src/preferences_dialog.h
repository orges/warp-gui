#pragma once

#include <QDialog>
#include <QStackedWidget>
#include <QString>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QComboBox;
class QPushButton;
class QTextEdit;
class QCheckBox;

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

signals:
    void settingsChanged();

private slots:
    void onCategoryChanged(int index);
    void onProtocolChanged(int index);
    void onFamiliesModeChanged(int index);
    void refreshSettings();

private:
    void setupUi();
    void createGeneralPage();
    void createConnectionPage();
    void createAccountPage();
    void createSplitTunnelPage();
    void createDnsPage();
    void createAdvancedPage();
    void applyStyles();
    void loadCurrentSettings(const QString &settingsText);
    void updateAccountStatus(const QString &statusText);

    QListWidget *m_sidebar;
    QStackedWidget *m_contentStack;

    // General page widgets
    QLabel *m_statusLabel;
    QLabel *m_dnsProtocolLabel;
    QLabel *m_coloLabel;
    QLabel *m_connectionTypeLabel;
    QLabel *m_publicIpLabel;
    QLabel *m_deviceIdLabel;
    QPushButton *m_registerBtn;
    QPushButton *m_enrollOrgBtn;
    QPushButton *m_attachLicenseBtn;

    // Connection page widgets
    QComboBox *m_modeCombo;
    QComboBox *m_protocolCombo;
    QLabel *m_connectionInfoLabel;

    // Split Tunnel page widgets
    QTextEdit *m_excludedHostsText;
    QTextEdit *m_excludedIpsText;
    QPushButton *m_viewSplitTunnelBtn;

    // DNS page widgets
    QComboBox *m_familiesModeCombo;
    QLabel *m_dnsInfoLabel;

    // Advanced page widgets
    QCheckBox *m_autoConnectCheck;
    QLabel *m_advancedInfoLabel;

    QString m_currentSettings;
};
