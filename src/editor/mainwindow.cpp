#include "editor/mainwindow.h"
#include "editor/sceneview.h"
#include "editor/componentinspector.h"
#include "editor/hierarchyview.h"
#include "editor/gamewindow.h"
#include "editor/scripteditor.h"
#include "editor/animatorgrapheditor.h"
#include "ecs/components/transform.h"
#include "ecs/components/name.h"
#include "ecs/components/mesh.h"
#include "ecs/components/rigidbody.h"
#include "ecs/components/boxcollider.h"
#include "ecs/components/hierarchy.h"
#include "ecs/components/animator.h"
#include "ecs/systems/animationsystem.h"
#include "renderer/meshloader.h"
#include "renderer/animation.h"
#include "renderer/skeleton.h"
#include "physics/simplephysics.h"
#include "physics/physicssystem.h"
#include "ecs/components/rigidbody.h"
#include "editor/undostack.h"
#include "editor/scenefile.h"
#include "debug/logger.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_world(new DabozzEngine::ECS::World())
    , m_gameWindow(nullptr)
    , m_scriptEditor(nullptr)
    , m_centralTabs(nullptr)
    , m_butsuri(nullptr)
    , m_physicsSystem(nullptr)
    , m_animationSystem(nullptr)
    , m_gameLoopTimer(new QTimer(this))
    , m_undoStack(new QUndoStack(this))
    , m_editorMode(EditorMode::Edit)
{
    setWindowTitle("Dabozz Editor");
    resize(1200, 800);
    
    createMenus();
    createToolBars();
    createDockWidgets();
    createStatusBar();
    setupLayout();
    
    connectViews();
    createSampleEntities();
    
    // Don't initialize physics at startup - do it when entering play mode
    m_butsuri = nullptr;
    m_physicsSystem = nullptr;
    m_animationSystem = new DabozzEngine::Systems::AnimationSystem(m_world);
    
    // Setup game loop timer (60 FPS)
    connect(m_gameLoopTimer, &QTimer::timeout, this, &MainWindow::updateGameLoop);
}

MainWindow::~MainWindow()
{
    if (m_gameWindow) {
        delete m_gameWindow;
    }
    if (m_animationSystem) {
        delete m_animationSystem;
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
	m_editToolBar = addToolBar("Edit");

	QAction *new_action = m_mainToolBar->addAction(QIcon("editor/icons/File.svg"), "New");
	connect(new_action, &QAction::triggered, this, &MainWindow::newScene);

	QAction *open_action = m_mainToolBar->addAction(QIcon("editor/icons/Load.svg"), "Open");
	connect(open_action, &QAction::triggered, this, &MainWindow::openScene);

	QAction *save_action = m_mainToolBar->addAction(QIcon("editor/icons/Save.svg"), "Save");
	connect(save_action, &QAction::triggered, this, &MainWindow::saveScene);

	m_mainToolBar->addSeparator();

	QAction *play_action = m_mainToolBar->addAction(QIcon("editor/icons/Play.svg"), "Play");
	connect(play_action, &QAction::triggered, this, &MainWindow::onPlayClicked);

	QAction *pause_action = m_mainToolBar->addAction(QIcon("editor/icons/Pause.svg"), "Pause");
	connect(pause_action, &QAction::triggered, this, &MainWindow::onPauseClicked);

	QAction *stop_action = m_mainToolBar->addAction(QIcon("editor/icons/Stop.svg"), "Stop");
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

    QString fileName = QFileDialog::getOpenFileName(this, "Open Scene", "", "DabozzEngine Scene (*.dabozz);;All Files (*.*)");
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
    QString fileName = QFileDialog::getSaveFileName(this, "Save Scene As", "", "DabozzEngine Scene (*.dabozz);;All Files (*.*)");
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

        // Save scene state before anything gets modified
        saveSceneState();

        // Initialize physics on first play
        if (!m_butsuri) {
            DEBUG_LOG << "Initializing Butsuri Engine" << std::endl;
            m_butsuri = new DabozzEngine::Physics::ButsuriEngine();
            m_butsuri->initialize();
            m_physicsSystem = new DabozzEngine::Systems::PhysicsSystem(m_world);
            m_physicsSystem->initialize();
            DEBUG_LOG << "Butsuri Engine initialized" << std::endl;
        }
        
        // Freeze the editor view during play mode
        m_sceneView->renderer()->setPlayMode(true);

        // Disable gizmo and editor selection
        DEBUG_LOG << "Disabling gizmo" << std::endl;
        m_sceneView->renderer()->setSelectedEntity(DabozzEngine::ECS::INVALID_ENTITY);
        
        // Mark all meshes as not uploaded so game window can upload them fresh
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
        
        // Create and show game window
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
        
        // Start game loop (60 FPS = ~16ms)
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
        
        // Update animations
        if (m_animationSystem) {
            m_animationSystem->update(deltaTime);
        }
        
        // Update physics
        if (m_physicsSystem) {
            m_physicsSystem->update(deltaTime);
        }
        
        // Update renderer
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
