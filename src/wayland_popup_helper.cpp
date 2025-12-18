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
        // Use LayerShellQt for Wayland positioning
        auto *layerShellWindow = LayerShellQt::Window::get(window);
        if (layerShellWindow) {
            // Set up as an overlay popup (like notifications/system tray)
            layerShellWindow->setLayer(LayerShellQt::Window::LayerTop);
            layerShellWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
            layerShellWindow->setExclusiveZone(-1);

            // Get screen dimensions to calculate proper margins
            QScreen *screen = QGuiApplication::screenAt(position);
            if (!screen) {
                screen = QGuiApplication::primaryScreen();
            }

            if (screen) {
                QRect screenGeom = screen->geometry();

                // Anchor to bottom-right for bottom panels, top-right for top panels
                if (anchorBottom) {
                    layerShellWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorBottom) |
                                                 LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorRight));

                    int bottomMargin = screenGeom.bottom() - position.y() - widget->height();
                    int rightMargin = screenGeom.right() - position.x() - widget->width();
                    layerShellWindow->setMargins(QMargins(0, 0, rightMargin, bottomMargin));
                } else {
                    layerShellWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop) |
                                                 LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorRight));

                    int topMargin = position.y() - screenGeom.top();
                    int rightMargin = screenGeom.right() - position.x() - widget->width();
                    layerShellWindow->setMargins(QMargins(0, topMargin, rightMargin, 0));
                }
            }
        }
    } else {
        // On X11, use standard positioning
        widget->move(position);
        window->setPosition(position);
    }
}

void WaylandPopupHelper::updatePopupPosition(QWidget *widget, const QPoint &position, bool anchorBottom) {
    // During drag with LayerShell, we can't move the window smoothly
    // The window will jump to the new position when drag ends (in enableLayerShell)
    // This is a Wayland/LayerShell limitation
    Q_UNUSED(widget);
    Q_UNUSED(position);
    Q_UNUSED(anchorBottom);
}

void WaylandPopupHelper::disableLayerShell(QWidget *widget) {
    // On Wayland with LayerShell, we can't truly "disable" it during drag
    // LayerShell windows can't be converted to normal windows
    // Accept that dragging won't be smooth - position will update when drag ends
    Q_UNUSED(widget);
}

void WaylandPopupHelper::enableLayerShell(QWidget *widget, const QPoint &position, bool anchorBottom) {
    if (!widget || !isWayland()) {
        return;
    }

    QWindow *window = widget->windowHandle();
    if (!window) {
        return;
    }

    auto *layerShellWindow = LayerShellQt::Window::get(window);
    if (!layerShellWindow) {
        return;
    }

    QScreen *screen = QGuiApplication::screenAt(position);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    if (!screen) {
        return;
    }

    QRect screenGeom = screen->geometry();

    if (anchorBottom) {
        layerShellWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorBottom) |
                                     LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorRight));

        int bottomMargin = screenGeom.bottom() - position.y() - widget->height();
        int rightMargin = screenGeom.right() - position.x() - widget->width();
        layerShellWindow->setMargins(QMargins(0, 0, rightMargin, bottomMargin));
    } else {
        layerShellWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop) |
                                     LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorRight));

        int topMargin = position.y() - screenGeom.top();
        int rightMargin = screenGeom.right() - position.x() - widget->width();
        layerShellWindow->setMargins(QMargins(0, topMargin, rightMargin, 0));
    }
}
