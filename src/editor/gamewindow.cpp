#include "editor/gamewindow.h"
#include "debug/logger.h"

GameWindow::GameWindow(DabozzEngine::ECS::World* world, QWidget* parent)
    : QWidget(parent)
    , m_world(world)
{
    DEBUG_LOG << "GameWindow constructor start" << std::endl;
    setWindowTitle("Game View");
    resize(1280, 720);
    DEBUG_LOG << "Calling setupUI" << std::endl;
    setupUI();
    DEBUG_LOG << "GameWindow constructor complete" << std::endl;
}

GameWindow::~GameWindow()
{
    DEBUG_LOG << "GameWindow destructor" << std::endl;
}

void GameWindow::setupUI()
{
    DEBUG_LOG << "GameWindow setupUI start" << std::endl;
    m_mainLayout = new QVBoxLayout(this);
    DEBUG_LOG << "Layout created" << std::endl;
    
    m_renderer = new OpenGLRenderer(this);
    DEBUG_LOG << "Renderer created" << std::endl;
    m_renderer->setWorld(m_world);
    DEBUG_LOG << "World set on renderer" << std::endl;
    m_renderer->setMinimumSize(800, 600);
    DEBUG_LOG << "Renderer size set" << std::endl;
    
    m_mainLayout->addWidget(m_renderer);
    DEBUG_LOG << "Renderer added to layout" << std::endl;
    setLayout(m_mainLayout);
    DEBUG_LOG << "GameWindow setupUI complete" << std::endl;
}
