#include <QApplication>

#include "tray_app.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    TrayApp tray;
    tray.start();

    return app.exec();
}
