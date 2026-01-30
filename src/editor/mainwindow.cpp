#include "editor/mainwindow.h"
#include "editor/sceneview.h"
#include "editor/componentinspector.h"
#include "editor/hierarchyview.h"
#include "editor/gamewindow.h"
#include "ecs/components/transform.h"
#include "ecs/components/name.h"
#include "ecs/components/mesh.h"
#include "renderer/meshloader.h"
#include "physics/simplephysics.h"
#include "physics/physicssystem.h"
#include "debug/logger.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_world(new DabozzEngine::ECS::World())
    , m_gameWindow(nullptr)
    , m_butsuri(nullptr)
    , m_physicsSystem(nullptr)
    , m_gameLoopTimer(new QTimer(this))
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
    
    // Setup game loop timer (60 FPS)
    connect(m_gameLoopTimer, &QTimer::timeout, this, &MainWindow::updateGameLoop);
}

MainWindow::~MainWindow()
{
    if (m_gameWindow) {
        delete m_gameWindow;
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
    
    m_fileMenu->addSeparator();
    
    QAction* exitAction = m_fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::exitApplication);
}

void MainWindow::createToolBars()
{
    m_mainToolBar = addToolBar("Main");
    m_editToolBar = addToolBar("Edit");
    
    m_mainToolBar->addAction(QIcon(), "New");
    m_mainToolBar->addAction(QIcon(), "Open");
    m_mainToolBar->addAction(QIcon(), "Save");
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(QIcon(), "Play");
    m_mainToolBar->addAction(QIcon(), "Pause");
    m_mainToolBar->addAction(QIcon(), "Stop");
}

void MainWindow::createDockWidgets()
{
    m_hierarchyView = new HierarchyView(this);
    m_hierarchyView->setWorld(m_world);
    QDockWidget* hierarchyDock = new QDockWidget("Hierarchy", this);
    hierarchyDock->setWidget(m_hierarchyView);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);
    
    m_componentInspector = new ComponentInspector(this);
    m_componentInspector->setWorld(m_world);
    QDockWidget* inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setWidget(m_componentInspector);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    
    m_sceneView = new SceneView(this);
    m_sceneView->setWorld(m_world);
    setCentralWidget(m_sceneView);
}

void MainWindow::connectViews()
{
    connect(m_hierarchyView, &HierarchyView::entitySelected, m_componentInspector, &ComponentInspector::setSelectedEntity);
    connect(m_hierarchyView, &HierarchyView::entitySelected, m_sceneView, &SceneView::setSelectedEntity);
    connect(m_sceneView->renderer(), &OpenGLRenderer::selectedEntityTransformChanged, m_componentInspector, &ComponentInspector::refreshSelectedEntity);
    
    // Connect play mode buttons
    connect(m_sceneView, &SceneView::playClicked, this, &MainWindow::onPlayClicked);
    connect(m_sceneView, &SceneView::pauseClicked, this, &MainWindow::onPauseClicked);
    connect(m_sceneView, &SceneView::stopClicked, this, &MainWindow::onStopClicked);
}

void MainWindow::createSampleEntities()
{
    // idk
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
    QMessageBox::information(this, "New Scene", "Creating new scene...");
}

void MainWindow::openScene()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Scene", "", "Scene Files (*.scene)");
    if (!fileName.isEmpty()) {
        statusBar()->showMessage(QString("Opened scene: %1").arg(fileName));
    }
}

void MainWindow::saveScene()
{
    statusBar()->showMessage("Scene saved");
}

void MainWindow::saveSceneAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Scene As", "", "Scene Files (*.scene)");
    if (!fileName.isEmpty()) {
        statusBar()->showMessage(QString("Saved scene as: %1").arg(fileName));
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
    
    auto meshes = DabozzEngine::Renderer::MeshLoader::LoadMesh(fileName.toStdString());

    if (!meshes.empty()) {
        QFileInfo fileInfo(fileName);
        
        for (size_t i = 0; i < meshes.size(); ++i) {
            DabozzEngine::ECS::EntityID entity = m_world->createEntity();
            
            auto* name = m_world->addComponent<DabozzEngine::ECS::Name>(entity);
            name->name = (meshes.size() > 1) 
                ? QString("%1_part%2").arg(fileInfo.baseName()).arg(i + 1)
                : fileInfo.baseName();

            m_world->addComponent<DabozzEngine::ECS::Transform>(entity);
            
            auto* meshComponent = m_world->addComponent<DabozzEngine::ECS::Mesh>(entity);
            *meshComponent = meshes[i];
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

void MainWindow::onPlayClicked()
{
    DEBUG_LOG << "=== PLAY BUTTON CLICKED ===" << std::endl;
    
    if (m_editorMode == EditorMode::Edit) {
        DEBUG_LOG << "Entering play mode from edit mode" << std::endl;
        m_editorMode = EditorMode::Play;
        statusBar()->showMessage("Play Mode");
        
        // Initialize physics on first play
        if (!m_butsuri) {
            DEBUG_LOG << "Initializing Butsuri Engine" << std::endl;
            m_butsuri = new DabozzEngine::Physics::ButsuriEngine();
            m_butsuri->initialize();
            m_physicsSystem = new DabozzEngine::Systems::PhysicsSystem(m_world);
            m_physicsSystem->initialize();
            DEBUG_LOG << "Butsuri Engine initialized" << std::endl;
        }
        
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
                // Don't reset textureID as it might be reusable
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
        m_gameLoopTimer->start(16);
    }
}

void MainWindow::onPauseClicked()
{
    if (m_editorMode == EditorMode::Play) {
        m_editorMode = EditorMode::Paused;
        statusBar()->showMessage("Play Mode (Paused)");
        m_gameLoopTimer->stop();
    }
}

void MainWindow::onStopClicked()
{
    if (m_editorMode == EditorMode::Play || m_editorMode == EditorMode::Paused) {
        m_editorMode = EditorMode::Edit;
        statusBar()->showMessage("Edit Mode");
        
        // Stop game loop
        m_gameLoopTimer->stop();
        
        // Hide game window
        if (m_gameWindow) {
            m_gameWindow->hide();
        }
        
        // TODO: Restore scene state
        
        m_hierarchyView->refreshHierarchy();
    }
}

void MainWindow::updateGameLoop()
{
    if (m_editorMode == EditorMode::Play) {
        if (m_physicsSystem) {
            m_physicsSystem->update(1.0f / 60.0f);
        }
        
        if (m_gameWindow) {
            m_gameWindow->renderer()->update();
        }
    }
}
