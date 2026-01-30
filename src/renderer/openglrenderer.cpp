#include "renderer/openglrenderer.h"
#include "ecs/components/transform.h"
#include "ecs/components/mesh.h"
#include "ecs/components/firstpersoncontroller.h"
#include "ecs/components/boxcollider.h"
#include "ecs/components/spherecollider.h"
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QtMath>
#include <QVector4D>
#include <algorithm>
#include <cmath>
#include "debug/logger.h"
#include <limits>

#define STB_IMAGE_IMPLEMENTATION
#include "../assimp_source/contrib/stb/stb_image.h"

OpenGLRenderer::OpenGLRenderer(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_shaderProgram(nullptr)
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
    , m_gridVAO(0)
    , m_gridVBO(0)
    , m_rotationAngle(0.0f)
    , m_world(nullptr)
    , m_selectedEntity(DabozzEngine::ECS::INVALID_ENTITY)
    , m_draggingGizmo(false)
    , m_rightMouseDown(false)
    , m_activeAxis(GizmoAxis::None)
    , m_hoverAxis(GizmoAxis::None)
    , m_dragStartAxisValue(0.0f)
    , m_dragStartPosition(0.0f, 0.0f, 0.0f)
    , m_dragPlaneNormal(0.0f, 0.0f, 1.0f)
    , m_cameraPosition(0.0f, 0.0f, 3.0f)
    , m_cameraForward(0.0f, 0.0f, -1.0f)
    , m_cameraRight(1.0f, 0.0f, 0.0f)
    , m_cameraUp(0.0f, 1.0f, 0.0f)
    , m_hasCamera(true)
{
    setFocusPolicy(Qt::StrongFocus);
    m_clearColor[0] = 0.2f;
    m_clearColor[1] = 0.3f;
    m_clearColor[2] = 0.3f;
    m_clearColor[3] = 1.0f;
    
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &OpenGLRenderer::updateAnimation);
    m_animationTimer->start(16);
}

OpenGLRenderer::~OpenGLRenderer()
{
    makeCurrent();
    delete m_shaderProgram;
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    glDeleteVertexArrays(1, &m_gridVAO);
    glDeleteBuffers(1, &m_gridVBO);
    doneCurrent();
}

void OpenGLRenderer::setWorld(DabozzEngine::ECS::World* world)
{
    m_world = world;
}

void OpenGLRenderer::setSelectedEntity(DabozzEngine::ECS::EntityID entity)
{
    m_selectedEntity = entity;
    m_draggingGizmo = false;
    m_activeAxis = GizmoAxis::None;
    m_hoverAxis = GizmoAxis::None;
    update();
}

void OpenGLRenderer::initializeGL()
{
    DEBUG_LOG << "OpenGLRenderer::initializeGL start" << std::endl;
    initializeOpenGLFunctions();
    DEBUG_LOG << "OpenGL functions initialized" << std::endl;
    
    glEnable(GL_DEPTH_TEST);
    DEBUG_LOG << "Depth test enabled" << std::endl;
    glDisable(GL_CULL_FACE);
    DEBUG_LOG << "Cull face disabled" << std::endl;
    // glCullFace(GL_BACK);
    
    DEBUG_LOG << "Setting up shaders..." << std::endl;
    setupShaders();
    DEBUG_LOG << "Setting up geometry..." << std::endl;
    setupGeometry();
    DEBUG_LOG << "Setting up matrices..." << std::endl;
    setupMatrices();
    DEBUG_LOG << "Setting up grid..." << std::endl;
    setupGrid();
    DEBUG_LOG << "OpenGLRenderer::initializeGL complete" << std::endl;
}

void OpenGLRenderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    float aspect = float(w) / float(h ? h : 1);
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, aspect, 0.1f, 100.0f);
}

void OpenGLRenderer::paintGL()
{
    glClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (!m_shaderProgram || !m_shaderProgram->isLinked())
        return;
        
    m_shaderProgram->bind();
    
    const QVector3D viewPos = m_hasCamera ? m_cameraPosition : QVector3D(0.0f, 0.0f, 3.0f);
    
    // Render entities from ECS
    if (m_world) {
        DEBUG_LOG << "Rendering " << m_world->getEntities().size() << " entities" << std::endl;
        for (DabozzEngine::ECS::EntityID entity : m_world->getEntities()) {
            DEBUG_LOG << "Processing entity " << entity << std::endl;
            DabozzEngine::ECS::Transform* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(entity);
            DabozzEngine::ECS::Mesh* mesh = m_world->getComponent<DabozzEngine::ECS::Mesh>(entity);
            
            DEBUG_LOG << "Entity " << entity << " - transform: " << (transform != nullptr) << " mesh: " << (mesh != nullptr) << std::endl;
            
            if (transform && mesh) {
                DEBUG_LOG << "Entity " << entity << " has mesh, checking upload status: " << mesh->isUploaded << std::endl;
                // Upload mesh to GPU if not already uploaded
                if (!mesh->isUploaded && !mesh->vertices.empty()) {
                    DEBUG_LOG << "Uploading mesh for entity " << entity << std::endl;
                    makeCurrent();
                    
                    glGenVertexArrays(1, &mesh->vao);
                    glGenBuffers(1, &mesh->vbo);
                    glGenBuffers(1, &mesh->ebo);
                    
                    glBindVertexArray(mesh->vao);
                    
                    // Interleave vertex data: position(3) + normal(3) + texcoord(2)
                    std::vector<float> interleavedData;
                    size_t vertexCount = mesh->vertices.size() / 3;
                    for (size_t i = 0; i < vertexCount; i++) {
                        // Position
                        interleavedData.push_back(mesh->vertices[i * 3 + 0]);
                        interleavedData.push_back(mesh->vertices[i * 3 + 1]);
                        interleavedData.push_back(mesh->vertices[i * 3 + 2]);
                        // Normal
                        if (i * 3 + 2 < mesh->normals.size()) {
                            interleavedData.push_back(mesh->normals[i * 3 + 0]);
                            interleavedData.push_back(mesh->normals[i * 3 + 1]);
                            interleavedData.push_back(mesh->normals[i * 3 + 2]);
                        } else {
                            interleavedData.push_back(0.0f);
                            interleavedData.push_back(1.0f);
                            interleavedData.push_back(0.0f);
                        }
                        // TexCoord
                        if (i * 2 + 1 < mesh->texCoords.size()) {
                            interleavedData.push_back(mesh->texCoords[i * 2 + 0]);
                            interleavedData.push_back(mesh->texCoords[i * 2 + 1]);
                        } else {
                            interleavedData.push_back(0.0f);
                            interleavedData.push_back(0.0f);
                        }
                    }
                    
                    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
                    glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), interleavedData.data(), GL_STATIC_DRAW);
                    
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(unsigned int), mesh->indices.data(), GL_STATIC_DRAW);
                    
                    // Position attribute
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
                    glEnableVertexAttribArray(0);
                    // Normal attribute
                    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
                    glEnableVertexAttribArray(1);
                    // TexCoord attribute
                    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
                    glEnableVertexAttribArray(2);
                    
                    glBindVertexArray(0);
                    
                    // Upload texture if available
                    if (mesh->hasTexture && mesh->textureID == 0) {
                        DEBUG_LOG << "=== TEXTURE UPLOAD START ===" << std::endl;
                        DEBUG_LOG << "Path: " << mesh->texturePath << std::endl;
                        DEBUG_LOG << "Embedded data size: " << mesh->embeddedTextureData.size() << std::endl;

                        glGenTextures(1, &mesh->textureID);
                        glBindTexture(GL_TEXTURE_2D, mesh->textureID);

                        int width = 0, height = 0, channels = 0;
                        unsigned char* imageData = nullptr;
                        bool loaded = false;

                        if (!mesh->embeddedTextureData.empty()) {
                            if (mesh->texturePath == "embedded_compressed") {
                                DEBUG_LOG << "Loading compressed embedded texture with STB..." << std::endl;
                                imageData = stbi_load_from_memory(
                                    mesh->embeddedTextureData.data(),
                                    mesh->embeddedTextureData.size(),
                                    &width, &height, &channels, 4
                                );
                                loaded = (imageData != nullptr);
                            } else if (mesh->texturePath == "embedded_raw") {
                                DEBUG_LOG << "Using raw embedded texture data..." << std::endl;
                                width = mesh->embeddedTextureWidth;
                                height = mesh->embeddedTextureHeight;
                                channels = 4;
                                imageData = mesh->embeddedTextureData.data();
                                loaded = true;
                            }
                        } else if (!mesh->texturePath.empty()) {
                            DEBUG_LOG << "Loading external texture: " << mesh->texturePath << std::endl;
                            imageData = stbi_load(mesh->texturePath.c_str(), &width, &height, &channels, 4);
                            loaded = (imageData != nullptr);
                        }
                        
                        if (loaded && imageData) {
                            DEBUG_LOG << "Texture loaded: " << width << "x" << height << " channels: " << channels << std::endl;
                            
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                            glGenerateMipmap(GL_TEXTURE_2D);
                            
                            GLenum err = glGetError();
                            if (err != GL_NO_ERROR) {
                                DEBUG_LOG << "OpenGL ERROR: " << err << std::endl;
                                mesh->hasTexture = false;
                                glDeleteTextures(1, &mesh->textureID);
                                mesh->textureID = 0;
                            } else {
                                DEBUG_LOG << "SUCCESS: Texture ID " << mesh->textureID << std::endl;
                            }
                            
                            // Free STB allocated memory (but not raw embedded data)
                            if (mesh->texturePath != "embedded_raw" && imageData) {
                                stbi_image_free(imageData);
                            }
                        } else {
                            DEBUG_LOG << "FAILED to load texture" << std::endl;
                            if (mesh->texturePath != "embedded_raw" && mesh->texturePath != "embedded_compressed") {
                                DEBUG_LOG << "STB Error: " << stbi_failure_reason() << std::endl;
                            }
                            mesh->hasTexture = false;
                            glDeleteTextures(1, &mesh->textureID);
                            mesh->textureID = 0;
                        }
                        
                        glBindTexture(GL_TEXTURE_2D, 0);
                        DEBUG_LOG << "=== TEXTURE UPLOAD END ===" << std::endl;
                    }
                    
                    mesh->isUploaded = true;
                }
                
                if (mesh->isUploaded) {
                    QMatrix4x4 modelMatrix = transform->getModelMatrix();
                    
                    m_shaderProgram->setUniformValue("model", modelMatrix);
                    m_shaderProgram->setUniformValue("view", m_view);
                    m_shaderProgram->setUniformValue("projection", m_projection);
                    
                    m_shaderProgram->setUniformValue("lightPos", QVector3D(2.0f, 2.0f, 2.0f));
                    m_shaderProgram->setUniformValue("viewPos", viewPos);
                    m_shaderProgram->setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
                    
                    if (mesh->hasTexture && mesh->textureID != 0) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, mesh->textureID);
                        m_shaderProgram->setUniformValue("useTexture", 1);
                        m_shaderProgram->setUniformValue("textureSampler", 0);
                    } else {
                        m_shaderProgram->setUniformValue("useTexture", 0);
                        m_shaderProgram->setUniformValue("objectColor", QVector3D(0.8f, 0.2f, 0.2f));
                    }
                    
                    glBindVertexArray(mesh->vao);
                    glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
                    glBindVertexArray(0);
                    
                    if (mesh->hasTexture) {
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }
                }
            }
        }
    }
    
    m_shaderProgram->release();
    
    // Render collider debug wireframes (green)
    renderColliders();

    // Render grid
    renderGrid();

    // Render transform gizmo for selected entity
    renderGizmo();
}

void OpenGLRenderer::setupShaders()
{
    DEBUG_LOG << "setupShaders: Creating shader program" << std::endl;
    m_shaderProgram = new QOpenGLShaderProgram(this);
    
    DEBUG_LOG << "setupShaders: Loading vertex shader" << std::endl;
    bool vertSuccess = m_shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vertex.glsl");
    DEBUG_LOG << "setupShaders: Loading fragment shader" << std::endl;
    bool fragSuccess = m_shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragment.glsl");
    
    if (!vertSuccess) {
        DEBUG_LOG << "Vertex shader failed to load" << std::endl;
        DEBUG_LOG << m_shaderProgram->log().toStdString() << std::endl;
    }
    
    if (!fragSuccess) {
        DEBUG_LOG << "Fragment shader failed to load" << std::endl;
        DEBUG_LOG << m_shaderProgram->log().toStdString() << std::endl;
    }
    
    DEBUG_LOG << "setupShaders: Linking shader program" << std::endl;
    bool linkSuccess = m_shaderProgram->link();
    
    if (!linkSuccess) {
        DEBUG_LOG << "Shader linking failed:" << std::endl;
        DEBUG_LOG << m_shaderProgram->log().toStdString() << std::endl;
    } else {
        DEBUG_LOG << "Shaders loaded successfully" << std::endl;
    }
}

void OpenGLRenderer::setupGeometry()
{
    // Correct cube vertices with proper winding order
    float vertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
         
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         
        // Right face
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         
        // Left face
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f
    };

    // Proper cube indices with correct winding
    unsigned int indices[] = {
        // Front face
        0,  1,  2,  2,  3,  0,
        // Back face  
        4,  5,  6,  6,  7,  4,
        // Top face
        8,  9, 10, 10, 11,  8,
        // Bottom face
        12, 13, 14, 14, 15, 12,
        // Right face
        16, 17, 18, 18, 19, 16,
        // Left face
        20, 21, 22, 22, 23, 20
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void OpenGLRenderer::setupMatrices()
{
    m_view.setToIdentity();
    m_view.translate(0.0f, 0.0f, -3.0f);
    
    m_model.setToIdentity();
    m_cameraPosition = QVector3D(0.0f, 0.0f, 3.0f);
    m_cameraForward = QVector3D(0.0f, 0.0f, -1.0f);
    m_cameraRight = QVector3D(1.0f, 0.0f, 0.0f);
    m_cameraUp = QVector3D(0.0f, 1.0f, 0.0f);
    m_hasCamera = true;
}

void OpenGLRenderer::setClearColor(float r, float g, float b, float a)
{
    m_clearColor[0] = r;
    m_clearColor[1] = g;
    m_clearColor[2] = b;
    m_clearColor[3] = a;
    update();
}

void OpenGLRenderer::keyPressEvent(QKeyEvent* event)
{
    if (!m_world) return;
    
    for (DabozzEngine::ECS::EntityID entity : m_world->getEntities()) {
        DabozzEngine::ECS::FirstPersonController* controller = m_world->getComponent<DabozzEngine::ECS::FirstPersonController>(entity);
        if (controller) {
            switch (event->key()) {
                case Qt::Key_W:
                    controller->moveForward = true;
                    break;
                case Qt::Key_S:
                    controller->moveBackward = true;
                    break;
                case Qt::Key_A:
                    controller->moveLeft = true;
                    break;
                case Qt::Key_D:
                    controller->moveRight = true;
                    break;
                case Qt::Key_E:
                    controller->moveUp = true;
                    break;
                case Qt::Key_Q:
                    controller->moveDown = true;
                    break;
                case Qt::Key_Escape:
                    m_rightMouseDown = false;
                    releaseMouse();
                    setCursor(Qt::ArrowCursor);
                    break;
            }
        }
    }
}

void OpenGLRenderer::keyReleaseEvent(QKeyEvent* event)
{
    if (!m_world) return;

    for (DabozzEngine::ECS::EntityID entity : m_world->getEntities()) {
        DabozzEngine::ECS::FirstPersonController* controller = m_world->getComponent<DabozzEngine::ECS::FirstPersonController>(entity);
        if (controller) {
            switch (event->key()) {
                case Qt::Key_W:
                    controller->moveForward = false;
                    break;
                case Qt::Key_S:
                    controller->moveBackward = false;
                    break;
                case Qt::Key_A:
                    controller->moveLeft = false;
                    break;
                case Qt::Key_D:
                    controller->moveRight = false;
                    break;
                case Qt::Key_E:
                    controller->moveUp = false;
                    break;
                case Qt::Key_Q:
                    controller->moveDown = false;
                    break;
            }
        }
    }
}

void OpenGLRenderer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        m_rightMouseDown = true;
        grabMouse();
        setCursor(Qt::BlankCursor);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (!m_world || m_selectedEntity == DabozzEngine::ECS::INVALID_ENTITY) return;

        DabozzEngine::ECS::Transform* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(m_selectedEntity);
        if (!transform) return;

        const GizmoAxis axis = pickGizmoAxis(event->position());
        if (axis != GizmoAxis::None) {
            m_activeAxis = axis;
            m_hoverAxis = axis;
            m_draggingGizmo = true;
            m_dragStartPosition = transform->position;
            m_dragPlaneNormal = computeDragPlaneNormal(axisDirection(axis));
            if (!computeAxisValue(event->position(), transform->position, axisDirection(axis), m_dragPlaneNormal, m_dragStartAxisValue)) {
                m_draggingGizmo = false;
                m_activeAxis = GizmoAxis::None;
            }
            update();
        }
    }
}

void OpenGLRenderer::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_world) return;

    if (m_rightMouseDown) {
        for (DabozzEngine::ECS::EntityID entity : m_world->getEntities()) {
            DabozzEngine::ECS::FirstPersonController* controller = m_world->getComponent<DabozzEngine::ECS::FirstPersonController>(entity);
            if (controller) {
                int dx = event->position().x() - width() / 2;
                int dy = event->position().y() - height() / 2;

                controller->yaw -= dx * controller->lookSpeed;
                controller->pitch -= dy * controller->lookSpeed;

                if (controller->pitch > 89.0f) controller->pitch = 89.0f;
                if (controller->pitch < -89.0f) controller->pitch = -89.0f;

                QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
            }
        }
        return;
    }

    if (m_draggingGizmo && m_activeAxis != GizmoAxis::None && m_selectedEntity != DabozzEngine::ECS::INVALID_ENTITY) {
        DabozzEngine::ECS::Transform* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(m_selectedEntity);
        if (!transform) return;

        float axisValue = 0.0f;
        if (computeAxisValue(event->position(), m_dragStartPosition, axisDirection(m_activeAxis), m_dragPlaneNormal, axisValue)) {
            const float delta = axisValue - m_dragStartAxisValue;
            transform->position = m_dragStartPosition + axisDirection(m_activeAxis) * delta;
            emit selectedEntityTransformChanged(m_selectedEntity);
            update();
        }
        return;
    }

    if (m_selectedEntity != DabozzEngine::ECS::INVALID_ENTITY) {
        const GizmoAxis hoverAxis = pickGizmoAxis(event->position());
        if (hoverAxis != m_hoverAxis) {
            m_hoverAxis = hoverAxis;
            update();
        }
    }
}

void OpenGLRenderer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        m_rightMouseDown = false;
        releaseMouse();
        setCursor(Qt::ArrowCursor);
    }

    if (event->button() == Qt::LeftButton) {
        if (m_draggingGizmo) {
            m_draggingGizmo = false;
            m_activeAxis = GizmoAxis::None;
            update();
        }
    }
}

void OpenGLRenderer::renderColliders()
{
    if (!m_world) return;

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0f);

    m_shaderProgram->bind();
    m_shaderProgram->setUniformValue("objectColor", QVector3D(0.0f, 1.0f, 0.0f));

    for (DabozzEngine::ECS::EntityID entity : m_world->getEntities()) {
        DabozzEngine::ECS::Transform* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(entity);
        DabozzEngine::ECS::BoxCollider* boxCollider = m_world->getComponent<DabozzEngine::ECS::BoxCollider>(entity);
        DabozzEngine::ECS::SphereCollider* sphereCollider = m_world->getComponent<DabozzEngine::ECS::SphereCollider>(entity);

        if (transform && (boxCollider || sphereCollider)) {
            QMatrix4x4 modelMatrix = transform->getModelMatrix();

            if (boxCollider) {
                QMatrix4x4 scaleMatrix;
                scaleMatrix.scale(boxCollider->size);
                modelMatrix = modelMatrix * scaleMatrix;
            }

            m_shaderProgram->setUniformValue("model", modelMatrix);
            m_shaderProgram->setUniformValue("view", m_view);
            m_shaderProgram->setUniformValue("projection", m_projection);

            glBindVertexArray(m_vao);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
    m_shaderProgram->release();
}

void OpenGLRenderer::renderGizmo()
{
    if (!m_world || m_selectedEntity == DabozzEngine::ECS::INVALID_ENTITY) {
        return;
    }
    if (!m_shaderProgram || !m_shaderProgram->isLinked()) return;

    DabozzEngine::ECS::Transform* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(m_selectedEntity);
    if (!transform) {
        return;
    }

    // Calculate distance-based scale so gizmo is always visible
    const QVector3D viewPos = m_hasCamera ? m_cameraPosition : QVector3D(0.0f, 0.0f, 3.0f);
    float distanceToCamera = (transform->position - viewPos).length();
    float gizmoScale = distanceToCamera * 0.15f; // Scale based on distance

    // Disable depth test so gizmo always renders on top
    glDisable(GL_DEPTH_TEST);

    m_shaderProgram->bind();
    m_shaderProgram->setUniformValue("view", m_view);
    m_shaderProgram->setUniformValue("projection", m_projection);
    m_shaderProgram->setUniformValue("lightPos", QVector3D(2.0f, 2.0f, 2.0f));
    m_shaderProgram->setUniformValue("viewPos", viewPos);
    m_shaderProgram->setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_shaderProgram->setUniformValue("useTexture", 0); // Disable textures for gizmo

    glBindVertexArray(m_vao);

    auto axisColor = [this](GizmoAxis axis, const QVector3D& baseColor) {
        if (axis == m_activeAxis || axis == m_hoverAxis) {
            return QVector3D(1.0f, 1.0f, 0.2f);
        }
        return baseColor;
    };

    auto drawAxis = [this, &transform, gizmoScale](GizmoAxis axis, const QVector3D& scale, const QVector3D& baseColor) {
        const QVector3D direction = axisDirection(axis);
        const QVector3D offset = direction * (kGizmoAxisLength * 0.5f * gizmoScale);
        QMatrix4x4 model;
        model.translate(transform->position + offset);
        model.scale(scale * gizmoScale);
        m_shaderProgram->setUniformValue("model", model);
        m_shaderProgram->setUniformValue("objectColor", baseColor);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    };

    drawAxis(GizmoAxis::X,
             QVector3D(kGizmoAxisLength, kGizmoAxisThickness, kGizmoAxisThickness),
             axisColor(GizmoAxis::X, QVector3D(1.0f, 0.1f, 0.1f)));
    drawAxis(GizmoAxis::Y,
             QVector3D(kGizmoAxisThickness, kGizmoAxisLength, kGizmoAxisThickness),
             axisColor(GizmoAxis::Y, QVector3D(0.1f, 1.0f, 0.1f)));
    drawAxis(GizmoAxis::Z,
             QVector3D(kGizmoAxisThickness, kGizmoAxisThickness, kGizmoAxisLength),
             axisColor(GizmoAxis::Z, QVector3D(0.1f, 0.4f, 1.0f)));

    glBindVertexArray(0);
    m_shaderProgram->release();
    
    // Re-enable depth test
    glEnable(GL_DEPTH_TEST);
}

OpenGLRenderer::Ray OpenGLRenderer::makeRayFromMouse(const QPointF& mousePos) const
{
    Ray ray;
    bool invertible = false;
    QMatrix4x4 inverseViewProjection = (m_projection * m_view).inverted(&invertible);
    if (!invertible || width() == 0 || height() == 0) {
        ray.origin = m_hasCamera ? m_cameraPosition : QVector3D(0.0f, 0.0f, 3.0f);
        ray.direction = m_hasCamera ? m_cameraForward.normalized() : QVector3D(0.0f, 0.0f, -1.0f);
        return ray;
    }

    const float ndcX = 2.0f * static_cast<float>(mousePos.x()) / static_cast<float>(width()) - 1.0f;
    const float ndcY = 1.0f - 2.0f * static_cast<float>(mousePos.y()) / static_cast<float>(height());

    QVector4D nearPoint = inverseViewProjection * QVector4D(ndcX, ndcY, -1.0f, 1.0f);
    QVector4D farPoint = inverseViewProjection * QVector4D(ndcX, ndcY, 1.0f, 1.0f);

    if (qFuzzyIsNull(nearPoint.w()) || qFuzzyIsNull(farPoint.w())) {
        ray.origin = m_hasCamera ? m_cameraPosition : QVector3D(0.0f, 0.0f, 3.0f);
        ray.direction = m_hasCamera ? m_cameraForward.normalized() : QVector3D(0.0f, 0.0f, -1.0f);
        return ray;
    }

    nearPoint /= nearPoint.w();
    farPoint /= farPoint.w();

    ray.origin = QVector3D(nearPoint.x(), nearPoint.y(), nearPoint.z());
    ray.direction = QVector3D(farPoint.x() - nearPoint.x(), farPoint.y() - nearPoint.y(), farPoint.z() - nearPoint.z()).normalized();
    return ray;
}

OpenGLRenderer::GizmoAxis OpenGLRenderer::pickGizmoAxis(const QPointF& mousePos) const
{
    if (!m_world || m_selectedEntity == DabozzEngine::ECS::INVALID_ENTITY) return GizmoAxis::None;

    DabozzEngine::ECS::Transform* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(m_selectedEntity);
    if (!transform) return GizmoAxis::None;

    const Ray ray = makeRayFromMouse(mousePos);
    const float halfPick = kGizmoPickThickness * 0.5f;
    const float halfLength = kGizmoAxisLength * 0.5f;

    struct HitResult {
        GizmoAxis axis;
        float t;
    };

    HitResult bestHit { GizmoAxis::None, std::numeric_limits<float>::max() };

    auto testAxis = [&](GizmoAxis axis, const QVector3D& halfExtents) {
        const QVector3D center = transform->position + axisDirection(axis) * halfLength;
        const QVector3D boxMin = center - halfExtents;
        const QVector3D boxMax = center + halfExtents;
        float t = 0.0f;
        if (intersectRayAabb(ray.origin, ray.direction, boxMin, boxMax, t)) {
            if (t < bestHit.t) {
                bestHit.axis = axis;
                bestHit.t = t;
            }
        }
    };

    testAxis(GizmoAxis::X, QVector3D(halfLength, halfPick, halfPick));
    testAxis(GizmoAxis::Y, QVector3D(halfPick, halfLength, halfPick));
    testAxis(GizmoAxis::Z, QVector3D(halfPick, halfPick, halfLength));

    return bestHit.axis;
}

QVector3D OpenGLRenderer::axisDirection(GizmoAxis axis) const
{
    switch (axis) {
        case GizmoAxis::X:
            return QVector3D(1.0f, 0.0f, 0.0f);
        case GizmoAxis::Y:
            return QVector3D(0.0f, 1.0f, 0.0f);
        case GizmoAxis::Z:
            return QVector3D(0.0f, 0.0f, 1.0f);
        default:
            return QVector3D(0.0f, 0.0f, 0.0f);
    }
}

QVector3D OpenGLRenderer::computeDragPlaneNormal(const QVector3D& axisDir) const
{
    QVector3D viewDir = m_hasCamera ? m_cameraForward.normalized() : QVector3D(0.0f, 0.0f, -1.0f);
    QVector3D normal = viewDir - axisDir * QVector3D::dotProduct(viewDir, axisDir);
    if (normal.lengthSquared() < 0.0001f) {
        normal = QVector3D(0.0f, 1.0f, 0.0f);
    }
    return normal.normalized();
}

bool OpenGLRenderer::computeAxisValue(const QPointF& mousePos,
                                      const QVector3D& origin,
                                      const QVector3D& axisDir,
                                      const QVector3D& planeNormal,
                                      float& outAxisValue) const
{
    const Ray ray = makeRayFromMouse(mousePos);
    const float denom = QVector3D::dotProduct(ray.direction, planeNormal);
    if (std::abs(denom) < 1e-5f) return false;

    const float t = QVector3D::dotProduct(origin - ray.origin, planeNormal) / denom;
    if (t < 0.0f) return false;

    const QVector3D hitPoint = ray.origin + ray.direction * t;
    outAxisValue = QVector3D::dotProduct(hitPoint - origin, axisDir);
    return true;
}

bool OpenGLRenderer::intersectRayAabb(const QVector3D& rayOrigin,
                                      const QVector3D& rayDirection,
                                      const QVector3D& boxMin,
                                      const QVector3D& boxMax,
                                      float& outT) const
{
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();

    auto updateSlab = [&](float origin, float direction, float minVal, float maxVal) {
        if (std::abs(direction) < 1e-6f) {
            if (origin < minVal || origin > maxVal) {
                tMin = 1.0f;
                tMax = 0.0f;
            }
            return;
        }

        float invDir = 1.0f / direction;
        float t1 = (minVal - origin) * invDir;
        float t2 = (maxVal - origin) * invDir;
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
    };

    updateSlab(rayOrigin.x(), rayDirection.x(), boxMin.x(), boxMax.x());
    updateSlab(rayOrigin.y(), rayDirection.y(), boxMin.y(), boxMax.y());
    updateSlab(rayOrigin.z(), rayDirection.z(), boxMin.z(), boxMax.z());

    if (tMax < tMin) return false;

    if (tMin < 0.0f) {
        if (tMax < 0.0f) return false;
        outT = tMax;
        return true;
    }

    outT = tMin;
    return true;
}

void OpenGLRenderer::updateAnimation()
{
    float deltaTime = 0.016f; 
    
    if (m_world) {
        bool cameraFound = false;
        for (DabozzEngine::ECS::EntityID entity : m_world->getEntities()) {
            DabozzEngine::ECS::Transform* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(entity);
            DabozzEngine::ECS::FirstPersonController* controller = m_world->getComponent<DabozzEngine::ECS::FirstPersonController>(entity);
            
            if (transform && controller) {
                float pitchRad = qDegreesToRadians(controller->pitch);
                float yawRad = qDegreesToRadians(controller->yaw);
                
                QVector3D forward;
                forward.setX(-sin(yawRad) * cos(pitchRad));
                forward.setY(sin(pitchRad));
                forward.setZ(-cos(yawRad) * cos(pitchRad));
                forward.normalize();
                
                QVector3D right = QVector3D::crossProduct(forward, QVector3D(0.0f, 1.0f, 0.0f)).normalized();
                QVector3D up = QVector3D::crossProduct(right, forward).normalized();
                
                if (controller->moveForward) {
                    transform->position += forward * controller->moveSpeed * deltaTime;
                }
                if (controller->moveBackward) {
                    transform->position -= forward * controller->moveSpeed * deltaTime;
                }
                if (controller->moveRight) {
                    transform->position += right * controller->moveSpeed * deltaTime;
                }
                if (controller->moveLeft) {
                    transform->position -= right * controller->moveSpeed * deltaTime;
                }
                if (controller->moveUp) {
                    transform->position += QVector3D(0.0f, 1.0f, 0.0f) * controller->moveSpeed * deltaTime;
                }
                if (controller->moveDown) {
                    transform->position -= QVector3D(0.0f, 1.0f, 0.0f) * controller->moveSpeed * deltaTime;
                }
                
                m_view.setToIdentity();
                m_view.rotate(-controller->pitch, 1.0f, 0.0f, 0.0f);
                m_view.rotate(-controller->yaw, 0.0f, 1.0f, 0.0f);
                m_view.translate(-transform->position);
                m_cameraPosition = transform->position;
                m_cameraForward = forward;
                m_cameraRight = right;
                m_cameraUp = up;
                m_hasCamera = true;
                cameraFound = true;
                break;
            }
        }

        if (!cameraFound) {
            m_hasCamera = false;
            m_view.setToIdentity();
            m_view.translate(0.0f, 0.0f, -3.0f);
        }
    }
    
    update();
}


void OpenGLRenderer::setupGrid()
{
    const int gridSize = 20;
    const float gridSpacing = 1.0f;
    
    std::vector<float> gridVertices;
    
    // Lines along X axis
    for (int i = -gridSize; i <= gridSize; ++i) {
        float pos = i * gridSpacing;
        // Line start
        gridVertices.push_back(pos);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(-gridSize * gridSpacing);
        // Line end
        gridVertices.push_back(pos);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(gridSize * gridSpacing);
    }
    
    // Lines along Z axis
    for (int i = -gridSize; i <= gridSize; ++i) {
        float pos = i * gridSpacing;
        // Line start
        gridVertices.push_back(-gridSize * gridSpacing);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(pos);
        // Line end
        gridVertices.push_back(gridSize * gridSpacing);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(pos);
    }
    
    glGenVertexArrays(1, &m_gridVAO);
    glGenBuffers(1, &m_gridVBO);
    
    glBindVertexArray(m_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void OpenGLRenderer::renderGrid()
{
    if (!m_shaderProgram || !m_shaderProgram->isLinked() || m_gridVAO == 0) return;

    m_shaderProgram->bind();
    
    QMatrix4x4 model;
    model.setToIdentity();
    
    m_shaderProgram->setUniformValue("model", model);
    m_shaderProgram->setUniformValue("view", m_view);
    m_shaderProgram->setUniformValue("projection", m_projection);
    m_shaderProgram->setUniformValue("useTexture", 0);
    m_shaderProgram->setUniformValue("objectColor", QVector3D(0.3f, 0.3f, 0.3f));

    glBindVertexArray(m_gridVAO);
    glDrawArrays(GL_LINES, 0, 164); // (20*2+1)*2*2 = 164 vertices
    glBindVertexArray(0);
    
    m_shaderProgram->release();
}
