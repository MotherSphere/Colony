#include <QApplication>

#include "LauncherWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LauncherWindow window;
    window.show();

    return app.exec();
}
