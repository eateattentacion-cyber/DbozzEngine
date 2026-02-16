#include "editor/mainwindow.h"
#include <QApplication>
#include "editor/assetbrowser.h"
#include "editor/sceneview.h"
#include "editor/componentinspector.h"
#include "editor/hierarchyview.h"
#include "editor/gamewindow.h"
#include "editor/scripteditor.h"
#include "editor/animatorgrapheditor.h"
#include "editor/projectmanager.h"
#include "scripting/scriptengine.h"
#include "scripting/scriptapi.h"
#include "ecs/components/transform.h"
#include "ecs/components/name.h"
#include "ecs/components/mesh.h"
#include "ecs/components/rigidbody.h"
#include "ecs/components/boxcollider.h"
#include "ecs/components/hierarchy.h"
#include "ecs/components/animator.h"
#include "ecs/systems/animationsystem.h"
#include "ecs/systems/audiosystem.h"
#include "renderer/meshloader.h"
#include "renderer/animation.h"
#include "renderer/skeleton.h"
#include "physics/simplephysics.h"
#include "physics/physicssystem.h"
#include "ecs/components/rigidbody.h"
#include "editor/undostack.h"
#include "editor/scenefile.h"
#include "debug/logger.h"
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(const QString& projectPath, QWidget* parent)
    : QMainWindow(parent)
    , m_world(new DabozzEngine::ECS::World())
    , m_gameWindow(nullptr)
    , m_scriptEditor(nullptr)
    , m_centralTabs(nullptr)
    , m_butsuri(nullptr)
    , m_physicsSystem(nullptr)
    , m_animationSystem(nullptr)
    , m_audioSystem(nullptr)
    , m_scriptEngine(nullptr)
    , m_gameLoopTimer(new QTimer(this))
    , m_undoStack(new QUndoStack(this))
    , m_projectPath(projectPath)
    , m_editorMode(EditorMode::Edit)
{
    setWindowTitle("Dabozz Editor");
    resize(1200, 800);

    applyDarkTheme();
    createMenus();
    createToolBars();
    createDockWidgets();
    createStatusBar();
    setupLayout();

    connectViews();

    if (!m_projectPath.isEmpty()) {
        initProject();
    } else {
        createSampleEntities();
    }
    
    // Don't initialize physics at startup - do it when entering play mode(engine might crash.)
    m_butsuri = nullptr;
    m_physicsSystem = nullptr;
    m_animationSystem = new DabozzEngine::Systems::AnimationSystem(m_world);
    m_audioSystem = new DabozzEngine::Systems::AudioSystem(m_world);
    m_audioSystem->initialize();
    
    // Setup game loop timer (60 FPS)
    connect(m_gameLoopTimer, &QTimer::timeout, this, &MainWindow::updateGameLoop);
}

MainWindow::~MainWindow()
{
    if (m_gameWindow) {
        delete m_gameWindow;
    }
    if (m_audioSystem) {
        m_audioSystem->shutdown();
        delete m_audioSystem;
    }
    if (m_animationSystem) {
        delete m_animationSystem;
    }
    if (m_scriptEngine) {
        m_scriptEngine->shutdown();
        delete m_scriptEngine;
    }
    if (m_physicsSystem) {
        delete m_physicsSystem;
    }
    if (m_butsuri) {
        m_butsuri->shutdown();
        delete m_butsuri;
    }
    delete m_world;
}

void MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu("&File");
    m_editMenu = menuBar()->addMenu("&Edit");
    m_viewMenu = menuBar()->addMenu("&View");
    m_helpMenu = menuBar()->addMenu("&Help");
    
    QAction* newAction = m_fileMenu->addAction("&New Scene");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newScene);
    
    QAction* openAction = m_fileMenu->addAction("&Open Scene...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openScene);
    
    QAction* saveAction = m_fileMenu->addAction("&Save Scene");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveScene);
    
    QAction* saveAsAction = m_fileMenu->addAction("Save Scene &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveSceneAs);
    
    m_fileMenu->addSeparator();
    
    QAction* importMeshAction = m_fileMenu->addAction("&Import Mesh...");
    importMeshAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    connect(importMeshAction, &QAction::triggered, this, &MainWindow::importMesh);

    QAction* importAnimAction = m_fileMenu->addAction("Import &Animation...");
    connect(importAnimAction, &QAction::triggered, this, &MainWindow::importAnimation);

    m_fileMenu->addSeparator();
    
    QAction* scriptEditorAction = m_fileMenu->addAction("&Script Editor");
    scriptEditorAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    connect(scriptEditorAction, &QAction::triggered, this, &MainWindow::openScriptEditor);

    m_fileMenu->addSeparator();

    QAction* undoAction = m_undoStack->createUndoAction(this, "&Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    m_editMenu->addAction(undoAction);

    QAction* redoAction = m_undoStack->createRedoAction(this, "&Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    m_editMenu->addAction(redoAction);

    m_editMenu->addSeparator();

    QAction* deleteAction = m_editMenu->addAction("&Delete");
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteSelected);

    QAction* duplicateAction = m_editMenu->addAction("D&uplicate");
    duplicateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    connect(duplicateAction, &QAction::triggered, this, &MainWindow::onDuplicateSelected);

    QAction* openAnimatorAction = m_viewMenu->addAction("&Animator Window");
    connect(openAnimatorAction, &QAction::triggered, this, [this]() {
        m_animatorGraphEditor->show();
        m_animatorGraphEditor->raise();
        m_animatorGraphEditor->activateWindow();
    });

    m_fileMenu->addSeparator();
    
    QAction* exitAction = m_fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::exitApplication);
}

void MainWindow::createToolBars()
{
	m_mainToolBar = addToolBar("Main");
	m_mainToolBar->setMovable(false);
	m_mainToolBar->setIconSize(QSize(16, 16));
	m_mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

	m_editToolBar = addToolBar("Edit");
	m_editToolBar->setMovable(false);
	m_editToolBar->setIconSize(QSize(16, 16));
	m_editToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

	QAction *new_action = m_mainToolBar->addAction(QIcon("editor/icons/File.svg"), "New");
	connect(new_action, &QAction::triggered, this, &MainWindow::newScene);

	QAction *open_action = m_mainToolBar->addAction(QIcon("editor/icons/Load.svg"), "Open");
	connect(open_action, &QAction::triggered, this, &MainWindow::openScene);

	QAction *save_action = m_mainToolBar->addAction(QIcon("editor/icons/Save.svg"), "Save");
	connect(save_action, &QAction::triggered, this, &MainWindow::saveScene);

	m_mainToolBar->addSeparator();

	// Play mode controls with colored styling
	QAction *play_action = m_mainToolBar->addAction(QIcon("editor/icons/Play.svg"), "Play");
	play_action->setToolTip("Enter Play Mode");
	connect(play_action, &QAction::triggered, this, &MainWindow::onPlayClicked);

	QAction *pause_action = m_mainToolBar->addAction(QIcon("editor/icons/Pause.svg"), "Pause");
	pause_action->setToolTip("Pause Play Mode");
	connect(pause_action, &QAction::triggered, this, &MainWindow::onPauseClicked);

	QAction *stop_action = m_mainToolBar->addAction(QIcon("editor/icons/Stop.svg"), "Stop");
	stop_action->setToolTip("Stop and Return to Edit Mode");
	connect(stop_action, &QAction::triggered, this, &MainWindow::onStopClicked);
}

void MainWindow::createDockWidgets()
{
    m_hierarchyView = new HierarchyView(this);
    m_hierarchyView->setWorld(m_world);
    m_hierarchyView->setUndoStack(m_undoStack);
    QDockWidget* hierarchyDock = new QDockWidget("Hierarchy", this);
    hierarchyDock->setWidget(m_hierarchyView);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    m_componentInspector = new ComponentInspector(this);
    m_componentInspector->setWorld(m_world);
    m_componentInspector->setUndoStack(m_undoStack);
    QDockWidget* inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setWidget(m_componentInspector);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    
    // Create tab widget for central area
    m_centralTabs = new QTabWidget(this);
    
    m_sceneView = new SceneView(this);
    m_sceneView->setWorld(m_world);
    m_centralTabs->addTab(m_sceneView, "Scene");
    
    m_scriptEditor = new ScriptEditor(this);
    m_centralTabs->addTab(m_scriptEditor, "Script Editor");
    
    setCentralWidget(m_centralTabs);

    m_assetBrowser = new AssetBrowser(this);
    m_assetBrowser->setRootPath(m_projectPath.isEmpty() ? QDir::currentPath() : m_projectPath);
    QDockWidget* assetDock = new QDockWidget("Assets", this);
    assetDock->setWidget(m_assetBrowser);
    addDockWidget(Qt::BottomDockWidgetArea, assetDock);

    m_animatorGraphEditor = new AnimatorGraphEditor(nullptr);
    m_animatorGraphEditor->setWorld(m_world);
    m_animatorGraphEditor->setWindowTitle("Animator");
    m_animatorGraphEditor->resize(1000, 700);
}

void MainWindow::connectViews()
{
    connect(m_hierarchyView, &HierarchyView::entitySelected, m_componentInspector, &ComponentInspector::setSelectedEntity);
    connect(m_hierarchyView, &HierarchyView::entitySelected, m_sceneView, &SceneView::setSelectedEntity);
    connect(m_sceneView->renderer(), &OpenGLRenderer::selectedEntityTransformChanged, m_componentInspector, &ComponentInspector::refreshSelectedEntity);
    
    // Track selected entity
    connect(m_hierarchyView, &HierarchyView::entitySelected, this, [this](DabozzEngine::ECS::EntityID entity) {
        m_selectedEntity = entity;
    });
    connect(m_hierarchyView, &HierarchyView::entitySelected, m_animatorGraphEditor, &AnimatorGraphEditor::setSelectedEntity);

    // Connect asset browser
    connect(m_assetBrowser, &AssetBrowser::assetDoubleClicked, this, &MainWindow::onAssetDoubleClicked);

    // Connect play mode buttons
    connect(m_sceneView, &SceneView::playClicked, this, &MainWindow::onPlayClicked);
    connect(m_sceneView, &SceneView::pauseClicked, this, &MainWindow::onPauseClicked);
    connect(m_sceneView, &SceneView::stopClicked, this, &MainWindow::onStopClicked);
}

void MainWindow::createSampleEntities()
{
    // Create static floor
    DabozzEngine::ECS::EntityID floor = m_world->createEntity();
    m_world->addComponent<DabozzEngine::ECS::Name>(floor, "Floor");
    
    auto* floorTransform = m_world->addComponent<DabozzEngine::ECS::Transform>(floor);
    floorTransform->position = QVector3D(0, -5, 0);
    floorTransform->rotation = QQuaternion();
    floorTransform->scale = QVector3D(10, 0.5f, 10);
    
    auto* floorMesh = m_world->addComponent<DabozzEngine::ECS::Mesh>(floor);
    // Generate floor cube mesh
    floorMesh->vertices = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f
    };
    floorMesh->normals = {
        0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
        0,0,1, 0,0,1, 0,0,1, 0,0,1
    };
    floorMesh->texCoords = {
        0,0, 1,0, 1,1, 0,1,
        0,0, 1,0, 1,1, 0,1
    };
    floorMesh->indices = {
        0,1,2, 2,3,0,  4,5,6, 6,7,4,
        0,4,7, 7,3,0,  1,5,6, 6,2,1,
        0,1,5, 5,4,0,  3,2,6, 6,7,3
    };
    
    auto* floorRb = m_world->addComponent<DabozzEngine::ECS::RigidBody>(floor, 0.0f, true, false);
    m_world->addComponent<DabozzEngine::ECS::BoxCollider>(floor, QVector3D(10, 0.5f, 10));
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage("Ready");
}

void MainWindow::setupLayout()
{
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks);
}

void MainWindow::newScene()
{
    if (m_sceneDirty) {
        auto result = QMessageBox::question(this, "Unsaved Changes",
            "You have unsaved changes. Do you want to save before creating a new scene?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (result == QMessageBox::Cancel) return;
        if (result == QMessageBox::Save) saveScene();
    }

    auto entities = m_world->getEntities();
    for (auto it = entities.rbegin(); it != entities.rend(); ++it) {
        m_world->destroyEntity(*it);
    }

    m_undoStack->clear();
    m_currentScenePath.clear();
    m_sceneDirty = false;
    m_selectedEntity = DabozzEngine::ECS::INVALID_ENTITY;

    m_hierarchyView->refreshHierarchy();
    m_componentInspector->clearSelection();
    m_sceneView->renderer()->update();

    setWindowTitle("Dabozz Editor - New Scene");
    statusBar()->showMessage("New scene created");
}

void MainWindow::openScene()
{
    if (m_sceneDirty) {
        auto result = QMessageBox::question(this, "Unsaved Changes",
            "You have unsaved changes. Do you want to save first?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (result == QMessageBox::Cancel) return;
        if (result == QMessageBox::Save) saveScene();
    }

    QString defaultDir = m_projectPath.isEmpty() ? "" : m_projectPath + "/Scenes";
    QString fileName = QFileDialog::getOpenFileName(this, "Open Scene", defaultDir, "DabozzEngine Scene (*.dabozz);;All Files (*.*)");
    if (fileName.isEmpty()) return;

    if (SceneFile::loadScene(m_world, fileName)) {
        m_currentScenePath = fileName;
        m_sceneDirty = false;
        m_undoStack->clear();
        m_selectedEntity = DabozzEngine::ECS::INVALID_ENTITY;

        m_hierarchyView->refreshHierarchy();
        m_componentInspector->clearSelection();
        m_sceneView->renderer()->update();

        QFileInfo fi(fileName);
        setWindowTitle(QString("Dabozz Editor - %1").arg(fi.fileName()));
        statusBar()->showMessage(QString("Opened scene: %1").arg(fileName));
    } else {
        QMessageBox::critical(this, "Open Failed", "Failed to load scene file.");
    }
}

void MainWindow::saveScene()
{
    if (m_currentScenePath.isEmpty()) {
        saveSceneAs();
        return;
    }

    if (SceneFile::saveScene(m_world, m_currentScenePath)) {
        m_sceneDirty = false;
        statusBar()->showMessage(QString("Scene saved: %1").arg(m_currentScenePath));
    } else {
        QMessageBox::critical(this, "Save Failed", "Failed to save scene file.");
    }
}

void MainWindow::saveSceneAs()
{
    QString defaultDir = m_projectPath.isEmpty() ? "" : m_projectPath + "/Scenes";
    QString fileName = QFileDialog::getSaveFileName(this, "Save Scene As", defaultDir, "DabozzEngine Scene (*.dabozz);;All Files (*.*)");
    if (fileName.isEmpty()) return;

    if (!fileName.endsWith(".dabozz")) {
        fileName += ".dabozz";
    }

    if (SceneFile::saveScene(m_world, fileName)) {
        m_currentScenePath = fileName;
        m_sceneDirty = false;
        QFileInfo fi(fileName);
        setWindowTitle(QString("Dabozz Editor - %1").arg(fi.fileName()));
        statusBar()->showMessage(QString("Saved scene as: %1").arg(fileName));
    } else {
        QMessageBox::critical(this, "Save Failed", "Failed to save scene file.");
    }
}

void MainWindow::exitApplication()
{
    close();
}

void MainWindow::importMesh()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Import Mesh", "", 
        "3D Models (*.obj *.fbx *.gltf *.glb *.dae *.blend *.3ds *.ply *.stl);;All Files (*.*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // Pre-load skeleton to get bone IDs before loading meshes
    auto skeleton = std::make_shared<DabozzEngine::Renderer::Skeleton>();
    skeleton->loadFromFile(fileName.toStdString());
    DEBUG_LOG << "Skeleton loaded with " << skeleton->getBoneCount() << " bones before mesh loading" << std::endl;
    
    auto meshes = DabozzEngine::Renderer::MeshLoader::LoadMesh(fileName.toStdString(), skeleton.get());

    if (!meshes.empty()) {
        QFileInfo fileInfo(fileName);
        
        if (meshes.size() == 1) {
            // Single mesh - create entity directly without parent
            DabozzEngine::ECS::EntityID entity = m_world->createEntity();
            
            auto* name = m_world->addComponent<DabozzEngine::ECS::Name>(entity);
            name->name = fileInfo.baseName();

            m_world->addComponent<DabozzEngine::ECS::Transform>(entity);
            m_world->addComponent<DabozzEngine::ECS::Hierarchy>(entity);
            
            auto* meshComponent = m_world->addComponent<DabozzEngine::ECS::Mesh>(entity);
            *meshComponent = meshes[0];
            
            // If mesh has animation, add Animator component and load animation
            if (meshes[0].hasAnimation) {
                DEBUG_LOG << "=== SETTING UP ANIMATION ===" << std::endl;
                auto* animator = m_world->addComponent<DabozzEngine::ECS::Animator>(entity);
                animator->skeleton = skeleton;

                auto animation = std::make_shared<DabozzEngine::Renderer::Animation>(fileName, skeleton.get());
                QString clipName = fileInfo.baseName();
                animator->addAnimation(clipName, animation);
                animator->loop = true;
                animator->play();
                DEBUG_LOG << "Animation '" << clipName.toStdString() << "' loaded and playing" << std::endl;
            }
        } else {
            // Multiple meshes - create parent entity with children
            DabozzEngine::ECS::EntityID parentEntity = m_world->createEntity();
            
            auto* parentName = m_world->addComponent<DabozzEngine::ECS::Name>(parentEntity);
            parentName->name = fileInfo.baseName();
            
            m_world->addComponent<DabozzEngine::ECS::Transform>(parentEntity);
            auto* parentHierarchy = m_world->addComponent<DabozzEngine::ECS::Hierarchy>(parentEntity);
            
            bool hasAnimation = false;
            
            // Create child entities for each mesh part
            for (size_t i = 0; i < meshes.size(); ++i) {
                DabozzEngine::ECS::EntityID childEntity = m_world->createEntity();
                
                auto* childName = m_world->addComponent<DabozzEngine::ECS::Name>(childEntity);
                childName->name = QString("%1_part%2").arg(fileInfo.baseName()).arg(i + 1);
                
                m_world->addComponent<DabozzEngine::ECS::Transform>(childEntity);
                
                auto* childHierarchy = m_world->addComponent<DabozzEngine::ECS::Hierarchy>(childEntity);
                childHierarchy->parent = parentEntity;
                
                parentHierarchy->children.push_back(childEntity);
                
                auto* meshComponent = m_world->addComponent<DabozzEngine::ECS::Mesh>(childEntity);
                *meshComponent = meshes[i];
                
                if (meshes[i].hasAnimation) {
                    hasAnimation = true;
                }
            }
            
            // If any mesh has animation, add Animator to parent
            if (hasAnimation) {
                DEBUG_LOG << "=== SETTING UP ANIMATION (MULTI-MESH) ===" << std::endl;
                auto* animator = m_world->addComponent<DabozzEngine::ECS::Animator>(parentEntity);
                animator->skeleton = skeleton;

                auto animation = std::make_shared<DabozzEngine::Renderer::Animation>(fileName, skeleton.get());
                QString clipName = fileInfo.baseName();
                animator->addAnimation(clipName, animation);
                animator->loop = true;
                animator->play();
                DEBUG_LOG << "Animation '" << clipName.toStdString() << "' loaded and playing" << std::endl;
            }
        }

        statusBar()->showMessage(QString("Imported mesh: %1").arg(fileName));
        m_hierarchyView->refreshHierarchy();
        QMessageBox::information(this, "Import Successful", 
            QString("Successfully imported %1 mesh(es) from %2.")
            .arg(meshes.size())
            .arg(fileInfo.fileName()));
    } else {
        QMessageBox::critical(this, "Import Failed", "Failed to load mesh file or file contains no meshes.");
    }
}

void MainWindow::importAnimation()
{
    if (!m_world || m_selectedEntity == DabozzEngine::ECS::INVALID_ENTITY) {
        QMessageBox::warning(this, "Import Animation", "Please select an entity with an Animator component first.");
        return;
    }

    // Find the animator - check selected entity and its parent
    DabozzEngine::ECS::Animator* animator = m_world->getComponent<DabozzEngine::ECS::Animator>(m_selectedEntity);
    DabozzEngine::ECS::EntityID animatorEntity = m_selectedEntity;

    if (!animator) {
        auto* hierarchy = m_world->getComponent<DabozzEngine::ECS::Hierarchy>(m_selectedEntity);
        if (hierarchy && hierarchy->parent != 0) {
            animator = m_world->getComponent<DabozzEngine::ECS::Animator>(hierarchy->parent);
            animatorEntity = hierarchy->parent;
        }
    }

    if (!animator || !animator->skeleton) {
        QMessageBox::warning(this, "Import Animation", "Selected entity has no Animator with a skeleton. Import a model with animation first.");
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "Import Animation",  "",
        "Animation Files (*.fbx *.gltf *.glb *.dae);;All Files (*.*)");

    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    QString clipName = fileInfo.baseName();

    auto animation = std::make_shared<DabozzEngine::Renderer::Animation>(fileName, animator->skeleton.get());
    animator->addAnimation(clipName, animation);

    // Auto-add state to graph if one exists
    if (animator->graph) {
        float y = animator->graph->states.size() * 80.0f;
        animator->graph->addState(clipName, clipName, QPointF(0, y));
        m_animatorGraphEditor->setSelectedEntity(animatorEntity);
    }

    statusBar()->showMessage(QString("Imported animation clip: %1").arg(clipName));
    m_componentInspector->updateUI();

    QMessageBox::information(this, "Import Successful",
        QString("Animation clip '%1' added. Total clips: %2")
        .arg(clipName)
        .arg(animator->animations.size()));
}

void MainWindow::onPlayClicked()
{
    DEBUG_LOG << "=== PLAY BUTTON CLICKED ===" << std::endl;
    
    if (m_editorMode == EditorMode::Edit) {
        DEBUG_LOG << "Entering play mode from edit mode" << std::endl;
        m_editorMode = EditorMode::Play;
        statusBar()->showMessage("Play Mode");
        m_sceneView->setModeLabel("Scene View - Play Mode");

        saveSceneState();

        if (!m_butsuri) {
            DEBUG_LOG << "Initializing Butsuri Engine" << std::endl;
            m_butsuri = new DabozzEngine::Physics::ButsuriEngine();
            m_butsuri->initialize();
            m_physicsSystem = new DabozzEngine::Systems::PhysicsSystem(m_world);
            m_physicsSystem->initialize();
            DEBUG_LOG << "Butsuri Engine initialized" << std::endl;
        }
        
        if (!m_scriptEngine) {
            DEBUG_LOG << "Initializing Script Engine" << std::endl;
            m_scriptEngine = new DabozzEngine::Scripting::ScriptEngine();
            m_scriptEngine->initialize(m_world);
            loadProjectScripts();
            m_scriptEngine->callLuaStart();
            m_scriptEngine->callAngelScriptStart();
            DEBUG_LOG << "Script Engine initialized" << std::endl;
        }
        
        m_sceneView->renderer()->setPlayMode(true);

        DEBUG_LOG << "Disabling gizmo" << std::endl;
        m_sceneView->renderer()->setSelectedEntity(DabozzEngine::ECS::INVALID_ENTITY);
        
        DEBUG_LOG << "Resetting mesh upload status for new OpenGL context" << std::endl;
        for (DabozzEngine::ECS::EntityID entity : m_world->getEntities()) {
            DabozzEngine::ECS::Mesh* mesh = m_world->getComponent<DabozzEngine::ECS::Mesh>(entity);
            if (mesh) {
                mesh->isUploaded = false;
                mesh->vao = 0;
                mesh->vbo = 0;
                mesh->ebo = 0;
                mesh->textureID = 0;
            }
        }
        
        DEBUG_LOG << "Creating game window" << std::endl;
        if (!m_gameWindow) {
            m_gameWindow = new GameWindow(m_world);
            DEBUG_LOG << "Game window created" << std::endl;
        }
        DEBUG_LOG << "Showing game window" << std::endl;
        m_gameWindow->show();
        m_gameWindow->raise();
        m_gameWindow->activateWindow();
        DEBUG_LOG << "Game window shown" << std::endl;
        
        DEBUG_LOG << "Starting game loop timer" << std::endl;
        m_gameLoopTimer->start(16);
        DEBUG_LOG << "Play mode setup complete" << std::endl;
        
    } else if (m_editorMode == EditorMode::Paused) {
        DEBUG_LOG << "Resuming from pause" << std::endl;
        m_editorMode = EditorMode::Play;
        statusBar()->showMessage("Play Mode (Resumed)");
        m_sceneView->setModeLabel("Scene View - Play Mode");
        m_gameLoopTimer->start(16);
    }
}

void MainWindow::onPauseClicked()
{
    if (m_editorMode == EditorMode::Play) {
        m_editorMode = EditorMode::Paused;
        statusBar()->showMessage("Play Mode (Paused)");
        m_sceneView->setModeLabel("Scene View - Paused");
        m_gameLoopTimer->stop();
    }
}

void MainWindow::onStopClicked()
{
    if (m_editorMode == EditorMode::Play || m_editorMode == EditorMode::Paused) {
        m_editorMode = EditorMode::Edit;
        statusBar()->showMessage("Edit Mode");
        m_sceneView->setModeLabel("Scene View - Edit Mode");

        // Stop game loop
        m_gameLoopTimer->stop();
        
        // Hide game window
        if (m_gameWindow) {
            m_gameWindow->hide();
        }

        // Restore scene state to pre-play values
        restoreSceneState();

        // Resume the editor view
        m_sceneView->renderer()->setPlayMode(false);

        m_hierarchyView->refreshHierarchy();
        m_sceneView->renderer()->update();
    }
}

void MainWindow::updateGameLoop()
{
    if (m_editorMode == EditorMode::Play) {
        float deltaTime = 1.0f / 60.0f;
        
        if (m_scriptEngine) {
            DabozzEngine::Scripting::ScriptAPI::SetDeltaTime(deltaTime);
            m_scriptEngine->callLuaUpdate(deltaTime);
            m_scriptEngine->callAngelScriptUpdate(deltaTime);
        }
        
        if (m_audioSystem) {
            // Update listener position to match camera
            // Find camera entity (you can tag it or use a specific name)
            for (auto entity : m_world->getEntities()) {
                auto* name = m_world->getComponent<DabozzEngine::ECS::Name>(entity);
                if (name && name->name == "Camera") {
                    auto* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(entity);
                    if (transform) {
                        m_audioSystem->setListenerPosition(transform->position);
                        // Calculate forward direction from rotation
                        QVector3D forward = transform->rotation.rotatedVector(QVector3D(0, 0, -1));
                        QVector3D up = transform->rotation.rotatedVector(QVector3D(0, 1, 0));
                        m_audioSystem->setListenerOrientation(forward, up);
                    }
                    break;
                }
            }
            m_audioSystem->update(deltaTime);
        }
        if (m_animationSystem) {
            m_animationSystem->update(deltaTime);
        }
        
        if (m_physicsSystem) {
            m_physicsSystem->update(deltaTime);
        }
        
        if (m_gameWindow) {
            m_gameWindow->renderer()->update();
        }
    }
}

void MainWindow::saveSceneState()
{
    m_savedState.clear();
    for (DabozzEngine::ECS::EntityID entity : m_world->getEntities()) {
        SavedEntityState state;

        auto* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(entity);
        if (transform) {
            state.position = transform->position;
            state.rotation = transform->rotation;
            state.scale = transform->scale;
        }

        auto* rb = m_world->getComponent<DabozzEngine::ECS::RigidBody>(entity);
        if (rb) {
            state.velocity = rb->velocity;
            state.angularVelocity = rb->angularVelocity;
        }

        auto* animator = m_world->getComponent<DabozzEngine::ECS::Animator>(entity);
        if (animator) {
            state.animatorTime = animator->currentTime;
            state.animatorPlaying = animator->isPlaying;
        }

        m_savedState[entity] = state;
    }
}

void MainWindow::restoreSceneState()
{
    for (auto& [entity, state] : m_savedState) {
        auto* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(entity);
        if (transform) {
            transform->position = state.position;
            transform->rotation = state.rotation;
            transform->scale = state.scale;
        }

        auto* rb = m_world->getComponent<DabozzEngine::ECS::RigidBody>(entity);
        if (rb) {
            rb->velocity = state.velocity;
            rb->angularVelocity = state.angularVelocity;
        }

        auto* animator = m_world->getComponent<DabozzEngine::ECS::Animator>(entity);
        if (animator) {
            animator->currentTime = state.animatorTime;
            animator->isPlaying = state.animatorPlaying;
        }

        // Reset mesh GPU state so the editor re-uploads for its own GL context
        auto* mesh = m_world->getComponent<DabozzEngine::ECS::Mesh>(entity);
        if (mesh) {
            mesh->isUploaded = false;
            mesh->vao = 0;
            mesh->vbo = 0;
            mesh->ebo = 0;
            mesh->textureID = 0;
        }
    }
    m_savedState.clear();
}

void MainWindow::openScriptEditor()
{
    if (m_centralTabs) {
        for (int i = 0; i < m_centralTabs->count(); ++i) {
            if (m_centralTabs->tabText(i) == "Script Editor") {
                m_centralTabs->setCurrentIndex(i);
                break;
            }
        }
    }
}

void MainWindow::onDeleteSelected()
{
    if (m_selectedEntity == DabozzEngine::ECS::INVALID_ENTITY) return;

    m_undoStack->push(new DeleteEntityCommand(m_world, m_selectedEntity, [this]() {
        m_hierarchyView->refreshHierarchy();
        m_componentInspector->clearSelection();
    }));

    m_selectedEntity = DabozzEngine::ECS::INVALID_ENTITY;
    m_sceneDirty = true;
}

void MainWindow::onDuplicateSelected()
{
    if (m_selectedEntity == DabozzEngine::ECS::INVALID_ENTITY || !m_world) return;
    m_hierarchyView->duplicateSelectedEntity();
    m_sceneDirty = true;
}

void MainWindow::initProject()
{
    QDir projectDir(m_projectPath);

    // Create standard folders if they don't exist
    projectDir.mkpath("Scenes");
    projectDir.mkpath("Assets");
    projectDir.mkpath("Scripts");

    // Set window title to project name
    QString projectName = projectDir.dirName();
    setWindowTitle(QString("Dabozz Editor - %1").arg(projectName));

    // Load main scene if it exists
    QString mainScene = m_projectPath + "/Scenes/main.dabozz";
    if (QFile::exists(mainScene)) {
        if (SceneFile::loadScene(m_world, mainScene)) {
            m_currentScenePath = mainScene;
            m_hierarchyView->refreshHierarchy();
            m_sceneView->renderer()->update();
            statusBar()->showMessage(QString("Loaded project: %1").arg(projectName));
        }
    } else {
        // No main scene - create sample entities
        createSampleEntities();
        statusBar()->showMessage(QString("Opened project: %1 (new)").arg(projectName));
    }
}

void MainWindow::onAssetDoubleClicked(const QString& filePath)
{
    QFileInfo info(filePath);
    QString ext = info.suffix().toLower();

    if (ext == "dabozz") {
        // Load as scene
        if (m_sceneDirty) {
            auto result = QMessageBox::question(this, "Unsaved Changes",
                "You have unsaved changes. Save first?",
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            if (result == QMessageBox::Cancel) return;
            if (result == QMessageBox::Save) saveScene();
        }

        if (SceneFile::loadScene(m_world, filePath)) {
            m_currentScenePath = filePath;
            m_sceneDirty = false;
            m_undoStack->clear();
            m_selectedEntity = DabozzEngine::ECS::INVALID_ENTITY;
            m_hierarchyView->refreshHierarchy();
            m_componentInspector->clearSelection();
            m_sceneView->renderer()->update();
            setWindowTitle(QString("Dabozz Editor - %1").arg(info.fileName()));
            statusBar()->showMessage(QString("Opened scene: %1").arg(filePath));
        }
    } else if (ext == "obj" || ext == "fbx" || ext == "gltf" || ext == "glb" || ext == "dae") {
        // Import as mesh - reuse existing importMesh logic but with a specific file
        auto skeleton = std::make_shared<DabozzEngine::Renderer::Skeleton>();
        skeleton->loadFromFile(filePath.toStdString());

        auto meshes = DabozzEngine::Renderer::MeshLoader::LoadMesh(filePath.toStdString(), skeleton.get());
        if (!meshes.empty()) {
            DabozzEngine::ECS::EntityID entity = m_world->createEntity();
            m_world->addComponent<DabozzEngine::ECS::Name>(entity, info.baseName());
            m_world->addComponent<DabozzEngine::ECS::Transform>(entity);
            m_world->addComponent<DabozzEngine::ECS::Hierarchy>(entity);

            auto* meshComp = m_world->addComponent<DabozzEngine::ECS::Mesh>(entity);
            *meshComp = meshes[0];

            m_hierarchyView->refreshHierarchy();
            m_sceneDirty = true;
            statusBar()->showMessage(QString("Imported: %1").arg(info.fileName()));
        }
    } else if (ext == "cs") {
        // Open in script editor
        if (m_centralTabs) {
            for (int i = 0; i < m_centralTabs->count(); ++i) {
                if (m_centralTabs->tabText(i) == "Script Editor") {
                    m_centralTabs->setCurrentIndex(i);
                    break;
                }
            }
        }
        statusBar()->showMessage(QString("Opened script: %1").arg(info.fileName()));
    }
}

void MainWindow::applyDarkTheme()
{
    const QString style = R"(
        /* === Global === */
        QMainWindow, QWidget {
            background-color: #1a1a1a;
            color: #d4d4d4;
            font-family: "Segoe UI", Arial, sans-serif;
            font-size: 12px;
        }

        /* === Menu Bar === */
        QMenuBar {
            background-color: #252525;
            color: #d4d4d4;
            border-bottom: 1px solid #111;
            padding: 2px;
        }
        QMenuBar::item {
            padding: 5px 10px;
            background: transparent;
            border-radius: 3px;
        }
        QMenuBar::item:selected {
            background-color: #3a3a3a;
        }
        QMenu {
            background-color: #252525;
            color: #d4d4d4;
            border: 1px solid #111;
            padding: 4px;
        }
        QMenu::item {
            padding: 6px 24px 6px 16px;
            border-radius: 3px;
        }
        QMenu::item:selected {
            background-color: #2563eb;
        }
        QMenu::separator {
            height: 1px;
            background: #3a3a3a;
            margin: 4px 8px;
        }

        /* === Toolbars === */
        QToolBar {
            background-color: #252525;
            border: none;
            border-bottom: 1px solid #111;
            padding: 4px;
            spacing: 4px;
        }
        QToolBar::separator {
            width: 1px;
            background: #3a3a3a;
            margin: 4px 6px;
        }
        QToolButton {
            background-color: transparent;
            color: #d4d4d4;
            border: 1px solid transparent;
            border-radius: 4px;
            padding: 6px 10px;
            font-weight: bold;
        }
        QToolButton:hover {
            background-color: #3a3a3a;
            border: 1px solid #4a4a4a;
        }
        QToolButton:pressed {
            background-color: #2563eb;
        }

        /* === Dock Widgets === */
        QDockWidget {
            color: #d4d4d4;
            titlebar-close-icon: none;
            titlebar-normal-icon: none;
        }
        QDockWidget::title {
            background-color: #252525;
            padding: 8px;
            border-bottom: 2px solid #2563eb;
            font-weight: bold;
        }
        QDockWidget::close-button, QDockWidget::float-button {
            background: transparent;
            border: none;
            padding: 2px;
        }
        QDockWidget::close-button:hover, QDockWidget::float-button:hover {
            background: #3a3a3a;
        }

        /* === Tab Widget === */
        QTabWidget::pane {
            border: 1px solid #333;
            background-color: #1a1a1a;
        }
        QTabBar::tab {
            background-color: #252525;
            color: #999;
            padding: 8px 20px;
            border: none;
            border-bottom: 2px solid transparent;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            color: #fff;
            border-bottom: 2px solid #2563eb;
            background-color: #1a1a1a;
        }
        QTabBar::tab:hover:!selected {
            color: #d4d4d4;
            background-color: #2a2a2a;
        }

        /* === Tree Widget (Hierarchy) === */
        QTreeWidget, QTreeView {
            background-color: #1e1e1e;
            alternate-background-color: #222;
            color: #d4d4d4;
            border: 1px solid #333;
            outline: none;
        }
        QTreeWidget::item {
            padding: 4px 2px;
            border: none;
        }
        QTreeWidget::item:selected {
            background-color: #2563eb;
            color: white;
        }
        QTreeWidget::item:hover:!selected {
            background-color: #2a2a2a;
        }
        QTreeWidget::branch {
            background-color: #1e1e1e;
        }
        QTreeWidget::branch:selected {
            background-color: #2563eb;
        }
        QHeaderView::section {
            background-color: #252525;
            color: #999;
            padding: 6px;
            border: none;
            border-bottom: 1px solid #333;
            font-weight: bold;
        }

        /* === Group Boxes (Inspector) === */
        QGroupBox {
            background-color: #222;
            border: 1px solid #333;
            border-radius: 4px;
            margin-top: 12px;
            padding-top: 20px;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 4px 8px;
            color: #a0c4ff;
        }

        /* === Line Edits === */
        QLineEdit {
            background-color: #2a2a2a;
            color: #d4d4d4;
            border: 1px solid #3a3a3a;
            border-radius: 3px;
            padding: 4px 6px;
            selection-background-color: #2563eb;
        }
        QLineEdit:focus {
            border: 1px solid #2563eb;
        }
        QLineEdit:disabled {
            background-color: #1e1e1e;
            color: #666;
        }

        /* === Buttons === */
        QPushButton {
            background-color: #333;
            color: #d4d4d4;
            border: 1px solid #444;
            border-radius: 4px;
            padding: 6px 14px;
        }
        QPushButton:hover {
            background-color: #3a3a3a;
            border: 1px solid #555;
        }
        QPushButton:pressed {
            background-color: #2563eb;
            border: 1px solid #2563eb;
            color: white;
        }
        QPushButton:disabled {
            background-color: #252525;
            color: #555;
            border: 1px solid #333;
        }

        /* === Combo Boxes === */
        QComboBox {
            background-color: #2a2a2a;
            color: #d4d4d4;
            border: 1px solid #3a3a3a;
            border-radius: 3px;
            padding: 4px 8px;
        }
        QComboBox:hover {
            border: 1px solid #555;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: #252525;
            color: #d4d4d4;
            border: 1px solid #333;
            selection-background-color: #2563eb;
        }

        /* === Check Boxes === */
        QCheckBox {
            color: #d4d4d4;
            spacing: 6px;
        }
        QCheckBox::indicator {
            width: 14px;
            height: 14px;
            border: 1px solid #555;
            border-radius: 2px;
            background-color: #2a2a2a;
        }
        QCheckBox::indicator:checked {
            background-color: #2563eb;
            border: 1px solid #2563eb;
        }

        /* === Scroll Bars === */
        QScrollBar:vertical {
            background-color: #1a1a1a;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background-color: #444;
            border-radius: 5px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #555;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar:horizontal {
            background-color: #1a1a1a;
            height: 10px;
            margin: 0;
        }
        QScrollBar::handle:horizontal {
            background-color: #444;
            border-radius: 5px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #555;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }

        /* === Status Bar === */
        QStatusBar {
            background-color: #1e1e1e;
            color: #999;
            border-top: 1px solid #333;
            padding: 2px;
        }

        /* === Labels === */
        QLabel {
            color: #d4d4d4;
        }

        /* === Splitters === */
        QSplitter::handle {
            background-color: #333;
        }
        QSplitter::handle:hover {
            background-color: #2563eb;
        }

        /* === Form Layout Labels === */
        QFormLayout {
            margin: 4px;
        }

        /* === Tooltips === */
        QToolTip {
            background-color: #2a2a2a;
            color: #d4d4d4;
            border: 1px solid #555;
            padding: 4px;
        }
    )";

    qApp->setStyleSheet(style);
}

void MainWindow::loadProjectScripts()
{
    if (m_projectPath.isEmpty() || !m_scriptEngine) return;
    
    QString scriptsPath = m_projectPath + "/Scripts";
    QDir scriptsDir(scriptsPath);
    
    if (!scriptsDir.exists()) {
        DEBUG_LOG << "Scripts folder not found: " << scriptsPath.toStdString() << std::endl;
        return;
    }
    
    QStringList filters;
    filters << "*.lua" << "*.as";
    QFileInfoList scriptFiles = scriptsDir.entryInfoList(filters, QDir::Files);
    
    DEBUG_LOG << "Loading " << scriptFiles.size() << " script(s) from " << scriptsPath.toStdString() << std::endl;
    
    for (const QFileInfo& fileInfo : scriptFiles) {
        QString filePath = fileInfo.absoluteFilePath();
        QString extension = fileInfo.suffix().toLower();
        
        if (extension == "lua") {
            DEBUG_LOG << "Loading Lua script: " << filePath.toStdString() << std::endl;
            if (!m_scriptEngine->loadLuaScript(filePath.toStdString())) {
                DEBUG_LOG << "Failed to load Lua script: " << filePath.toStdString() << std::endl;
            }
        } else if (extension == "as") {
            DEBUG_LOG << "Loading AngelScript: " << filePath.toStdString() << std::endl;
            if (!m_scriptEngine->loadAngelScript(filePath.toStdString())) {
                DEBUG_LOG << "Failed to load AngelScript: " << filePath.toStdString() << std::endl;
            }
        }
    }
}


void MainWindow::openProjectManager()
{
    // Close the main window first to avoid multiple windows
    close();
    
    ProjectManager* projectManager = new ProjectManager();
    projectManager->setAttribute(Qt::WA_DeleteOnClose);
    projectManager->show();
    projectManager->raise();
    projectManager->activateWindow();
}

void MainWindow::openEsquemaEditor()
{
    // Esquema disabled for now
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // If we're in play mode, stop the game first
    if (m_editorMode == EditorMode::Play || m_editorMode == EditorMode::Paused) {
        onStopClicked();
    }
    
    // Save any unsaved changes if needed
    if (m_sceneDirty) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, 
            "Unsaved Changes", 
            "You have unsaved changes. Do you want to save before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );
        
        if (reply == QMessageBox::Save) {
            saveScene();
        } else if (reply == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    
    // Accept the close event
    event->accept();
    
    // If this is the last window and no project is open, don't try to reopen anything
    if (m_projectPath.isEmpty()) {
        QApplication::quit();
    }
}
