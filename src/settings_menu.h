#pragma once

#include <QWidget>
#include <QVBoxLayout>

class QLabel;
class QPushButton;

class SettingsMenu : public QWidget {
    Q_OBJECT

public:
    explicit SettingsMenu(QWidget *parent = nullptr);

    void setActionsEnabled(bool enabled);
    void setCurrentMode(const QString &mode);

signals:
    void modeChangeRequested(const QString &targetMode);
    void preferencesRequested();
    void aboutRequested();
    void exitRequested();

protected:
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPushButton *m_warpBtn;
    QPushButton *m_dnsOnlyBtn;
    QPushButton *m_preferencesBtn;
    QPushButton *m_aboutBtn;
    QPushButton *m_exitBtn;

    QString m_currentMode;
};
