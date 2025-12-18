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
    void setAnchorBottom(bool anchorBottom); // Set whether panel is at bottom (for Wayland)
    void setCurrentPosition(const QPoint &pos); // Set current LayerShell position for drag calculations

signals:
    void requestConnect();
    void requestDisconnect();
    void requestClose();
    void requestSettings();
    void positionChanged(const QPoint &offset); // Emitted when user drags popup

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onToggleChanged(bool checked);

private:
    void applyStyle();
    void updateTitle();
    void updateTitleColor();

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
    bool m_anchorBottom; // Whether panel is at bottom (for Wayland positioning)
    QPoint m_currentPosition; // Current LayerShell position (for drag calculations)

    // For dragging
    bool m_dragging;
    QPoint m_dragStartPos;
    QPoint m_windowStartPos;
};
