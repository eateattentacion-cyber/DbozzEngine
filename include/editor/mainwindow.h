#pragma once
#include <QMainWindow>
#include <QDockWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QTimer>
#include "ecs/world.h"

namespace DabozzEngine {
namespace Systems {
    class PhysicsSystem;
}
namespace Physics {
    class ButsuriEngine;
}
}

class GameWindow;
class SceneView;
class ComponentInspector;
class HierarchyView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void newScene();
    void openScene();
    void saveScene();
    void saveSceneAs();
    void exitApplication();
    void importMesh();
    void onPlayClicked();
    void onPauseClicked();
    void onStopClicked();
    void updateGameLoop();

public:
    bool isPlayMode() const { return m_editorMode == EditorMode::Play || m_editorMode == EditorMode::Paused; }

private:
    enum class EditorMode {
        Edit,
        Play,
        Paused
    };

    void createMenus();
    void createToolBars();
    void createDockWidgets();
    void createStatusBar();
    void setupLayout();
    void connectViews();
    void createSampleEntities();

    SceneView* m_sceneView;
    ComponentInspector* m_componentInspector;
    HierarchyView* m_hierarchyView;
    GameWindow* m_gameWindow;
    DabozzEngine::ECS::World* m_world;
    DabozzEngine::Physics::ButsuriEngine* m_butsuri;
    DabozzEngine::Systems::PhysicsSystem* m_physicsSystem;
    QTimer* m_gameLoopTimer;
    
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_helpMenu;
    
    QToolBar* m_mainToolBar;
    QToolBar* m_editToolBar;
    
    EditorMode m_editorMode;
};