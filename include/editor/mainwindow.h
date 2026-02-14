#pragma once
#include <QMainWindow>
#include <QDockWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QTimer>
#include <QTabWidget>
#include <QVector3D>
#include <QQuaternion>
#include <map>
#include <QUndoStack>
#include "ecs/world.h"

namespace DabozzEngine {
namespace Systems {
    class PhysicsSystem;
    class AnimationSystem;
    class AudioSystem;
}
namespace Physics {
    class ButsuriEngine;
}
namespace Scripting {
    class ScriptEngine;
}
}

class GameWindow;
class SceneView;
class ComponentInspector;
class HierarchyView;
class AnimatorGraphEditor;
class ScriptEditor;
class AssetBrowser;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const QString& projectPath = QString(), QWidget* parent = nullptr);
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
    void onDeleteSelected();
    void onDuplicateSelected();
    void onAssetDoubleClicked(const QString& filePath);

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
    QTabWidget* m_centralTabs;
    DabozzEngine::ECS::World* m_world;
    DabozzEngine::Physics::ButsuriEngine* m_butsuri;
    DabozzEngine::Systems::PhysicsSystem* m_physicsSystem;
    DabozzEngine::Systems::AnimationSystem* m_animationSystem;
    DabozzEngine::Systems::AudioSystem* m_audioSystem;
    DabozzEngine::Scripting::ScriptEngine* m_scriptEngine;
    QTimer* m_gameLoopTimer;
    
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_helpMenu;
    
    QToolBar* m_mainToolBar;
    QToolBar* m_editToolBar;
    AssetBrowser* m_assetBrowser;

    QUndoStack* m_undoStack;
    QString m_projectPath;
    QString m_currentScenePath;
    bool m_sceneDirty = false;

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

    void applyDarkTheme();
    void initProject();
    void saveSceneState();
    void restoreSceneState();
    void loadProjectScripts();
};