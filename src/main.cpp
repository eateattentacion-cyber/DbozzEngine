#include <QApplication>
#include "editor/mainwindow.h"
#include "editor/projectmanager.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DabozzEditor");
    app.setOrganizationName("Dabozz Studios");
    
    if (argc > 1) {
        QString projectPath = QString::fromLocal8Bit(argv[1]);
        MainWindow window(projectPath);
        window.showMaximized();
        return app.exec();
    } else {
        ProjectManager projectManager;
        projectManager.show();
        return app.exec();
    }
}
