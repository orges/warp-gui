#include "wayland_popup_helper.h"

#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <QWindow>
#include <KWindowSystem>
#include <LayerShellQt/Window>

bool WaylandPopupHelper::isWayland() {
    return KWindowSystem::isPlatformWayland();
}

void WaylandPopupHelper::setupPopupWindow(QWidget *widget, const QPoint &position) {
    if (!widget) {
        return;
    }

    QWindow *window = widget->windowHandle();
    if (!window) {
        return;
    }

    if (isWayland()) {
        // Use LayerShellQt for proper Wayland positioning
        auto *layerShellWindow = LayerShellQt::Window::get(window);
        if (layerShellWindow) {
            qDebug() << "Setting up LayerShellQt window at position:" << position;

            // Set up as an overlay popup (like notifications/system tray)
            layerShellWindow->setLayer(LayerShellQt::Window::LayerTop);
            layerShellWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);

            // Anchor to top-right corner
            layerShellWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop) |
                                         LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorRight));

            // Get screen dimensions to calculate proper margins
            QScreen *screen = QGuiApplication::screenAt(position);
            if (screen) {
                QRect screenGeom = screen->geometry();

                // Calculate margins from anchored edges (top and right)
                int topMargin = position.y() - screenGeom.top();
                int rightMargin = screenGeom.right() - position.x() - widget->width();

                qDebug() << "Screen geometry:" << screenGeom;
                qDebug() << "Anchored to top-right with margins - T:" << topMargin << "R:" << rightMargin;

                // Set margins for anchored edges only
                layerShellWindow->setMargins(QMargins(0, topMargin, rightMargin, 0));
            }

            // Don't take exclusive space
            layerShellWindow->setExclusiveZone(-1);
        } else {
            qDebug() << "LayerShellQt::Window::get returned nullptr!";
        }
    } else {
        // On X11, use standard positioning
        widget->move(position);
        window->setPosition(position);
    }
}
