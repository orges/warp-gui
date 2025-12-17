#pragma once

#include <QWidget>
#include <QString>

class QLabel;
class QPushButton;
class ToggleSwitch;

class WarpPopup : public QWidget {
    Q_OBJECT

public:
    explicit WarpPopup(QWidget *parent = nullptr);

    void setStatusText(const QString &status, const QString &reason);
    void setBusy(bool busy);
    void setMode(const QString &mode);
    void setZeroTrust(bool isZeroTrust);

signals:
    void requestConnect();
    void requestDisconnect();
    void requestClose();
    void requestSettings();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    void onToggleChanged(bool checked);

private:
    void applyStyle();
    void updateTitle();

    QLabel *m_title;
    ToggleSwitch *m_toggle;
    QLabel *m_status;
    QLabel *m_subtitle;
    QWidget *m_bottomBar;
    QLabel *m_brandingLabel;
    QPushButton *m_settingsBtn;

    bool m_busy;
    QString m_currentMode;
    bool m_isZeroTrust;
};
