#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include "renderer/openglrenderer.h"
#include "ecs/world.h"

class GameWindow : public QWidget
{
    Q_OBJECT

public:
    GameWindow(DabozzEngine::ECS::World* world, QWidget* parent = nullptr);
    ~GameWindow();

    OpenGLRenderer* renderer() const { return m_renderer; }

private:
    void setupUI();
    
    QVBoxLayout* m_mainLayout;
    OpenGLRenderer* m_renderer;
    DabozzEngine::ECS::World* m_world;
};
