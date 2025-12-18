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
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent),
      m_sidebar(new QListWidget(this)),
      m_contentStack(new QStackedWidget(this)),
      m_isZeroTrust(false) {

    setWindowTitle(QStringLiteral("WARP Preferences"));
    setMinimumSize(850, 750);

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
    m_sidebar->addItem(QStringLiteral("Account"));
    m_sidebar->addItem(QStringLiteral("Split Tunneling"));
    m_sidebar->addItem(QStringLiteral("DNS & Families"));
    m_sidebar->addItem(QStringLiteral("Advanced"));

    connect(m_sidebar, &QListWidget::currentRowChanged, this, &PreferencesDialog::onCategoryChanged);

    // Content pages
    createGeneralPage();
    createConnectionPage();
    createAccountPage();
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

    layout->addStretch();

    m_contentStack->addWidget(page);
}

void PreferencesDialog::createAccountPage() {
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    // Header
    auto *header = new QLabel(QStringLiteral("Account"));
    QFont headerFont = header->font();
    headerFont.setPointSize(16);
    headerFont.setBold(true);
    header->setFont(headerFont);
    layout->addWidget(header);

    // Account status group
    auto *statusGroup = new QGroupBox(QStringLiteral("Account Status"));
    auto *statusLayout = new QVBoxLayout(statusGroup);

    auto *accountStatusLabel = new QLabel(QStringLiteral("Loading account information..."));
    accountStatusLabel->setWordWrap(true);
    statusLayout->addWidget(accountStatusLabel);

    layout->addWidget(statusGroup);

    // Registration actions group
    auto *actionsGroup = new QGroupBox(QStringLiteral("Registration"));
    auto *actionsLayout = new QVBoxLayout(actionsGroup);

    auto *registerDesc = new QLabel(QStringLiteral("Register this device to start using WARP."));
    registerDesc->setWordWrap(true);
    registerDesc->setStyleSheet(QStringLiteral("color: #999; font-size: 11px; margin-bottom: 5px;"));
    actionsLayout->addWidget(registerDesc);

    auto *registerBtn = new QPushButton(QStringLiteral("Register New Device"));
    registerBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(registerBtn, &QPushButton::clicked, this, [this]() {
        QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("registration"), QStringLiteral("new")});
        QMessageBox::information(this, QStringLiteral("Register"), QStringLiteral("Registration command executed. Check status below."));
        refreshSettings();
        emit settingsChanged();
    });
    actionsLayout->addWidget(registerBtn);

    actionsLayout->addSpacing(10);

    auto *enrollDesc = new QLabel(QStringLiteral("Connect to your organization's Zero Trust network."));
    enrollDesc->setWordWrap(true);
    enrollDesc->setStyleSheet(QStringLiteral("color: #999; font-size: 11px; margin-bottom: 5px;"));
    actionsLayout->addWidget(enrollDesc);

    auto *enrollOrgBtn = new QPushButton(QStringLiteral("Enroll in Zero Trust Organization"));
    enrollOrgBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(enrollOrgBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString org = QInputDialog::getText(this, QStringLiteral("Enroll Organization"),
                                           QStringLiteral("Enter your organization name:"), QLineEdit::Normal,
                                           QString(), &ok);
        if (ok && !org.isEmpty()) {
            // Show ToS dialog BEFORE running the command
            QMessageBox tosDialog(this);
            tosDialog.setWindowTitle(QStringLiteral("Terms of Service"));
            tosDialog.setIcon(QMessageBox::Information);
            tosDialog.setText(QStringLiteral("Your organization is using Cloudflare for Teams"));
            tosDialog.setInformativeText(
                QStringLiteral("The following information may be viewed by administrators:\n\n"
                              "• The websites you visit\n"
                              "• The times you visited them\n\n"
                              "More information:\n"
                              "https://www.cloudflare.com/application/terms/\n"
                              "https://www.cloudflare.com/application/privacypolicy/\n\n"
                              "Do you accept the Terms of Service and Privacy Policy?")
            );

            QPushButton *acceptBtn = tosDialog.addButton(QStringLiteral("Accept"), QMessageBox::AcceptRole);
            QPushButton *declineBtn = tosDialog.addButton(QStringLiteral("Decline"), QMessageBox::RejectRole);

            tosDialog.exec();

            if (tosDialog.clickedButton() != acceptBtn) {
                // User declined
                return;
            }

            // User accepted, now run enrollment with script command to provide PTY and auto-answer
            QProcess *enrollProcess = new QProcess(this);
            QString *allOutput = new QString();
            bool *urlOpened = new bool(false);
            QMetaObject::Connection *readConnection = new QMetaObject::Connection();

            // Read output as it comes
            *readConnection = connect(enrollProcess, &QProcess::readyReadStandardOutput, this, [enrollProcess, allOutput, urlOpened, readConnection, this]() {
                QString newOutput = QString::fromUtf8(enrollProcess->readAllStandardOutput());
                *allOutput += newOutput;

                // Check if URL appeared in the NEW output (not the accumulated output)
                QRegularExpression urlRegex(QStringLiteral("https://[^\\s]+"));
                auto urlMatch = urlRegex.match(newOutput);

                if (urlMatch.hasMatch() && !*urlOpened) {
                    QString url = urlMatch.captured(0);
                    *urlOpened = true;

                    // Disconnect this signal immediately to prevent duplicate triggers
                    disconnect(*readConnection);

                    // NOTE: For enrollment, warp-cli does NOT automatically open browser
                    // when run through script/unbuffer, so we need to open it manually
                    QProcess::startDetached(QStringLiteral("xdg-open"), {url});

                    // Show non-blocking notification
                    QMessageBox *msgBox = new QMessageBox(this);
                    msgBox->setWindowTitle(QStringLiteral("Browser Opened"));
                    msgBox->setIcon(QMessageBox::Information);
                    msgBox->setText(QStringLiteral("Complete authentication in browser"));
                    msgBox->setInformativeText(
                        QStringLiteral("Browser opened to:\n") + url + QStringLiteral("\n\n"
                                      "Complete the authentication to finish enrollment.")
                    );
                    msgBox->setStandardButtons(QMessageBox::Ok);
                    msgBox->setAttribute(Qt::WA_DeleteOnClose);
                    msgBox->setModal(false);
                    msgBox->show();
                }
            });

            connect(enrollProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    this, [this, enrollProcess, allOutput, urlOpened, readConnection, org](int exitCode, QProcess::ExitStatus exitStatus) {
                *allOutput += QString::fromUtf8(enrollProcess->readAllStandardOutput());
                *allOutput += QString::fromUtf8(enrollProcess->readAllStandardError());

                if (exitCode == 0 && *urlOpened) {
                    // URL was opened, authentication should be complete
                    // Don't show "Enrollment Successful" popup yet - wait for polling to confirm
                    
                    // Poll for Zero Trust status to change (check multiple times)
                    int *retryCount = new int(0);
                    QTimer *pollTimer = new QTimer(this);
                    connect(pollTimer, &QTimer::timeout, this, [this, pollTimer, retryCount]() {
                        (*retryCount)++;
                        
                        // Check if Zero Trust status has updated
                        QProcess checkProcess;
                        checkProcess.start(QStringLiteral("warp-cli"), {QStringLiteral("registration"), QStringLiteral("show")});
                        checkProcess.waitForFinished();
                        QString regOutput = QString::fromUtf8(checkProcess.readAllStandardOutput());
                        
                        bool isZeroTrust = regOutput.contains(QStringLiteral("Account type: Team"), Qt::CaseInsensitive) ||
                                          regOutput.contains(QStringLiteral("Organization:"), Qt::CaseInsensitive);
                        
                        // If Zero Trust detected or we've tried 10 times (10 seconds), stop and refresh
                        if (isZeroTrust || *retryCount >= 10) {
                            pollTimer->stop();
                            pollTimer->deleteLater();
                            delete retryCount;
                            
                            refreshSettings();
                            emit settingsChanged();
                        }
                    });
                    pollTimer->start(1000); // Check every second
                } else if (exitCode == 0 && !*urlOpened) {
                    // Successful but no URL? Might be already registered
                    QTimer::singleShot(1000, this, [this]() {
                        refreshSettings();
                        emit settingsChanged();
                    });
                } else if (allOutput->contains(QStringLiteral("Old registration is still around"), Qt::CaseInsensitive)) {
                    // Ask if user wants to delete old registration and retry
                    auto reply = QMessageBox::question(this,
                        QStringLiteral("Existing Registration"),
                        QStringLiteral("An existing registration was found. Delete it and enroll in the new organization?"),
                        QMessageBox::Yes | QMessageBox::No);

                    if (reply == QMessageBox::Yes) {
                        // Delete old registration and retry
                        QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("registration"), QStringLiteral("delete")});

                        // Retry enrollment
                        QProcess *retryProcess = new QProcess(this);
                        connect(retryProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                                this, [this, retryProcess](int code, QProcess::ExitStatus status) {
                            QString output = QString::fromUtf8(retryProcess->readAllStandardOutput()) +
                                           QString::fromUtf8(retryProcess->readAllStandardError());

                            // Check for URL in retry output
                            QRegularExpression urlRegex(QStringLiteral("https://[^\\s]+"));
                            auto urlMatch = urlRegex.match(output);

                            if (urlMatch.hasMatch()) {
                                QString url = urlMatch.captured(0);
                                QProcess::startDetached(QStringLiteral("xdg-open"), {url});

                                QMessageBox::information(this, QStringLiteral("Complete Enrollment in Browser"),
                                                       QStringLiteral("Browser opened. Complete authentication and click OK when done."));
                            } else {
                                QMessageBox::warning(this, QStringLiteral("Enrollment Failed"),
                                                   QStringLiteral("Failed to enroll. Please try again or check warp-cli status."));
                            }
                            
                            // Wait a moment for the registration to fully process, then refresh
                            QTimer::singleShot(1000, this, [this]() {
                                refreshSettings();
                                emit settingsChanged();
                            });
                            
                            retryProcess->deleteLater();
                        });

                        QString retryCmd = QStringLiteral("echo y | script -qec 'warp-cli registration new %1' /dev/null").arg(org);
                        retryProcess->start(QStringLiteral("sh"), {QStringLiteral("-c"), retryCmd});
                    }
                } else {
                    // Show only relevant error lines, not the whole ToS text
                    QStringList lines = allOutput->split(QLatin1Char('\n'));
                    QString errorMsg;
                    for (const QString &line : lines) {
                        QString trimmed = line.trimmed();
                        if (!trimmed.isEmpty() &&
                            !trimmed.startsWith(QStringLiteral("*")) &&
                            !trimmed.contains(QStringLiteral("NOTICE:")) &&
                            !trimmed.contains(QStringLiteral("Your organization is using")) &&
                            !trimmed.contains(QStringLiteral("What information")) &&
                            !trimmed.contains(QStringLiteral("More information")) &&
                            !trimmed.contains(QStringLiteral("https://")) &&
                            !trimmed.contains(QStringLiteral("Accept Terms")) &&
                            trimmed != QStringLiteral("y") &&
                            trimmed.length() > 5) {
                            errorMsg += trimmed + QStringLiteral("\n");
                        }
                    }

                    if (errorMsg.isEmpty()) {
                        errorMsg = QStringLiteral("Enrollment failed. Exit code: ") + QString::number(exitCode);
                    }

                    QMessageBox::warning(this, QStringLiteral("Enrollment Failed"), errorMsg.trimmed());
                }

                delete allOutput;
                delete urlOpened;
                delete readConnection;
                enrollProcess->deleteLater();
            });

            // Use unbuffer (from expect-lite or expect package) or script to keep process running
            // This allows the process to stay alive and receive the callback from the browser
            QString command;

            // Try unbuffer first (cleaner than script)
            if (QProcess::execute(QStringLiteral("which"), {QStringLiteral("unbuffer")}) == 0) {
                command = QStringLiteral("(sleep 0.5; echo y) | unbuffer -p warp-cli registration new %1").arg(org);
            } else {
                // Fallback: use script with a longer timeout to keep process alive
                command = QStringLiteral("(sleep 0.5; echo y; sleep 120) | script -qec 'warp-cli registration new %1' /dev/null").arg(org);
            }

            enrollProcess->start(QStringLiteral("sh"), {QStringLiteral("-c"), command});
        }
    });
    actionsLayout->addWidget(enrollOrgBtn);

    layout->addWidget(actionsGroup);
    
    // Store registration group for show/hide later
    page->setProperty("registrationGroup", QVariant::fromValue(actionsGroup));

    // License group
    auto *licenseGroup = new QGroupBox(QStringLiteral("WARP+ License"));
    auto *licenseLayout = new QVBoxLayout(licenseGroup);

    auto *licenseDesc = new QLabel(QStringLiteral("Enter your WARP+ license key to unlock premium features."));
    licenseDesc->setWordWrap(true);
    licenseDesc->setStyleSheet(QStringLiteral("color: #999; font-size: 11px; margin-bottom: 5px;"));
    licenseLayout->addWidget(licenseDesc);

    auto *attachLicenseBtn = new QPushButton(QStringLiteral("Attach License Key"));
    attachLicenseBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(attachLicenseBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString key = QInputDialog::getText(this, QStringLiteral("Attach License"),
                                           QStringLiteral("Enter your WARP+ license key:"), QLineEdit::Password,
                                           QString(), &ok);
        if (ok && !key.isEmpty()) {
            QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("registration"), QStringLiteral("license"), key});
            QMessageBox::information(this, QStringLiteral("License"), QStringLiteral("License command executed. Check status below."));
            refreshSettings();
            emit settingsChanged();
        }
    });
    licenseLayout->addWidget(attachLicenseBtn);

    layout->addWidget(licenseGroup);

    // Zero Trust / Teams account management group
    auto *teamsGroup = new QGroupBox(QStringLiteral("Zero Trust Account"));
    auto *teamsLayout = new QVBoxLayout(teamsGroup);

    auto *reauthDesc = new QLabel(QStringLiteral("Refresh your authentication with the Zero Trust organization."));
    reauthDesc->setWordWrap(true);
    reauthDesc->setStyleSheet(QStringLiteral("color: #999; font-size: 11px; margin-bottom: 5px;"));
    teamsLayout->addWidget(reauthDesc);

    auto *reauthBtn = new QPushButton(QStringLiteral("Re-Authenticate Session"));
    reauthBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(reauthBtn, &QPushButton::clicked, this, [this]() {
        // First check if WARP is connected
        QProcess checkProcess;
        checkProcess.start(QStringLiteral("warp-cli"), {QStringLiteral("status")});
        checkProcess.waitForFinished();
        QString statusOutput = QString::fromUtf8(checkProcess.readAllStandardOutput());
        
        bool isConnected = statusOutput.contains(QStringLiteral("Status update: Connected"), Qt::CaseInsensitive) ||
                          statusOutput.contains(QStringLiteral("\"status\": \"Connected\""), Qt::CaseInsensitive);
        
        if (!isConnected) {
            // Need to connect first
            auto reply = QMessageBox::question(this,
                QStringLiteral("WARP Not Connected"),
                QStringLiteral("Re-authentication requires WARP to be connected.\n\nConnect now and then re-authenticate?"),
                QMessageBox::Yes | QMessageBox::No);
            
            if (reply != QMessageBox::Yes) {
                return;
            }
            
            // Connect WARP
            QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("connect")});
            
            // Wait a moment for connection
            QMessageBox::information(this, QStringLiteral("Connecting"),
                QStringLiteral("Connecting to WARP...\n\nClick OK to continue with re-authentication."));
        }
        
        // Re-authenticate with Cloudflare Access
        QProcess *reauthProcess = new QProcess(this);
        QString *allOutput = new QString();
        bool *urlOpened = new bool(false);
        QMetaObject::Connection *readConnection = new QMetaObject::Connection();

        // Read output as it comes
        *readConnection = connect(reauthProcess, &QProcess::readyReadStandardOutput, this, [reauthProcess, allOutput, urlOpened, readConnection, this]() {
            QString newOutput = QString::fromUtf8(reauthProcess->readAllStandardOutput());
            *allOutput += newOutput;

            // Check if URL appeared in the NEW output (not the accumulated output)
            QRegularExpression urlRegex(QStringLiteral("https://[^\\s]+"));
            auto urlMatch = urlRegex.match(newOutput);

            if (urlMatch.hasMatch() && !*urlOpened) {
                QString url = urlMatch.captured(0);
                *urlOpened = true;

                // Disconnect this signal immediately to prevent duplicate triggers
                disconnect(*readConnection);

                // NOTE: Don't open browser - warp-cli already opens it automatically
                // Just show notification that browser was opened

                // Show non-blocking notification
                QMessageBox *msgBox = new QMessageBox(this);
                msgBox->setWindowTitle(QStringLiteral("Browser Opened"));
                msgBox->setIcon(QMessageBox::Information);
                msgBox->setText(QStringLiteral("Complete authentication in browser"));
                msgBox->setInformativeText(
                    QStringLiteral("Browser opened to:\n") + url + QStringLiteral("\n\n"
                                  "Complete the authentication to refresh your session.")
                );
                msgBox->setStandardButtons(QMessageBox::Ok);
                msgBox->setAttribute(Qt::WA_DeleteOnClose);
                msgBox->setModal(false);
                msgBox->show();
            }
        });

        connect(reauthProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, reauthProcess, allOutput, urlOpened, readConnection](int exitCode, QProcess::ExitStatus exitStatus) {
            *allOutput += QString::fromUtf8(reauthProcess->readAllStandardOutput());
            *allOutput += QString::fromUtf8(reauthProcess->readAllStandardError());

            if (exitCode == 0) {
                if (*urlOpened) {
                    // URL was opened - user already got the "Browser Opened" notification
                    // Don't show another popup, just refresh silently
                } else {
                    // Command succeeded but no URL (might already be authenticated)
                    QMessageBox::information(this, QStringLiteral("Re-Authentication Complete"),
                                           QStringLiteral("Re-authentication completed successfully."));
                }
                refreshSettings();
                emit settingsChanged();
            } else {
                // Show the actual error message from warp-cli
                QString errorMsg = *allOutput;
                if (errorMsg.isEmpty()) {
                    errorMsg = QStringLiteral("Failed to re-authenticate. Please try logging out and enrolling again.");
                }
                QMessageBox::warning(this, QStringLiteral("Re-Authentication Failed"), errorMsg.trimmed());
            }

            delete allOutput;
            delete urlOpened;
            delete readConnection;
            reauthProcess->deleteLater();
        });

        reauthProcess->start(QStringLiteral("warp-cli"), {QStringLiteral("debug"), QStringLiteral("access-reauth")});
    });
    teamsLayout->addWidget(reauthBtn);

    teamsLayout->addSpacing(10);

    auto *logoutDesc = new QLabel(QStringLiteral("Remove this device from your Zero Trust organization."));
    logoutDesc->setWordWrap(true);
    logoutDesc->setStyleSheet(QStringLiteral("color: #999; font-size: 11px; margin-bottom: 5px;"));
    teamsLayout->addWidget(logoutDesc);

    auto *logoutBtn = new QPushButton(QStringLiteral("Log Out from Organization"));
    logoutBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(logoutBtn, &QPushButton::clicked, this, [this]() {
        auto reply = QMessageBox::question(this,
            QStringLiteral("Log Out from Organization"),
            QStringLiteral("Are you sure you want to log out from your Zero Trust organization?\n\nYour device will remain registered with WARP but will no longer be part of the organization."),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            // To log out from Teams while keeping device registered:
            // 1. Delete current registration (which is enrolled in Teams)
            // 2. Re-register without organization (regular WARP account)
            
            QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("registration"), QStringLiteral("delete")});
            
            // Wait a moment for deletion to complete
            QThread::msleep(500);
            
            // Re-register as a regular WARP device (no organization)
            int exitCode = QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("registration"), QStringLiteral("new")});
            
            if (exitCode == 0) {
                QMessageBox::information(this, QStringLiteral("Logged Out"), 
                    QStringLiteral("Successfully logged out from Zero Trust organization.\n\nYour device is now registered with regular WARP."));
            } else {
                QMessageBox::warning(this, QStringLiteral("Registration Error"), 
                    QStringLiteral("Logged out from organization but failed to re-register.\n\nPlease manually register using 'warp-cli registration new'."));
            }
            
            refreshSettings();
            emit settingsChanged();
        }
    });
    teamsLayout->addWidget(logoutBtn);

    layout->addWidget(teamsGroup);

    // Initially hide Teams group (will be shown if enrolled)
    teamsGroup->setVisible(false);
    page->setProperty("teamsGroup", QVariant::fromValue(teamsGroup));

    layout->addStretch();

    // Store the account status label for updates
    page->setProperty("accountStatusLabel", QVariant::fromValue(accountStatusLabel));

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

    // Network Exclusion (Consumer only)
    m_networkExclusionWidget = new QWidget();
    auto *networkExclusionLayout = new QVBoxLayout(m_networkExclusionWidget);
    networkExclusionLayout->setContentsMargins(0, 0, 0, 0);
    networkExclusionLayout->setSpacing(15);

    auto *excludeGroup = new QGroupBox(QStringLiteral("Excluded Networks"));
    auto *excludeLayout = new QVBoxLayout(excludeGroup);

    auto *excludeDesc = new QLabel(QStringLiteral("WARP will pause when connected to these WiFi networks:"));
    excludeDesc->setWordWrap(true);
    excludeDesc->setStyleSheet(QStringLiteral("color: #999; font-size: 11px;"));
    excludeLayout->addWidget(excludeDesc);

    m_excludedNetworksList = new QListWidget();
    m_excludedNetworksList->setMaximumHeight(150);
    excludeLayout->addWidget(m_excludedNetworksList);

    auto *networkBtnLayout = new QHBoxLayout();
    m_addNetworkBtn = new QPushButton(QStringLiteral("Add Network…"));
    m_removeNetworkBtn = new QPushButton(QStringLiteral("Remove"));
    m_removeNetworkBtn->setEnabled(false);
    networkBtnLayout->addWidget(m_addNetworkBtn);
    networkBtnLayout->addWidget(m_removeNetworkBtn);
    networkBtnLayout->addStretch();
    excludeLayout->addLayout(networkBtnLayout);

    connect(m_addNetworkBtn, &QPushButton::clicked, this, &PreferencesDialog::onAddNetwork);
    connect(m_removeNetworkBtn, &QPushButton::clicked, this, &PreferencesDialog::onRemoveNetwork);
    connect(m_excludedNetworksList, &QListWidget::itemSelectionChanged, this, [this]() {
        m_removeNetworkBtn->setEnabled(m_excludedNetworksList->currentRow() >= 0);
    });

    networkExclusionLayout->addWidget(excludeGroup);

    // Disable WiFi/Ethernet checkboxes
    auto *disableGroup = new QGroupBox(QStringLiteral("Network Type Exclusion"));
    auto *disableLayout = new QVBoxLayout(disableGroup);

    m_disableWifiCheck = new QCheckBox(QStringLiteral("Disable for all WiFi networks"));
    m_disableEthernetCheck = new QCheckBox(QStringLiteral("Disable for all Ethernet connections"));

    connect(m_disableWifiCheck, &QCheckBox::toggled, this, &PreferencesDialog::onDisableWifiChanged);
    connect(m_disableEthernetCheck, &QCheckBox::toggled, this, &PreferencesDialog::onDisableEthernetChanged);

    disableLayout->addWidget(m_disableWifiCheck);
    disableLayout->addWidget(m_disableEthernetCheck);

    networkExclusionLayout->addWidget(disableGroup);

    layout->addWidget(m_networkExclusionWidget);

    // Consumer DNS settings (1.1.1.1 for Families)
    m_consumerDnsWidget = new QWidget();
    auto *consumerDnsLayout = new QVBoxLayout(m_consumerDnsWidget);
    consumerDnsLayout->setContentsMargins(0, 0, 0, 0);

    auto *familiesGroup = new QGroupBox(QStringLiteral("1.1.1.1 for Families"));
    auto *familiesLayout = new QFormLayout(familiesGroup);

    m_familiesModeComboConnection = new QComboBox();
    m_familiesModeComboConnection->addItem(QStringLiteral("Off"), QStringLiteral("off"));
    m_familiesModeComboConnection->addItem(QStringLiteral("Malware Blocking"), QStringLiteral("malware"));
    m_familiesModeComboConnection->addItem(QStringLiteral("Malware and Adult Content"), QStringLiteral("full"));

    connect(m_familiesModeComboConnection, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::onFamiliesModeConnectionChanged);

    familiesLayout->addRow(QStringLiteral("Mode:"), m_familiesModeComboConnection);

    auto *familiesDesc = new QLabel(QStringLiteral("Filter malware and adult content"));
    familiesDesc->setWordWrap(true);
    familiesDesc->setStyleSheet(QStringLiteral("color: #999; font-size: 11px;"));
    familiesLayout->addRow(familiesDesc);

    consumerDnsLayout->addWidget(familiesGroup);

    layout->addWidget(m_consumerDnsWidget);

    // Zero Trust DNS settings (Gateway DoH)
    m_zeroTrustDnsWidget = new QWidget();
    auto *zeroTrustDnsLayout = new QVBoxLayout(m_zeroTrustDnsWidget);
    zeroTrustDnsLayout->setContentsMargins(0, 0, 0, 0);

    auto *gatewayGroup = new QGroupBox(QStringLiteral("Gateway DNS over HTTPS"));
    auto *gatewayLayout = new QFormLayout(gatewayGroup);

    m_gatewayDohInput = new QLineEdit();
    m_gatewayDohInput->setPlaceholderText(QStringLiteral("Enter subdomain (e.g., abc123def)"));
    m_gatewayDohInput->setMaxLength(32);

    connect(m_gatewayDohInput, &QLineEdit::editingFinished, this, &PreferencesDialog::onGatewayDohChanged);

    gatewayLayout->addRow(QStringLiteral("DoH Subdomain:"), m_gatewayDohInput);

    auto *gatewayDesc = new QLabel(
        QStringLiteral("Enter only the subdomain from your Cloudflare Gateway location.\n"
                      "Example: if your endpoint is https://abc123def.cloudflare-gateway.com/dns-query\n"
                      "enter only: abc123def"));
    gatewayDesc->setWordWrap(true);
    gatewayDesc->setStyleSheet(QStringLiteral("color: #999; font-size: 11px;"));
    gatewayLayout->addRow(gatewayDesc);

    zeroTrustDnsLayout->addWidget(gatewayGroup);

    layout->addWidget(m_zeroTrustDnsWidget);

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
    if (index == 0 || index == 1 || index == 2) { // General, Connection, or Account page
        refreshSettings();
    }
}

void PreferencesDialog::onFamiliesModeChanged(int index) {
    QString mode = m_familiesModeCombo->itemData(index).toString();
    QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("dns"), QStringLiteral("families"), mode});
    QMessageBox::information(this, QStringLiteral("Families Mode Changed"),
        QStringLiteral("DNS filtering has been updated."));
    emit settingsChanged();
}

void PreferencesDialog::onFamiliesModeConnectionChanged(int index) {
    QString mode = m_familiesModeComboConnection->itemData(index).toString();
    QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("dns"), QStringLiteral("families"), mode});
    emit settingsChanged();
}

void PreferencesDialog::onAddNetwork() {
    bool ok = false;
    QString networkName = QInputDialog::getText(this,
                                                QStringLiteral("Add Network"),
                                                QStringLiteral("WiFi Network Name (SSID):"),
                                                QLineEdit::Normal,
                                                QString(),
                                                &ok).trimmed();
    if (ok && !networkName.isEmpty()) {
        QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("trusted"), QStringLiteral("ssid"), QStringLiteral("add"), networkName});
        m_excludedNetworksList->addItem(networkName);
        emit settingsChanged();
    }
}

void PreferencesDialog::onRemoveNetwork() {
    QListWidgetItem *item = m_excludedNetworksList->currentItem();
    if (item) {
        QString networkName = item->text();
        QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("trusted"), QStringLiteral("ssid"), QStringLiteral("remove"), networkName});
        delete m_excludedNetworksList->takeItem(m_excludedNetworksList->currentRow());
        emit settingsChanged();
    }
}

void PreferencesDialog::onDisableWifiChanged(bool checked) {
    if (checked) {
        // Checkbox checked = user wants to disable WARP on WiFi
        // So enable the "trusted wifi" feature (auto-disconnect on WiFi)
        QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("trusted"), QStringLiteral("wifi"), QStringLiteral("enable")});
    } else {
        // Checkbox unchecked = user wants WARP to work on WiFi
        // So disable the "trusted wifi" feature
        QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("trusted"), QStringLiteral("wifi"), QStringLiteral("disable")});
    }
    emit settingsChanged();
}

void PreferencesDialog::onDisableEthernetChanged(bool checked) {
    if (checked) {
        // Checkbox checked = user wants to disable WARP on Ethernet
        // So enable the "trusted ethernet" feature (auto-disconnect on Ethernet)
        QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("trusted"), QStringLiteral("ethernet"), QStringLiteral("enable")});
    } else {
        // Checkbox unchecked = user wants WARP to work on Ethernet
        // So disable the "trusted ethernet" feature
        QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("trusted"), QStringLiteral("ethernet"), QStringLiteral("disable")});
    }
    emit settingsChanged();
}

void PreferencesDialog::onGatewayDohChanged() {
    QString subdomain = m_gatewayDohInput->text().trimmed();
    if (!subdomain.isEmpty()) {
        QProcess::execute(QStringLiteral("warp-cli"), {QStringLiteral("dns"), QStringLiteral("gateway-id"), QStringLiteral("set"), subdomain});
        emit settingsChanged();
    }
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

    // Update Account page status
    QString accountStatusText;
    QRegularExpression accountTypeRegex(QStringLiteral("Account type:\\s*([^\\n]+)"));
    auto accountTypeMatch = accountTypeRegex.match(regOutput);
    if (accountTypeMatch.hasMatch()) {
        QString accountType = accountTypeMatch.captured(1).trimmed();
        accountStatusText = QStringLiteral("<b>Account Type:</b> ") + accountType + QStringLiteral("<br>");
    }

    QRegularExpression accountIdRegex(QStringLiteral("Account ID:\\s*([^\\n]+)"));
    auto accountIdMatch = accountIdRegex.match(regOutput);
    if (accountIdMatch.hasMatch()) {
        QString accountId = accountIdMatch.captured(1).trimmed();
        accountStatusText += QStringLiteral("<b>Account ID:</b> ") + accountId + QStringLiteral("<br>");
    }

    accountStatusText += QStringLiteral("<b>Device ID:</b> ") + deviceId;

    // Check if device has WARP+ license
    if (regOutput.contains(QStringLiteral("License:"), Qt::CaseInsensitive)) {
        QRegularExpression licenseRegex(QStringLiteral("License:\\s*([^\\n]+)"));
        auto licenseMatch = licenseRegex.match(regOutput);
        if (licenseMatch.hasMatch()) {
            QString license = licenseMatch.captured(1).trimmed();
            if (license != QStringLiteral("None") && !license.isEmpty()) {
                accountStatusText += QStringLiteral("<br><b>WARP+ License:</b> Active");
            }
        }
    }

    // Check if this is a Teams/Zero Trust account
    bool isTeamsAccount = regOutput.contains(QStringLiteral("Account type: Team"), Qt::CaseInsensitive) ||
                         regOutput.contains(QStringLiteral("Organization:"), Qt::CaseInsensitive);

    // Update the Account page label
    if (m_contentStack->count() > 2) {
        auto *accountPage = m_contentStack->widget(2); // Account page is at index 2
        if (accountPage) {
            auto *accountLabel = accountPage->property("accountStatusLabel").value<QLabel*>();
            if (accountLabel) {
                if (regOutput.contains(QStringLiteral("No registration found"), Qt::CaseInsensitive)) {
                    accountLabel->setText(QStringLiteral("<span style='color:#ff6a00'><b>Not Registered</b></span><br>Please register your device to use WARP."));
                } else {
                    accountLabel->setText(accountStatusText);
                }
            }

            // Show/hide Registration group (show when not registered OR registered but not with a team)
            auto *registrationGroup = accountPage->property("registrationGroup").value<QGroupBox*>();
            bool isRegistered = !regOutput.contains(QStringLiteral("No registration found"), Qt::CaseInsensitive);
            if (registrationGroup) {
                // Show registration options when: not registered OR registered but not in a team
                registrationGroup->setVisible(!isRegistered || !isTeamsAccount);
            }

            // Show/hide Teams account management group (only show when registered with Teams)
            auto *teamsGroup = accountPage->property("teamsGroup").value<QGroupBox*>();
            if (teamsGroup) {
                teamsGroup->setVisible(isTeamsAccount && isRegistered);
            }
        }
    }

    // Update Zero Trust enrollment status and Connection page visibility
    m_isZeroTrust = isZeroTrustEnrolled();
    updateConnectionPageVisibility();
}

bool PreferencesDialog::isZeroTrustEnrolled() {
    QProcess process;
    process.start(QStringLiteral("warp-cli"), {QStringLiteral("registration"), QStringLiteral("show")});
    process.waitForFinished();
    QString output = QString::fromUtf8(process.readAllStandardOutput());

    // Check for Zero Trust indicators
    return output.contains(QStringLiteral("Account type: Team"), Qt::CaseInsensitive) ||
           output.contains(QStringLiteral("Organization:"), Qt::CaseInsensitive);
}

void PreferencesDialog::updateConnectionPageVisibility() {
    // Show/hide widgets based on Zero Trust enrollment
    if (m_networkExclusionWidget) {
        m_networkExclusionWidget->setVisible(!m_isZeroTrust);
    }
    if (m_consumerDnsWidget) {
        m_consumerDnsWidget->setVisible(!m_isZeroTrust);
    }
    if (m_zeroTrustDnsWidget) {
        m_zeroTrustDnsWidget->setVisible(m_isZeroTrust);
    }
}
