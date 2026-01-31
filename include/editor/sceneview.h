#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include "renderer/openglrenderer.h"
#include "ecs/world.h"

class SceneView : public QWidget
{
    Q_OBJECT

public:
    SceneView(QWidget* parent = nullptr);
    ~SceneView();

    void setWorld(DabozzEngine::ECS::World* world);
    OpenGLRenderer* renderer() const { return m_renderer; }

public slots:
    void setSelectedEntity(DabozzEngine::ECS::EntityID entity);

public slots:
    void setModeLabel(const QString& text);

signals:
    void playClicked();
    void pauseClicked();
    void stopClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void setupUI();
    
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_toolbarLayout;
    OpenGLRenderer* m_renderer;
    QPushButton* m_playButton;
    QPushButton* m_pauseButton;
    QPushButton* m_stopButton;
    QLabel* m_modeLabel;
};