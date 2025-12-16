#pragma once

#include <QObject>
#include <QPoint>

class QWindow;
class QWidget;

class WaylandPopupHelper : public QObject {
    Q_OBJECT

public:
    static void setupPopupWindow(QWidget *widget, const QPoint &position, bool anchorBottom = false);
    static bool isWayland();
};
