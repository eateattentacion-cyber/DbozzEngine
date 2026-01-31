#pragma once
#include <QMainWindow>
#include <QDockWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QTimer>
#include <QVector3D>
#include <QQuaternion>
#include <map>
#include "ecs/world.h"

namespace DabozzEngine {
namespace Systems {
    class PhysicsSystem;
    class AnimationSystem;
}
namespace Physics {
    class ButsuriEngine;
}
}

class GameWindow;
class SceneView;
class ComponentInspector;
class HierarchyView;
class AnimatorGraphEditor;
class ScriptEditor;

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
    void importAnimation();
    void openScriptEditor();
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
    AnimatorGraphEditor* m_animatorGraphEditor;
    ScriptEditor* m_scriptEditor;
    GameWindow* m_gameWindow;
    DabozzEngine::ECS::World* m_world;
    DabozzEngine::Physics::ButsuriEngine* m_butsuri;
    DabozzEngine::Systems::PhysicsSystem* m_physicsSystem;
    DabozzEngine::Systems::AnimationSystem* m_animationSystem;
    QTimer* m_gameLoopTimer;
    
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_helpMenu;
    
    QToolBar* m_mainToolBar;
    QToolBar* m_editToolBar;
    
    EditorMode m_editorMode;
    DabozzEngine::ECS::EntityID m_selectedEntity = DabozzEngine::ECS::INVALID_ENTITY;

    // Saved state for restoring after play mode
    struct SavedEntityState {
        QVector3D position;
        QQuaternion rotation;
        QVector3D scale;
        QVector3D velocity;
        QVector3D angularVelocity;
        float animatorTime;
        bool animatorPlaying;
    };
    std::map<DabozzEngine::ECS::EntityID, SavedEntityState> m_savedState;

    void saveSceneState();
    void restoreSceneState();
};