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

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void setupUI();
    
    QVBoxLayout* m_mainLayout;
    OpenGLRenderer* m_renderer;
    DabozzEngine::ECS::World* m_world;
};
