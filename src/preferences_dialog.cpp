#include "preferences_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QTextEdit>
#include <QVBoxLayout>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent),
      m_sidebar(new QListWidget(this)),
      m_contentStack(new QStackedWidget(this)) {

    setWindowTitle(QStringLiteral("WARP Preferences"));
    setMinimumSize(750, 500);

    setupUi();
    applyStyles();
    refreshSettings();
}

void PreferencesDialog::setupUi() {
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Sidebar
    m_sidebar->setFixedWidth(180);
    m_sidebar->addItem(QStringLiteral("General"));
    m_sidebar->addItem(QStringLiteral("Connection"));
    m_sidebar->addItem(QStringLiteral("Split Tunneling"));
    m_sidebar->addItem(QStringLiteral("DNS & Families"));
    m_sidebar->addItem(QStringLiteral("Advanced"));

    connect(m_sidebar, &QListWidget::currentRowChanged, this, &PreferencesDialog::onCategoryChanged);

    // Content pages
    createGeneralPage();
    createConnectionPage();
    createSplitTunnelPage();
    createDnsPage();
    createAdvancedPage();

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_contentStack, 1);

    m_sidebar->setCurrentRow(0);
}

void PreferencesDialog::createGeneralPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    // Header
    auto *header = new QLabel(QStringLiteral("General"));
    QFont headerFont = header->font();
    headerFont.setPointSize(16);
    headerFont.setBold(true);
    header->setFont(headerFont);
    layout->addWidget(header);

    // Connectivity Information group
    auto *connectivityGroup = new QGroupBox(QStringLiteral("Connectivity Information"));
    auto *connectivityLayout = new QFormLayout(connectivityGroup);
    connectivityLayout->setSpacing(8);

    auto *connectionLabel = new QLabel(QStringLiteral("Loading..."));
    auto *dnsProtocolLabel = new QLabel(QStringLiteral("Loading..."));
    auto *coloLabel = new QLabel(QStringLiteral("Loading..."));

    connectivityLayout->addRow(QStringLiteral("Connection:"), connectionLabel);
    connectivityLayout->addRow(QStringLiteral("DNS Protocol:"), dnsProtocolLabel);
    connectivityLayout->addRow(QStringLiteral("Colocation center:"), coloLabel);

    layout->addWidget(connectivityGroup);

    // Your Device group
    auto *deviceGroup = new QGroupBox(QStringLiteral("Your Device"));
    auto *deviceLayout = new QFormLayout(deviceGroup);
    deviceLayout->setSpacing(8);

    auto *connectionTypeLabel = new QLabel(QStringLiteral("Loading..."));
    auto *publicIpLabel = new QLabel(QStringLiteral("Loading..."));
    auto *deviceIdLabel = new QLabel(QStringLiteral("Loading..."));

    deviceLayout->addRow(QStringLiteral("Connection type:"), connectionTypeLabel);
    deviceLayout->addRow(QStringLiteral("Public IP:"), publicIpLabel);
    deviceLayout->addRow(QStringLiteral("Device ID:"), deviceIdLabel);

    layout->addWidget(deviceGroup);

    // Store labels for updates
    m_statusLabel = connectionLabel;
    m_dnsProtocolLabel = dnsProtocolLabel;
    m_coloLabel = coloLabel;
    m_connectionTypeLabel = connectionTypeLabel;
    m_publicIpLabel = publicIpLabel;
    m_deviceIdLabel = deviceIdLabel;

    // Refresh button
    auto *refreshBtn = new QPushButton(QStringLiteral("Refresh Connection Info"));
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        refreshSettings();
    });
    layout->addWidget(refreshBtn);

    // Account actions group
    auto *actionsGroup = new QGroupBox(QStringLiteral("Account Management"));
    auto *actionsLayout = new QVBoxLayout(actionsGroup);

    m_registerBtn = new QPushButton(QStringLiteral("Register Device"));
    m_enrollOrgBtn = new QPushButton(QStringLiteral("Enroll Zero Trust Organization"));
    m_attachLicenseBtn = new QPushButton(QStringLiteral("Attach WARP+ License"));

    connect(m_registerBtn, &QPushButton::clicked, this, [this]() {
        QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("registration"), QStringLiteral("new")});
        QMessageBox::information(this, QStringLiteral("Register"), QStringLiteral("Registration command executed. Check warp-cli status."));
        refreshSettings();
    });

    connect(m_enrollOrgBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString org = QInputDialog::getText(this, QStringLiteral("Enroll Organization"),
                                           QStringLiteral("Organization name:"), QLineEdit::Normal,
                                           QString(), &ok);
        if (ok && !org.isEmpty()) {
            QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("registration"), QStringLiteral("new"), org});
            QMessageBox::information(this, QStringLiteral("Enroll"), QStringLiteral("Enrollment command executed."));
            refreshSettings();
        }
    });

    connect(m_attachLicenseBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString key = QInputDialog::getText(this, QStringLiteral("Attach License"),
                                           QStringLiteral("License key:"), QLineEdit::Password,
                                           QString(), &ok);
        if (ok && !key.isEmpty()) {
            QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("registration"), QStringLiteral("license"), key});
            QMessageBox::information(this, QStringLiteral("License"), QStringLiteral("License command executed."));
            refreshSettings();
        }
    });

    actionsLayout->addWidget(m_registerBtn);
    actionsLayout->addWidget(m_enrollOrgBtn);
    actionsLayout->addWidget(m_attachLicenseBtn);

    layout->addWidget(actionsGroup);
    layout->addStretch();

    m_contentStack->addWidget(page);
}

void PreferencesDialog::createConnectionPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    // Header
    auto *header = new QLabel(QStringLiteral("Connection"));
    QFont headerFont = header->font();
    headerFont.setPointSize(16);
    headerFont.setBold(true);
    header->setFont(headerFont);
    layout->addWidget(header);

    // Mode settings
    auto *modeGroup = new QGroupBox(QStringLiteral("Connection Mode"));
    auto *modeLayout = new QFormLayout(modeGroup);

    m_modeCombo = new QComboBox();
    m_modeCombo->addItem(QStringLiteral("WARP"), QStringLiteral("warp"));
    m_modeCombo->addItem(QStringLiteral("WARP with DoH"), QStringLiteral("warp+doh"));
    m_modeCombo->addItem(QStringLiteral("DNS Only (DoH)"), QStringLiteral("doh"));
    m_modeCombo->addItem(QStringLiteral("DNS Only (DoT)"), QStringLiteral("dot"));
    m_modeCombo->addItem(QStringLiteral("WARP with DoT"), QStringLiteral("warp+dot"));
    m_modeCombo->addItem(QStringLiteral("Proxy Mode"), QStringLiteral("proxy"));

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        QString mode = m_modeCombo->currentData().toString();
        QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("mode"), mode});
        emit settingsChanged();
    });

    modeLayout->addRow(QStringLiteral("Mode:"), m_modeCombo);

    auto *modeDesc = new QLabel(
        QStringLiteral("WARP: Full tunnel with UDP DNS\n"
                      "WARP+DoH: Full tunnel with DNS over HTTPS\n"
                      "DNS Only: No tunnel, only DNS protection"));
    modeDesc->setWordWrap(true);
    modeDesc->setStyleSheet(QStringLiteral("color: #999; font-size: 11px;"));
    modeLayout->addRow(modeDesc);

    layout->addWidget(modeGroup);

    // Protocol settings
    auto *protocolGroup = new QGroupBox(QStringLiteral("Tunnel Protocol"));
    auto *protocolLayout = new QFormLayout(protocolGroup);

    m_protocolCombo = new QComboBox();
    m_protocolCombo->addItem(QStringLiteral("MASQUE (HTTP/3)"), QStringLiteral("masque"));
    m_protocolCombo->addItem(QStringLiteral("WireGuard"), QStringLiteral("wireguard"));

    connect(m_protocolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onProtocolChanged);

    protocolLayout->addRow(QStringLiteral("Protocol:"), m_protocolCombo);

    auto *protocolDesc = new QLabel(
        QStringLiteral("MASQUE: Modern HTTP/3-based protocol (default)\n"
                      "WireGuard: Traditional VPN protocol"));
    protocolDesc->setWordWrap(true);
    protocolDesc->setStyleSheet(QStringLiteral("color: #999; font-size: 11px;"));
    protocolLayout->addRow(protocolDesc);

    layout->addWidget(protocolGroup);

    // Connection info
    m_connectionInfoLabel = new QLabel();
    m_connectionInfoLabel->setWordWrap(true);
    m_connectionInfoLabel->setStyleSheet(QStringLiteral("background: #2a2a2a; padding: 10px; border-radius: 5px; border: 1px solid #3a3a3a;"));
    layout->addWidget(m_connectionInfoLabel);

    layout->addStretch();

    m_contentStack->addWidget(page);
}

void PreferencesDialog::createSplitTunnelPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    // Header
    auto *header = new QLabel(QStringLiteral("Split Tunneling"));
    QFont headerFont = header->font();
    headerFont.setPointSize(16);
    headerFont.setBold(true);
    header->setFont(headerFont);
    layout->addWidget(header);

    auto *desc = new QLabel(
        QStringLiteral("Configure which traffic should bypass the WARP tunnel. "
                      "By default, local network traffic is excluded."));
    desc->setWordWrap(true);
    desc->setStyleSheet(QStringLiteral("color: #999;"));
    layout->addWidget(desc);

    // View current split tunnel
    m_viewSplitTunnelBtn = new QPushButton(QStringLiteral("View Live Routing Dump"));
    connect(m_viewSplitTunnelBtn, &QPushButton::clicked, this, [this]() {
        QProcess process;
        process.start(QStringLiteral("warp-cli"), {QStringLiteral("tunnel"), QStringLiteral("dump")});
        process.waitForFinished();
        QString output = QString::fromUtf8(process.readAllStandardOutput());

        if (process.exitCode() != 0) {
            output = QStringLiteral("Error: WARP must be connected to view live routing dump.\n\n") +
                     QString::fromUtf8(process.readAllStandardError());
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle(QStringLiteral("Live Routing Dump"));
        msgBox.setText(output);
        msgBox.setDetailedText(output);
        msgBox.exec();
    });
    layout->addWidget(m_viewSplitTunnelBtn);

    // Excluded hosts
    auto *hostsGroup = new QGroupBox(QStringLiteral("Excluded Hosts & Fallback Domains"));
    auto *hostsLayout = new QVBoxLayout(hostsGroup);

    auto *hostsLabel = new QLabel(QStringLiteral("Domains excluded from WARP tunnel (read-only):"));
    hostsLayout->addWidget(hostsLabel);

    m_excludedHostsText = new QTextEdit();
    m_excludedHostsText->setPlaceholderText(QStringLiteral("No host exclusions configured"));
    m_excludedHostsText->setMaximumHeight(100);
    m_excludedHostsText->setReadOnly(true);
    hostsLayout->addWidget(m_excludedHostsText);

    auto *addHostBtn = new QPushButton(QStringLiteral("Add Host to Exclusions"));
    connect(addHostBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString host = QInputDialog::getText(this, QStringLiteral("Add Host"),
                                            QStringLiteral("Hostname or domain:"), QLineEdit::Normal,
                                            QString(), &ok);
        if (ok && !host.isEmpty()) {
            QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("tunnel"), QStringLiteral("host"), QStringLiteral("add"), host});
            QMessageBox::information(this, QStringLiteral("Host Added"), QStringLiteral("Host added to split tunnel exclusions."));
        }
    });
    hostsLayout->addWidget(addHostBtn);

    layout->addWidget(hostsGroup);

    // Excluded IPs
    auto *ipsGroup = new QGroupBox(QStringLiteral("Excluded IP Ranges"));
    auto *ipsLayout = new QVBoxLayout(ipsGroup);

    auto *ipsLabel = new QLabel(QStringLiteral("IP ranges excluded from tunnel (read-only):"));
    ipsLayout->addWidget(ipsLabel);

    m_excludedIpsText = new QTextEdit();
    m_excludedIpsText->setPlaceholderText(QStringLiteral("No IP exclusions configured"));
    m_excludedIpsText->setMaximumHeight(120);
    m_excludedIpsText->setReadOnly(true);
    ipsLayout->addWidget(m_excludedIpsText);

    auto *addIpBtn = new QPushButton(QStringLiteral("Add IP Range to Exclusions"));
    connect(addIpBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString ip = QInputDialog::getText(this, QStringLiteral("Add IP Range"),
                                          QStringLiteral("IP range (CIDR):"), QLineEdit::Normal,
                                          QString(), &ok);
        if (ok && !ip.isEmpty()) {
            QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("tunnel"), QStringLiteral("ip"), QStringLiteral("add"), ip});
            QMessageBox::information(this, QStringLiteral("IP Added"), QStringLiteral("IP range added to split tunnel exclusions."));
        }
    });
    ipsLayout->addWidget(addIpBtn);

    layout->addWidget(ipsGroup);

    layout->addStretch();

    m_contentStack->addWidget(page);
}

void PreferencesDialog::createDnsPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    // Header
    auto *header = new QLabel(QStringLiteral("DNS & Families"));
    QFont headerFont = header->font();
    headerFont.setPointSize(16);
    headerFont.setBold(true);
    header->setFont(headerFont);
    layout->addWidget(header);

    // Families mode
    auto *familiesGroup = new QGroupBox(QStringLiteral("1.1.1.1 for Families"));
    auto *familiesLayout = new QFormLayout(familiesGroup);

    auto *familiesDesc = new QLabel(
        QStringLiteral("Protect your home network from malware and adult content."));
    familiesDesc->setWordWrap(true);
    familiesDesc->setStyleSheet(QStringLiteral("color: #999; margin-bottom: 10px;"));
    familiesLayout->addRow(familiesDesc);

    m_familiesModeCombo = new QComboBox();
    m_familiesModeCombo->addItem(QStringLiteral("Off"), QStringLiteral("off"));
    m_familiesModeCombo->addItem(QStringLiteral("Malware Protection"), QStringLiteral("malware"));
    m_familiesModeCombo->addItem(QStringLiteral("Malware + Adult Content"), QStringLiteral("full"));

    connect(m_familiesModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onFamiliesModeChanged);

    familiesLayout->addRow(QStringLiteral("Protection level:"), m_familiesModeCombo);

    layout->addWidget(familiesGroup);

    // DNS info
    auto *dnsGroup = new QGroupBox(QStringLiteral("DNS Settings"));
    auto *dnsLayout = new QVBoxLayout(dnsGroup);

    m_dnsInfoLabel = new QLabel();
    m_dnsInfoLabel->setWordWrap(true);
    dnsLayout->addWidget(m_dnsInfoLabel);

    auto *dnsStatsBtn = new QPushButton(QStringLiteral("View DNS Statistics"));
    connect(dnsStatsBtn, &QPushButton::clicked, this, []() {
        QProcess process;
        process.start(QStringLiteral("warp-cli"), {QStringLiteral("dns"), QStringLiteral("stats")});
        process.waitForFinished();
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QMessageBox::information(nullptr, QStringLiteral("DNS Stats"), output);
    });
    dnsLayout->addWidget(dnsStatsBtn);

    layout->addWidget(dnsGroup);

    layout->addStretch();

    m_contentStack->addWidget(page);
}

void PreferencesDialog::createAdvancedPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    // Header
    auto *header = new QLabel(QStringLiteral("Advanced"));
    QFont headerFont = header->font();
    headerFont.setPointSize(16);
    headerFont.setBold(true);
    header->setFont(headerFont);
    layout->addWidget(header);

    // Advanced settings
    auto *settingsGroup = new QGroupBox(QStringLiteral("Advanced Settings"));
    auto *settingsLayout = new QVBoxLayout(settingsGroup);

    m_autoConnectCheck = new QCheckBox(QStringLiteral("Always stay connected (requires daemon configuration)"));
    m_autoConnectCheck->setToolTip(QStringLiteral("This requires configuring the warp-svc daemon"));
    settingsLayout->addWidget(m_autoConnectCheck);

    layout->addWidget(settingsGroup);

    // Diagnostics
    auto *diagGroup = new QGroupBox(QStringLiteral("Diagnostics"));
    auto *diagLayout = new QVBoxLayout(diagGroup);

    auto *viewStatsBtn = new QPushButton(QStringLiteral("View Connection Statistics"));
    connect(viewStatsBtn, &QPushButton::clicked, this, []() {
        QProcess process;
        process.start(QStringLiteral("warp-cli"), {QStringLiteral("tunnel"), QStringLiteral("stats")});
        process.waitForFinished();
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QMessageBox::information(nullptr, QStringLiteral("Tunnel Stats"), output);
    });
    diagLayout->addWidget(viewStatsBtn);

    auto *rotateKeysBtn = new QPushButton(QStringLiteral("Rotate Tunnel Keys"));
    connect(rotateKeysBtn, &QPushButton::clicked, this, []() {
        auto reply = QMessageBox::question(nullptr, QStringLiteral("Rotate Keys"),
            QStringLiteral("Generate a new key-pair for the tunnel? This will maintain your registration."),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("tunnel"), QStringLiteral("rotate-keys")});
            QMessageBox::information(nullptr, QStringLiteral("Keys Rotated"), QStringLiteral("Tunnel keys have been rotated."));
        }
    });
    diagLayout->addWidget(rotateKeysBtn);

    layout->addWidget(diagGroup);

    // Info label
    m_advancedInfoLabel = new QLabel();
    m_advancedInfoLabel->setWordWrap(true);
    m_advancedInfoLabel->setStyleSheet(QStringLiteral("background: #2a2a2a; padding: 10px; border-radius: 5px; border: 1px solid #3a3a3a;"));
    layout->addWidget(m_advancedInfoLabel);

    layout->addStretch();

    m_contentStack->addWidget(page);
}

void PreferencesDialog::applyStyles() {
    setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background-color: #1e1e1e;"
        "  color: #ffffff;"
        "}"
        "QListWidget {"
        "  background-color: #2a2a2a;"
        "  border: none;"
        "  border-right: 1px solid #3a3a3a;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  padding: 12px 20px;"
        "  border: none;"
        "  color: #ffffff;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #ff6a00;"
        "  color: #ffffff;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #3a3a3a;"
        "}"
        "QLabel {"
        "  color: #ffffff;"
        "  background: transparent;"
        "}"
        "QGroupBox {"
        "  color: #ffffff;"
        "  font-weight: bold;"
        "  border: 1px solid #3a3a3a;"
        "  border-radius: 5px;"
        "  margin-top: 10px;"
        "  padding-top: 10px;"
        "  background-color: #2a2a2a;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 5px;"
        "  color: #ff6a00;"
        "}"
        "QPushButton {"
        "  background-color: #ff6a00;"
        "  color: #ffffff;"
        "  border: none;"
        "  padding: 8px 16px;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #ff8533;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #cc5500;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #3a3a3a;"
        "  color: #666666;"
        "}"
        "QComboBox {"
        "  background-color: #2a2a2a;"
        "  color: #ffffff;"
        "  padding: 5px;"
        "  border: 1px solid #3a3a3a;"
        "  border-radius: 3px;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  background-color: #3a3a3a;"
        "}"
        "QComboBox::down-arrow {"
        "  image: none;"
        "  border-left: 4px solid transparent;"
        "  border-right: 4px solid transparent;"
        "  border-top: 6px solid #ffffff;"
        "  width: 0;"
        "  height: 0;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #2a2a2a;"
        "  color: #ffffff;"
        "  selection-background-color: #ff6a00;"
        "  border: 1px solid #3a3a3a;"
        "}"
        "QTextEdit {"
        "  background-color: #2a2a2a;"
        "  color: #ffffff;"
        "  border: 1px solid #3a3a3a;"
        "  border-radius: 3px;"
        "}"
        "QCheckBox {"
        "  color: #ffffff;"
        "  spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border: 1px solid #3a3a3a;"
        "  border-radius: 3px;"
        "  background-color: #2a2a2a;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: #ff6a00;"
        "  border-color: #ff6a00;"
        "}"
        "QCheckBox::indicator:hover {"
        "  border-color: #ff6a00;"
        "}"
    ));
}

void PreferencesDialog::onCategoryChanged(int index) {
    m_contentStack->setCurrentIndex(index);
    if (index == 1) { // Connection page
        refreshSettings();
    }
}

void PreferencesDialog::onProtocolChanged(int index) {
    QString protocol = m_protocolCombo->itemData(index).toString();
    QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("tunnel"), QStringLiteral("protocol"), protocol});
    QMessageBox::information(this, QStringLiteral("Protocol Changed"),
        QStringLiteral("Tunnel protocol changed. You may need to reconnect for changes to take effect."));
    emit settingsChanged();
}

void PreferencesDialog::onFamiliesModeChanged(int index) {
    QString mode = m_familiesModeCombo->itemData(index).toString();
    QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("dns"), QStringLiteral("families"), mode});
    QMessageBox::information(this, QStringLiteral("Families Mode Changed"),
        QStringLiteral("DNS filtering has been updated."));
    emit settingsChanged();
}

void PreferencesDialog::refreshSettings() {
    // Get settings
    QProcess process;
    process.start(QStringLiteral("warp-cli"), {QStringLiteral("settings")});
    process.waitForFinished();
    m_currentSettings = QString::fromUtf8(process.readAllStandardOutput());

    // Get status for account info
    QProcess statusProcess;
    statusProcess.start(QStringLiteral("warp-cli"), {QStringLiteral("status")});
    statusProcess.waitForFinished();
    QString statusOutput = QString::fromUtf8(statusProcess.readAllStandardOutput());

    loadCurrentSettings(m_currentSettings);
    updateAccountStatus(statusOutput);
}

void PreferencesDialog::loadCurrentSettings(const QString &settingsText) {
    // Parse current mode
    QRegularExpression modeRegex(QStringLiteral("Mode:\\s*([\\w+]+)"));
    auto modeMatch = modeRegex.match(settingsText);
    if (modeMatch.hasMatch()) {
        QString mode = modeMatch.captured(1).toLower();
        int idx = m_modeCombo->findData(mode);
        if (idx >= 0) {
            m_modeCombo->setCurrentIndex(idx);
        }
    }

    // Parse protocol
    if (settingsText.contains(QStringLiteral("MASQUE"), Qt::CaseInsensitive)) {
        m_protocolCombo->setCurrentIndex(0);
        m_connectionInfoLabel->setText(QStringLiteral("Currently using MASQUE (HTTP/3) protocol"));
    } else if (settingsText.contains(QStringLiteral("WireGuard"), Qt::CaseInsensitive)) {
        m_protocolCombo->setCurrentIndex(1);
        m_connectionInfoLabel->setText(QStringLiteral("Currently using WireGuard protocol"));
    }

    // Parse split tunnel exclusions
    QStringList ipExclusions;
    QStringList hostExclusions;

    QRegularExpression excludeRegex(QStringLiteral("Exclude mode, with hosts/ips:([\\s\\S]*?)(?=\\n\\([^)]+\\)|$)"));
    auto excludeMatch = excludeRegex.match(settingsText);
    if (excludeMatch.hasMatch()) {
        QString exclusions = excludeMatch.captured(1);
        QStringList lines = exclusions.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;

            // Check if it's an IP/CIDR or a hostname
            if (trimmed.contains(QLatin1Char('/')) || trimmed.contains(QLatin1Char(':')) ||
                QRegularExpression(QStringLiteral("^\\d+\\.\\d+\\.\\d+\\.\\d+")).match(trimmed).hasMatch()) {
                ipExclusions.append(trimmed);
            } else {
                hostExclusions.append(trimmed);
            }
        }
    }

    // Parse fallback domains (these are also excluded)
    QRegularExpression fallbackRegex(QStringLiteral("Fallback domains:([\\s\\S]*?)(?=\\n\\([^)]+\\)|$)"));
    auto fallbackMatch = fallbackRegex.match(settingsText);
    if (fallbackMatch.hasMatch()) {
        QString domains = fallbackMatch.captured(1);
        QStringList lines = domains.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (!trimmed.isEmpty() && !hostExclusions.contains(trimmed)) {
                hostExclusions.append(trimmed);
            }
        }
    }

    // Update text areas
    if (!ipExclusions.isEmpty()) {
        m_excludedIpsText->setPlainText(ipExclusions.join(QLatin1Char('\n')));
    }
    if (!hostExclusions.isEmpty()) {
        m_excludedHostsText->setPlainText(hostExclusions.join(QLatin1Char('\n')));
    }

    // Update info labels
    m_dnsInfoLabel->setText(QStringLiteral("DNS resolver: ") +
        (settingsText.contains(QStringLiteral("cloudflare-dns.com")) ?
         QStringLiteral("Cloudflare (1.1.1.1)") : QStringLiteral("Default")));

    m_advancedInfoLabel->setText(QStringLiteral("Current configuration loaded from warp-cli settings"));
}

void PreferencesDialog::updateAccountStatus(const QString &statusText) {
    // Get mode from settings for connection type
    QString mode = QStringLiteral("Unknown");
    QRegularExpression modeRegex(QStringLiteral("Mode:\\s*([\\w+]+)"));
    auto modeMatch = modeRegex.match(m_currentSettings);
    if (modeMatch.hasMatch()) {
        mode = modeMatch.captured(1);
    }

    // Update connection mode
    m_statusLabel->setText(mode);

    // Get DNS protocol from tunnel stats
    QProcess tunnelProcess;
    tunnelProcess.start(QStringLiteral("warp-cli"), {QStringLiteral("tunnel"), QStringLiteral("stats")});
    tunnelProcess.waitForFinished();
    QString tunnelOutput = QString::fromUtf8(tunnelProcess.readAllStandardOutput());

    QString dnsProtocol = mode;
    if (tunnelOutput.contains(QStringLiteral("MASQUE"), Qt::CaseInsensitive)) {
        dnsProtocol = QStringLiteral("WARP (MASQUE)");
    } else if (tunnelOutput.contains(QStringLiteral("WireGuard"), Qt::CaseInsensitive)) {
        dnsProtocol = QStringLiteral("WARP (WireGuard)");
    }

    // Get colocation and public IP from Cloudflare trace
    QProcess traceProcess;
    traceProcess.start(QStringLiteral("curl"), {QStringLiteral("-s"), QStringLiteral("https://www.cloudflare.com/cdn-cgi/trace")});
    traceProcess.waitForFinished();
    QString traceOutput = QString::fromUtf8(traceProcess.readAllStandardOutput());

    QString publicIp = QStringLiteral("N/A");
    QString colo = QStringLiteral("N/A");

    QRegularExpression ipRegex(QStringLiteral("ip=([^\\n]+)"));
    auto ipMatch = ipRegex.match(traceOutput);
    if (ipMatch.hasMatch()) {
        publicIp = ipMatch.captured(1).trimmed();
    }

    QRegularExpression coloRegex(QStringLiteral("colo=([^\\n]+)"));
    auto coloMatch = coloRegex.match(traceOutput);
    if (coloMatch.hasMatch()) {
        colo = coloMatch.captured(1).trimmed();
    }

    // Get connection type from network debug
    QProcess networkProcess;
    networkProcess.start(QStringLiteral("warp-cli"), {QStringLiteral("debug"), QStringLiteral("network")});
    networkProcess.waitForFinished();
    QString networkOutput = QString::fromUtf8(networkProcess.readAllStandardOutput());

    QString connectionType = QStringLiteral("Unknown");
    if (networkOutput.contains(QStringLiteral("WiFi:"), Qt::CaseInsensitive)) {
        connectionType = QStringLiteral("Wi-Fi");
    } else if (networkOutput.contains(QStringLiteral("Ethernet"), Qt::CaseInsensitive)) {
        connectionType = QStringLiteral("Ethernet");
    }

    // Get Device ID
    QProcess regProcess;
    regProcess.start(QStringLiteral("warp-cli"), {QStringLiteral("registration"), QStringLiteral("show")});
    regProcess.waitForFinished();
    QString regOutput = QString::fromUtf8(regProcess.readAllStandardOutput());

    QString deviceId = QStringLiteral("N/A");
    QRegularExpression deviceIdRegex(QStringLiteral("Device ID:\\s*([^\\n]+)"));
    auto deviceIdMatch = deviceIdRegex.match(regOutput);
    if (deviceIdMatch.hasMatch()) {
        deviceId = deviceIdMatch.captured(1).trimmed();
    }

    // Update the labels directly
    if (m_dnsProtocolLabel) m_dnsProtocolLabel->setText(dnsProtocol);
    if (m_coloLabel) m_coloLabel->setText(colo);
    if (m_connectionTypeLabel) m_connectionTypeLabel->setText(connectionType);
    if (m_publicIpLabel) m_publicIpLabel->setText(publicIp);
    if (m_deviceIdLabel) m_deviceIdLabel->setText(deviceId);
}
