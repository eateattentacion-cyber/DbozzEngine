#include <QApplication>
#include "editor/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DabozzEditor");
    app.setOrganizationName("Dabozz Studios");
    
    QString projectPath;
    if (argc > 1) {
        projectPath = QString::fromLocal8Bit(argv[1]);
    }

    MainWindow window(projectPath);
    window.showMaximized();
    
    return app.exec();
}
