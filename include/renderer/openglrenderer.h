#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QPointF>
#include <QTimer>
#include <QVector3D>
#include "ecs/world.h"

class OpenGLRenderer : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    OpenGLRenderer(QWidget* parent = nullptr);
    ~OpenGLRenderer();

    void setWorld(DabozzEngine::ECS::World* world);
    DabozzEngine::ECS::World* getWorld() const { return m_world; }

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void setClearColor(float r, float g, float b, float a = 1.0f);
    void setPlayMode(bool playing);
    void setAnimationEnabled(bool enabled) { m_animationEnabled = enabled; }

public slots:
    void setSelectedEntity(DabozzEngine::ECS::EntityID entity);

signals:
    void entitySelected(DabozzEngine::ECS::EntityID entity);
    void selectedEntityTransformChanged(DabozzEngine::ECS::EntityID entity);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void updateAnimation();

private:
    enum class GizmoMode { Translate, Rotate, Scale };
    enum class GizmoAxis { None, X, Y, Z, XY, YZ, XZ, Center };

    void setupShaders();
    void setupGeometry();
    void setupMatrices();
    void setupGrid();
    void setupSkybox();
    void renderColliders();
    void renderGizmo();
    void renderGrid();
    void renderSkybox();
    void renderTranslateGizmo(const QVector3D& position, float scale);
    void renderRotateGizmo(const QVector3D& position, float scale);
    void renderScaleGizmo(const QVector3D& position, float scale);
    
    QMatrix4x4 getWorldTransform(DabozzEngine::ECS::EntityID entity) const;

    struct Ray {
        QVector3D origin;
        QVector3D direction;
    };

    Ray makeRayFromMouse(const QPointF& mousePos) const;
    GizmoAxis pickGizmoAxis(const QPointF& mousePos) const;
    QVector3D axisDirection(GizmoAxis axis) const;
    QVector3D computeDragPlaneNormal(const QVector3D& axisDirection) const;
    bool computeAxisValue(const QPointF& mousePos,
                          const QVector3D& origin,
                          const QVector3D& axisDirection,
                          const QVector3D& planeNormal,
                          float& outAxisValue) const;
    bool intersectRayAabb(const QVector3D& rayOrigin,
                          const QVector3D& rayDirection,
                          const QVector3D& boxMin,
                          const QVector3D& boxMax,
                          float& outT) const;

    static constexpr float kGizmoAxisLength = 1.0f;
    static constexpr float kGizmoAxisThickness = 0.05f;
    static constexpr float kGizmoPickThickness = 0.2f;
    static constexpr float kGizmoArrowSize = 0.15f;
    static constexpr float kGizmoPlaneSize = 0.25f;

    QOpenGLShaderProgram* m_shaderProgram;
    QOpenGLShaderProgram* m_skyboxShader;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    GLuint m_gridVAO;
    GLuint m_gridVBO;
    GLuint m_arrowVAO;
    GLuint m_arrowVBO;
    GLuint m_arrowEBO;
    GLuint m_skyboxVAO;
    GLuint m_skyboxVBO;
    
    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;
    QMatrix4x4 m_model;
    
    QTimer* m_animationTimer;
    float m_rotationAngle;

    float m_clearColor[4];
    DabozzEngine::ECS::World* m_world;
    DabozzEngine::ECS::EntityID m_selectedEntity;
    bool m_draggingGizmo;
    bool m_rightMouseDown;
    GizmoAxis m_activeAxis;
    GizmoAxis m_hoverAxis;
    float m_dragStartAxisValue;
    QVector3D m_dragStartPosition;
    QVector3D m_dragStartScale;
    QQuaternion m_dragStartRotation;
    QVector3D m_dragPlaneNormal;
    QVector3D m_cameraPosition;
    QVector3D m_cameraForward;
    QVector3D m_cameraRight;
    QVector3D m_cameraUp;
    bool m_hasCamera;
    bool m_playMode = false;
    bool m_animationEnabled = false;
    GizmoMode m_gizmoMode = GizmoMode::Translate;
};