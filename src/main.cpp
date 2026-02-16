#include <QApplication>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QDateTime>
#include <QStandardPaths>
#include "editor/mainwindow.h"
#include "editor/projectmanager.h"
#include "editor/splashscreen.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DabozzEditor");
    app.setOrganizationName("Dabozz Studios");
    
    app.setQuitOnLastWindowClosed(true);

    // Session-based suppression
    QString lockFilePath = QDir::tempPath() + "/dabozz_editor_session.lock";
    bool sessionActive = QFile::exists(lockFilePath);
    bool splashSuppressed = sessionActive;
    
    QStringList args = app.arguments();
    if (args.contains("--no-splash")) {
        splashSuppressed = true;
    }
    
    // Create lock file if not existing
    if (!sessionActive) {
        QFile lockFile(lockFilePath);
        if (lockFile.open(QIODevice::WriteOnly)) {
            lockFile.write("session");
            lockFile.close();
        }
    }

    // Cleanup lock file on exit
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [lockFilePath]() {
        QFile::remove(lockFilePath);
    });

    // Detect project path from arguments
    QString projectPath;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg != "--no-splash" && !arg.startsWith("-")) {
            projectPath = arg;
            break;
        }
    }

    // If session is active and no project arg, try auto-open last project
    if (projectPath.isEmpty() && sessionActive) {
        QSettings settings("DabozzStudios", "DabozzEngine");
        int size = settings.beginReadArray("projects");
        QDateTime latestTime;
        QString latestPath;
        
        for (int i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            QDateTime lastOpened = settings.value("lastOpened").toDateTime();
            QString path = settings.value("path").toString();
            if (path.isEmpty()) continue;
            
            if (latestPath.isEmpty() || lastOpened > latestTime) {
                latestTime = lastOpened;
                latestPath = path;
            }
        }
        settings.endArray();
        
        if (!latestPath.isEmpty() && QDir(latestPath).exists()) {
            projectPath = latestPath;
        }
    }
    
    QPixmap splashPixmap("mmm/dabozzstudios.png");
    
    if (!splashPixmap.isNull() && !splashSuppressed) {
        QPixmap finalPixmap(splashPixmap.width(), splashPixmap.height() + 60);
        finalPixmap.fill(Qt::black);
        
        QPainter painter(&finalPixmap);
        painter.drawPixmap(0, 0, splashPixmap);
        
        QFont font("Arial", 16, QFont::Bold);
        painter.setFont(font);
        painter.setPen(Qt::white);
        painter.drawText(QRect(0, splashPixmap.height() + 10, finalPixmap.width(), 50),
                        Qt::AlignCenter, "A Game Engine by Dabozz Studios");
        
        SplashScreen* splash = new SplashScreen(finalPixmap);
        
        if (!projectPath.isEmpty()) {
            // Open MainWindow with project
            MainWindow* window = new MainWindow(projectPath);
            QObject::connect(splash, &SplashScreen::finished, [window, splash]() {
                window->showMaximized();
                splash->deleteLater();
            });
        } else {
            // Open ProjectManager when no project is specified
            ProjectManager* projectManager = new ProjectManager();
            QObject::connect(splash, &SplashScreen::finished, [projectManager, splash]() {
                projectManager->show();
                splash->deleteLater();
            });
        }
        splash->showWithDelay(2500);
        return app.exec();
    } else {
        // Splash suppressed or missing pixmap
        if (!projectPath.isEmpty()) {
            // Open MainWindow with project
            MainWindow* window = new MainWindow(projectPath);
            window->showMaximized();
        } else {
            // Open ProjectManager when no project is specified
            ProjectManager* projectManager = new ProjectManager();
            projectManager->show();
        }
        return app.exec();
    }
}
