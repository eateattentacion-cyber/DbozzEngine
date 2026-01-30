#include <QApplication>
#include "editor/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DabozzEditor");
    app.setOrganizationName("Dabozz Studios");
    
    MainWindow window;
    window.showMaximized();
    
    return app.exec();
}
