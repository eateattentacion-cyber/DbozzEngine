#include "editor/gamewindow.h"
#include "input/inputmanager.h"
#include "debug/logger.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

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
    m_renderer->setAnimationEnabled(true);
    DEBUG_LOG << "World set on renderer" << std::endl;
    m_renderer->setMinimumSize(800, 600);
    DEBUG_LOG << "Renderer size set" << std::endl;
    
    m_mainLayout->addWidget(m_renderer);
    DEBUG_LOG << "Renderer added to layout" << std::endl;
    setLayout(m_mainLayout);
    
    setFocusPolicy(Qt::StrongFocus);
    m_renderer->setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    m_renderer->setMouseTracking(true);
    
    DEBUG_LOG << "GameWindow setupUI complete" << std::endl;
}

void GameWindow::keyPressEvent(QKeyEvent* event)
{
    DabozzEngine::Input::InputManager::getInstance().keyPressed(event->key());
    QWidget::keyPressEvent(event);
}

void GameWindow::keyReleaseEvent(QKeyEvent* event)
{
    DabozzEngine::Input::InputManager::getInstance().keyReleased(event->key());
    QWidget::keyReleaseEvent(event);
}

void GameWindow::mousePressEvent(QMouseEvent* event)
{
    DabozzEngine::Input::InputManager::getInstance().mousePressed(event->button());
    QWidget::mousePressEvent(event);
}

void GameWindow::mouseReleaseEvent(QMouseEvent* event)
{
    DabozzEngine::Input::InputManager::getInstance().mouseReleased(event->button());
    QWidget::mouseReleaseEvent(event);
}

void GameWindow::mouseMoveEvent(QMouseEvent* event)
{
    DabozzEngine::Input::InputManager::getInstance().mouseMoved(event->pos());
    QWidget::mouseMoveEvent(event);
}

void GameWindow::wheelEvent(QWheelEvent* event)
{
    DabozzEngine::Input::InputManager::getInstance().mouseScrolled(event->angleDelta().y());
    QWidget::wheelEvent(event);
}
