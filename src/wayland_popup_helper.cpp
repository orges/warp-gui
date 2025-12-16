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

void WaylandPopupHelper::setupPopupWindow(QWidget *widget, const QPoint &position, bool anchorBottom) {
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
            // Set up as an overlay popup (like notifications/system tray)
            layerShellWindow->setLayer(LayerShellQt::Window::LayerTop);
            layerShellWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);

            // Get screen dimensions to calculate proper margins
            QScreen *screen = QGuiApplication::screenAt(position);
            if (screen) {
                QRect screenGeom = screen->geometry();

                if (anchorBottom) {
                    // Anchor to bottom-right corner (panel at bottom)
                    layerShellWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorBottom) |
                                                 LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorRight));

                    // Calculate margins from anchored edges (bottom and right)
                    int bottomMargin = screenGeom.bottom() - position.y() - widget->height();
                    int rightMargin = screenGeom.right() - position.x() - widget->width();

                    qDebug() << "Anchoring to BOTTOM-RIGHT with margins - B:" << bottomMargin << "R:" << rightMargin;
                    // Set margins for anchored edges only
                    layerShellWindow->setMargins(QMargins(0, 0, rightMargin, bottomMargin));
                } else {
                    // Anchor to top-right corner (panel at top)
                    layerShellWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop) |
                                                 LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorRight));

                    // Calculate margins from anchored edges (top and right)
                    int topMargin = position.y() - screenGeom.top();
                    int rightMargin = screenGeom.right() - position.x() - widget->width();

                    qDebug() << "Anchoring to TOP-RIGHT with margins - T:" << topMargin << "R:" << rightMargin;
                    // Set margins for anchored edges only
                    layerShellWindow->setMargins(QMargins(0, topMargin, rightMargin, 0));
                }
            }

            // Don't take exclusive space
            layerShellWindow->setExclusiveZone(-1);
        }
    } else {
        // On X11, use standard positioning
        widget->move(position);
        window->setPosition(position);
    }
}
