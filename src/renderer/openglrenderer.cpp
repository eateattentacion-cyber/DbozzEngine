#include "renderer/openglrenderer.h"
#include "ecs/components/transform.h"
#include "ecs/components/mesh.h"
#include "ecs/components/firstpersoncontroller.h"
#include "ecs/components/boxcollider.h"
#include "ecs/components/spherecollider.h"
#include "ecs/components/hierarchy.h"
#include "ecs/components/animator.h"
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
    , m_skyboxShader(nullptr)
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
    , m_gridVAO(0)
    , m_gridVBO(0)
    , m_arrowVAO(0)
    , m_arrowVBO(0)
    , m_arrowEBO(0)
    , m_skyboxVAO(0)
    , m_skyboxVBO(0)
    , m_rotationAngle(0.0f)
    , m_world(nullptr)
    , m_selectedEntity(DabozzEngine::ECS::INVALID_ENTITY)
    , m_draggingGizmo(false)
    , m_rightMouseDown(false)
    , m_activeAxis(GizmoAxis::None)
    , m_hoverAxis(GizmoAxis::None)
    , m_dragStartAxisValue(0.0f)
    , m_dragStartPosition(0.0f, 0.0f, 0.0f)
    , m_dragStartScale(1.0f, 1.0f, 1.0f)
    , m_dragStartRotation()
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
    delete m_skyboxShader;
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    glDeleteVertexArrays(1, &m_gridVAO);
    glDeleteBuffers(1, &m_gridVBO);
    glDeleteVertexArrays(1, &m_arrowVAO);
    glDeleteBuffers(1, &m_arrowVBO);
    glDeleteBuffers(1, &m_arrowEBO);
    glDeleteVertexArrays(1, &m_skyboxVAO);
    glDeleteBuffers(1, &m_skyboxVBO);
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
    DEBUG_LOG << "Setting up skybox..." << std::endl;
    setupSkybox();
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
                    
                    // Upload bone data if mesh has animation
                    if (mesh->hasAnimation) {
                        glGenBuffers(1, &mesh->boneVBO);
                        glGenBuffers(1, &mesh->weightVBO);
                        
                        // Bone IDs
                        glBindBuffer(GL_ARRAY_BUFFER, mesh->boneVBO);
                        glBufferData(GL_ARRAY_BUFFER, mesh->boneIds.size() * sizeof(int), mesh->boneIds.data(), GL_STATIC_DRAW);
                        glVertexAttribIPointer(3, 4, GL_INT, 4 * sizeof(int), (void*)0);
                        glEnableVertexAttribArray(3);
                        
                        // Bone Weights
                        glBindBuffer(GL_ARRAY_BUFFER, mesh->weightVBO);
                        glBufferData(GL_ARRAY_BUFFER, mesh->boneWeights.size() * sizeof(float), mesh->boneWeights.data(), GL_STATIC_DRAW);
                        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
                        glEnableVertexAttribArray(4);
                        
                        DEBUG_LOG << "Uploaded bone data for animated mesh" << std::endl;
                    }
                    
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
                            DEBUG_LOG << "Texture loaded successfully: " << width << "x" << height << " channels: " << channels << std::endl;
                            
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                            glGenerateMipmap(GL_TEXTURE_2D);
                            
                            GLenum err = glGetError();
                            if (err != GL_NO_ERROR) {
                                DEBUG_LOG << "OpenGL ERROR during texture upload: " << err << std::endl;
                                mesh->hasTexture = false;
                                glDeleteTextures(1, &mesh->textureID);
                                mesh->textureID = 0;
                            } else {
                                DEBUG_LOG << "SUCCESS: Texture uploaded to GPU (ID: " << mesh->textureID << ")" << std::endl;
                            }
                            
                            // Free STB allocated memory (but not raw embedded data)
                            if (mesh->texturePath != "embedded_raw" && imageData) {
                                stbi_image_free(imageData);
                            }
                        } else {
                            DEBUG_LOG << "=== TEXTURE LOAD FAILED ===" << std::endl;
                            DEBUG_LOG << "Texture path: " << mesh->texturePath << std::endl;
                            DEBUG_LOG << "Has embedded data: " << (!mesh->embeddedTextureData.empty() ? "yes" : "no") << std::endl;
                            if (!mesh->embeddedTextureData.empty()) {
                                DEBUG_LOG << "Embedded data size: " << mesh->embeddedTextureData.size() << " bytes" << std::endl;
                            }
                            if (mesh->texturePath != "embedded_raw" && mesh->texturePath != "embedded_compressed") {
                                DEBUG_LOG << "STB Error: " << stbi_failure_reason() << std::endl;
                            } else if (mesh->texturePath == "embedded_compressed") {
                                DEBUG_LOG << "STB Error (compressed embedded): " << stbi_failure_reason() << std::endl;
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
                    // Calculate world transform by multiplying parent transforms
                    QMatrix4x4 modelMatrix = getWorldTransform(entity);
                    
                    m_shaderProgram->setUniformValue("model", modelMatrix);
                    m_shaderProgram->setUniformValue("view", m_view);
                    m_shaderProgram->setUniformValue("projection", m_projection);
                    
                    m_shaderProgram->setUniformValue("lightPos", QVector3D(2.0f, 2.0f, 2.0f));
                    m_shaderProgram->setUniformValue("viewPos", viewPos);
                    m_shaderProgram->setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
                    m_shaderProgram->setUniformValue("roughness", 0.5f);
                    m_shaderProgram->setUniformValue("metallic", 0.0f);
                    m_shaderProgram->setUniformValue("specular", 0.5f);
                    
                    // Check for animator component (on this entity or parent)
                    DabozzEngine::ECS::Animator* animator = m_world->getComponent<DabozzEngine::ECS::Animator>(entity);
                    
                    // If not found, check parent
                    if (!animator) {
                        DabozzEngine::ECS::Hierarchy* hierarchy = m_world->getComponent<DabozzEngine::ECS::Hierarchy>(entity);
                        if (hierarchy && hierarchy->parent != 0) {
                            animator = m_world->getComponent<DabozzEngine::ECS::Animator>(hierarchy->parent);
                        }
                    }
                    
                    if (m_animationEnabled && animator && mesh->hasAnimation) {
                        // Upload bone matrices to shader (convert GLM to Qt)
                        // Both GLM and QMatrix4x4 use column-major storage
                        // GLM: glmMat[col][row], QMatrix4x4: qtMat(row, col)
                        for (size_t i = 0; i < animator->boneMatrices.size() && i < 100; i++) {
                            QString uniformName = QString("finalBonesMatrices[%1]").arg(i);
                            
                            const glm::mat4& glmMat = animator->boneMatrices[i];
                            QMatrix4x4 qtMat;
                            // GLM stores column-major: glmMat[col][row]
                            // QMatrix4x4 operator(row, col) also expects column-major
                            for (int col = 0; col < 4; col++) {
                                for (int row = 0; row < 4; row++) {
                                    qtMat(row, col) = glmMat[col][row];
                                }
                            }
                            
                            m_shaderProgram->setUniformValue(uniformName.toStdString().c_str(), qtMat);
                        }
                        m_shaderProgram->setUniformValue("hasAnimation", 1);
                    } else {
                        m_shaderProgram->setUniformValue("hasAnimation", 0);
                    }
                    
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
    
    // Render skybox first
    renderSkybox();
    
    // Render collider debug wireframes (green)
    renderColliders();

    // Render grid
    renderGrid();

    // Render transform gizmo for selected entity
    renderGizmo();
}

void OpenGLRenderer::setPlayMode(bool playing)
{
    m_playMode = playing;
    if (playing) {
        m_animationTimer->stop();
    } else {
        m_animationTimer->start(16);
        update();
    }
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
    
    // Create cone geometry for arrow tips
    std::vector<float> coneVertices;
    std::vector<unsigned int> coneIndices;
    const int segments = 16;
    const float radius = 0.5f;
    const float height = 1.0f;
    
    // Tip vertex
    coneVertices.insert(coneVertices.end(), {0.0f, height, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f});
    
    // Base vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * 2.0f * M_PI;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        coneVertices.insert(coneVertices.end(), {x, 0.0f, z, 0.0f, -1.0f, 0.0f, (float)i / segments, 0.0f});
    }
    
    // Side triangles
    for (int i = 0; i < segments; ++i) {
        coneIndices.push_back(0);
        coneIndices.push_back(i + 1);
        coneIndices.push_back(i + 2);
    }
    
    // Base center
    int baseCenter = coneVertices.size() / 8;
    coneVertices.insert(coneVertices.end(), {0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.5f, 0.5f});
    
    // Base triangles
    for (int i = 0; i < segments; ++i) {
        coneIndices.push_back(baseCenter);
        coneIndices.push_back(i + 2);
        coneIndices.push_back(i + 1);
    }
    
    glGenVertexArrays(1, &m_arrowVAO);
    glGenBuffers(1, &m_arrowVBO);
    glGenBuffers(1, &m_arrowEBO);
    
    glBindVertexArray(m_arrowVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_arrowVBO);
    glBufferData(GL_ARRAY_BUFFER, coneVertices.size() * sizeof(float), coneVertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_arrowEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, coneIndices.size() * sizeof(unsigned int), coneIndices.data(), GL_STATIC_DRAW);
    
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
    
    // Gizmo mode switching (W = translate, E = rotate, R = scale)
    if (!m_playMode) {
        switch (event->key()) {
            case Qt::Key_W:
                if (!event->isAutoRepeat()) {
                    m_gizmoMode = GizmoMode::Translate;
                    update();
                }
                break;
            case Qt::Key_E:
                if (!event->isAutoRepeat()) {
                    m_gizmoMode = GizmoMode::Rotate;
                    update();
                }
                break;
            case Qt::Key_R:
                if (!event->isAutoRepeat()) {
                    m_gizmoMode = GizmoMode::Scale;
                    update();
                }
                break;
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
            m_dragStartScale = transform->scale;
            m_dragStartRotation = transform->rotation;
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
            
            switch (m_gizmoMode) {
                case GizmoMode::Translate:
                    transform->position = m_dragStartPosition + axisDirection(m_activeAxis) * delta;
                    break;
                    
                case GizmoMode::Scale: {
                    // Scale based on drag distance
                    const float scaleFactor = 1.0f + delta * 0.5f;
                    QVector3D scaleChange = axisDirection(m_activeAxis) * (scaleFactor - 1.0f);
                    if (m_activeAxis == GizmoAxis::Center) {
                        // Uniform scale
                        transform->scale = m_dragStartScale * scaleFactor;
                    } else {
                        // Per-axis scale
                        transform->scale = m_dragStartScale + scaleChange;
                    }
                    break;
                }
                    
                case GizmoMode::Rotate: {
                    // Rotate around axis
                    const float rotationDegrees = delta * 50.0f;
                    QVector3D axis = axisDirection(m_activeAxis);
                    QQuaternion rotation = QQuaternion::fromAxisAndAngle(axis, rotationDegrees);
                    transform->rotation = rotation * m_dragStartRotation;
                    break;
                }
            }
            
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
	if (!m_world) {
		return;
	}

	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(3.0f);

	m_shaderProgram->bind();

	for (DabozzEngine::ECS::EntityID entity : m_world->getEntities()) {
		DabozzEngine::ECS::Transform *transform = m_world->getComponent<DabozzEngine::ECS::Transform>(entity);
		DabozzEngine::ECS::BoxCollider *boxCollider = m_world->getComponent<DabozzEngine::ECS::BoxCollider>(entity);
		DabozzEngine::ECS::SphereCollider *sphereCollider = m_world->getComponent<DabozzEngine::ECS::SphereCollider>(entity);

		if (transform && (boxCollider || sphereCollider)) {
			QMatrix4x4 modelMatrix = getWorldTransform(entity);

			if (boxCollider) {
				QMatrix4x4 scaleMatrix;
				scaleMatrix.scale(boxCollider->size);
				modelMatrix = modelMatrix * scaleMatrix;
				m_shaderProgram->setUniformValue("objectColor", QVector3D(0.0f, 1.0f, 0.0f));
			} else if (sphereCollider) {
				m_shaderProgram->setUniformValue("objectColor", QVector3D(0.0f, 0.8f, 1.0f));
			}

			m_shaderProgram->setUniformValue("model", modelMatrix);
			m_shaderProgram->setUniformValue("view", m_view);
			m_shaderProgram->setUniformValue("projection", m_projection);
			m_shaderProgram->setUniformValue("useTexture", 0);

			glBindVertexArray(m_vao);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glLineWidth(1.0f);
	glEnable(GL_DEPTH_TEST);
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

    const QVector3D viewPos = m_hasCamera ? m_cameraPosition : QVector3D(0.0f, 0.0f, 3.0f);
    float distanceToCamera = (transform->position - viewPos).length();
    float gizmoScale = distanceToCamera * 0.15f;

    glDisable(GL_DEPTH_TEST);

    switch (m_gizmoMode) {
        case GizmoMode::Translate:
            renderTranslateGizmo(transform->position, gizmoScale);
            break;
        case GizmoMode::Rotate:
            renderRotateGizmo(transform->position, gizmoScale);
            break;
        case GizmoMode::Scale:
            renderScaleGizmo(transform->position, gizmoScale);
            break;
    }

    glEnable(GL_DEPTH_TEST);
}

void OpenGLRenderer::renderTranslateGizmo(const QVector3D& position, float scale)
{
    m_shaderProgram->bind();
    m_shaderProgram->setUniformValue("view", m_view);
    m_shaderProgram->setUniformValue("projection", m_projection);
    const QVector3D viewPos = m_hasCamera ? m_cameraPosition : QVector3D(0.0f, 0.0f, 3.0f);
    m_shaderProgram->setUniformValue("lightPos", QVector3D(2.0f, 2.0f, 2.0f));
    m_shaderProgram->setUniformValue("viewPos", viewPos);
    m_shaderProgram->setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_shaderProgram->setUniformValue("useTexture", 0);

    glBindVertexArray(m_vao);

    auto axisColor = [this](GizmoAxis axis, const QVector3D& baseColor) {
        if (axis == m_activeAxis || axis == m_hoverAxis) {
            return QVector3D(1.0f, 1.0f, 0.0f);
        }
        return baseColor;
    };

    auto drawAxis = [this, &position, scale](GizmoAxis axis, const QVector3D& axisScale, const QVector3D& color) {
        const QVector3D direction = axisDirection(axis);
        const QVector3D offset = direction * (kGizmoAxisLength * 0.5f * scale);
        QMatrix4x4 model;
        model.translate(position + offset);
        model.scale(axisScale * scale);
        m_shaderProgram->setUniformValue("model", model);
        m_shaderProgram->setUniformValue("objectColor", color);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    };

    auto drawArrow = [this, &position, scale](GizmoAxis axis, const QVector3D& color) {
        const QVector3D direction = axisDirection(axis);
        const QVector3D offset = direction * (kGizmoAxisLength * scale);
        QMatrix4x4 model;
        model.translate(position + offset);
        if (axis == GizmoAxis::X) {
            model.rotate(90, 0, 0, 1);
        } else if (axis == GizmoAxis::Z) {
            model.rotate(90, 1, 0, 0);
        }
        model.scale(kGizmoArrowSize * scale);
        m_shaderProgram->setUniformValue("model", model);
        m_shaderProgram->setUniformValue("objectColor", color);
        
        glBindVertexArray(m_arrowVAO);
        glDrawElements(GL_TRIANGLES, 48, GL_UNSIGNED_INT, 0);
        glBindVertexArray(m_vao);
    };

    // Draw axes
    drawAxis(GizmoAxis::X, QVector3D(kGizmoAxisLength, kGizmoAxisThickness, kGizmoAxisThickness),
             axisColor(GizmoAxis::X, QVector3D(0.8f, 0.0f, 0.0f)));
    drawAxis(GizmoAxis::Y, QVector3D(kGizmoAxisThickness, kGizmoAxisLength, kGizmoAxisThickness),
             axisColor(GizmoAxis::Y, QVector3D(0.0f, 0.8f, 0.0f)));
    drawAxis(GizmoAxis::Z, QVector3D(kGizmoAxisThickness, kGizmoAxisThickness, kGizmoAxisLength),
             axisColor(GizmoAxis::Z, QVector3D(0.0f, 0.4f, 0.8f)));

    // Draw arrow tips
    drawArrow(GizmoAxis::X, axisColor(GizmoAxis::X, QVector3D(1.0f, 0.0f, 0.0f)));
    drawArrow(GizmoAxis::Y, axisColor(GizmoAxis::Y, QVector3D(0.0f, 1.0f, 0.0f)));
    drawArrow(GizmoAxis::Z, axisColor(GizmoAxis::Z, QVector3D(0.0f, 0.5f, 1.0f)));

    // Draw plane handles (semi-transparent)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto drawPlane = [this, &position, scale](GizmoAxis axis, const QVector3D& color) {
        QMatrix4x4 model;
        model.translate(position);
        if (axis == GizmoAxis::XY) {
            model.translate(kGizmoPlaneSize * scale, kGizmoPlaneSize * scale, 0);
        } else if (axis == GizmoAxis::YZ) {
            model.translate(0, kGizmoPlaneSize * scale, kGizmoPlaneSize * scale);
            model.rotate(90, 0, 1, 0);
        } else if (axis == GizmoAxis::XZ) {
            model.translate(kGizmoPlaneSize * scale, 0, kGizmoPlaneSize * scale);
            model.rotate(90, 1, 0, 0);
        }
        model.scale(kGizmoPlaneSize * scale, kGizmoPlaneSize * scale, 0.01f * scale);
        m_shaderProgram->setUniformValue("model", model);
        QVector3D planeColor = (axis == m_activeAxis || axis == m_hoverAxis) ? QVector3D(1.0f, 1.0f, 0.0f) : color;
        m_shaderProgram->setUniformValue("objectColor", QVector3D(planeColor.x(), planeColor.y(), planeColor.z()));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    };

    drawPlane(GizmoAxis::XY, QVector3D(0.5f, 0.5f, 0.0f));
    drawPlane(GizmoAxis::YZ, QVector3D(0.0f, 0.5f, 0.5f));
    drawPlane(GizmoAxis::XZ, QVector3D(0.5f, 0.0f, 0.5f));

    glDisable(GL_BLEND);
    glBindVertexArray(0);
    m_shaderProgram->release();
}

void OpenGLRenderer::renderRotateGizmo(const QVector3D& position, float scale)
{
    m_shaderProgram->bind();
    m_shaderProgram->setUniformValue("view", m_view);
    m_shaderProgram->setUniformValue("projection", m_projection);
    const QVector3D viewPos = m_hasCamera ? m_cameraPosition : QVector3D(0.0f, 0.0f, 3.0f);
    m_shaderProgram->setUniformValue("lightPos", QVector3D(2.0f, 2.0f, 2.0f));
    m_shaderProgram->setUniformValue("viewPos", viewPos);
    m_shaderProgram->setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_shaderProgram->setUniformValue("useTexture", 0);

    glBindVertexArray(m_vao);
    glLineWidth(3.0f);

    auto drawCircle = [this, &position, scale](GizmoAxis axis, const QVector3D& color) {
        const int segments = 64;
        const float radius = kGizmoAxisLength * scale;
        const float thickness = kGizmoAxisThickness * 3.0f * scale;
        
        QVector3D finalColor = (axis == m_activeAxis || axis == m_hoverAxis) ? QVector3D(1.0f, 1.0f, 0.0f) : color;
        
        for (int i = 0; i < segments; ++i) {
            float angle1 = (float)i / segments * 2.0f * M_PI;
            float angle2 = (float)(i + 1) / segments * 2.0f * M_PI;
            
            QVector3D p1, p2;
            if (axis == GizmoAxis::X) {
                p1 = QVector3D(0, cos(angle1) * radius, sin(angle1) * radius);
                p2 = QVector3D(0, cos(angle2) * radius, sin(angle2) * radius);
            } else if (axis == GizmoAxis::Y) {
                p1 = QVector3D(cos(angle1) * radius, 0, sin(angle1) * radius);
                p2 = QVector3D(cos(angle2) * radius, 0, sin(angle2) * radius);
            } else {
                p1 = QVector3D(cos(angle1) * radius, sin(angle1) * radius, 0);
                p2 = QVector3D(cos(angle2) * radius, sin(angle2) * radius, 0);
            }
            
            // Draw a small cylinder segment
            QVector3D mid = (p1 + p2) * 0.5f;
            QVector3D dir = (p2 - p1).normalized();
            float length = (p2 - p1).length();
            
            QMatrix4x4 model;
            model.translate(position + mid);
            
            // Orient the cylinder along the segment
            QVector3D up(0, 1, 0);
            if (qAbs(QVector3D::dotProduct(dir, up)) > 0.99f) {
                up = QVector3D(1, 0, 0);
            }
            QVector3D right = QVector3D::crossProduct(dir, up).normalized();
            up = QVector3D::crossProduct(right, dir).normalized();
            
            QMatrix4x4 rotation;
            rotation.setColumn(0, QVector4D(right, 0));
            rotation.setColumn(1, QVector4D(up, 0));
            rotation.setColumn(2, QVector4D(dir, 0));
            rotation.setColumn(3, QVector4D(0, 0, 0, 1));
            
            model = model * rotation;
            model.scale(thickness, thickness, length);
            
            m_shaderProgram->setUniformValue("model", model);
            m_shaderProgram->setUniformValue("objectColor", finalColor);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
    };

    drawCircle(GizmoAxis::X, QVector3D(1.0f, 0.0f, 0.0f));
    drawCircle(GizmoAxis::Y, QVector3D(0.0f, 1.0f, 0.0f));
    drawCircle(GizmoAxis::Z, QVector3D(0.0f, 0.5f, 1.0f));

    glLineWidth(1.0f);
    glBindVertexArray(0);
    m_shaderProgram->release();
}

void OpenGLRenderer::renderScaleGizmo(const QVector3D& position, float scale)
{
    m_shaderProgram->bind();
    m_shaderProgram->setUniformValue("view", m_view);
    m_shaderProgram->setUniformValue("projection", m_projection);
    const QVector3D viewPos = m_hasCamera ? m_cameraPosition : QVector3D(0.0f, 0.0f, 3.0f);
    m_shaderProgram->setUniformValue("lightPos", QVector3D(2.0f, 2.0f, 2.0f));
    m_shaderProgram->setUniformValue("viewPos", viewPos);
    m_shaderProgram->setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_shaderProgram->setUniformValue("useTexture", 0);

    glBindVertexArray(m_vao);

    auto axisColor = [this](GizmoAxis axis, const QVector3D& baseColor) {
        if (axis == m_activeAxis || axis == m_hoverAxis) {
            return QVector3D(1.0f, 1.0f, 0.0f);
        }
        return baseColor;
    };

    auto drawAxis = [this, &position, scale](GizmoAxis axis, const QVector3D& axisScale, const QVector3D& color) {
        const QVector3D direction = axisDirection(axis);
        const QVector3D offset = direction * (kGizmoAxisLength * 0.5f * scale);
        QMatrix4x4 model;
        model.translate(position + offset);
        model.scale(axisScale * scale);
        m_shaderProgram->setUniformValue("model", model);
        m_shaderProgram->setUniformValue("objectColor", color);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    };

    auto drawCube = [this, &position, scale](GizmoAxis axis, const QVector3D& color) {
        const QVector3D direction = axisDirection(axis);
        const QVector3D offset = direction * (kGizmoAxisLength * scale);
        QMatrix4x4 model;
        model.translate(position + offset);
        model.scale(kGizmoArrowSize * scale);
        m_shaderProgram->setUniformValue("model", model);
        m_shaderProgram->setUniformValue("objectColor", color);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    };

    // Draw axes
    drawAxis(GizmoAxis::X, QVector3D(kGizmoAxisLength, kGizmoAxisThickness, kGizmoAxisThickness),
             axisColor(GizmoAxis::X, QVector3D(0.8f, 0.0f, 0.0f)));
    drawAxis(GizmoAxis::Y, QVector3D(kGizmoAxisThickness, kGizmoAxisLength, kGizmoAxisThickness),
             axisColor(GizmoAxis::Y, QVector3D(0.0f, 0.8f, 0.0f)));
    drawAxis(GizmoAxis::Z, QVector3D(kGizmoAxisThickness, kGizmoAxisThickness, kGizmoAxisLength),
             axisColor(GizmoAxis::Z, QVector3D(0.0f, 0.4f, 0.8f)));

    // Draw cube handles
    drawCube(GizmoAxis::X, axisColor(GizmoAxis::X, QVector3D(1.0f, 0.0f, 0.0f)));
    drawCube(GizmoAxis::Y, axisColor(GizmoAxis::Y, QVector3D(0.0f, 1.0f, 0.0f)));
    drawCube(GizmoAxis::Z, axisColor(GizmoAxis::Z, QVector3D(0.0f, 0.5f, 1.0f)));

    // Center cube for uniform scale
    QMatrix4x4 model;
    model.translate(position);
    model.scale(kGizmoArrowSize * 0.7f * scale);
    m_shaderProgram->setUniformValue("model", model);
    m_shaderProgram->setUniformValue("objectColor", axisColor(GizmoAxis::Center, QVector3D(0.8f, 0.8f, 0.8f)));
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    m_shaderProgram->release();
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
    const QVector3D viewPos = m_hasCamera ? m_cameraPosition : QVector3D(0.0f, 0.0f, 3.0f);
    float distanceToCamera = (transform->position - viewPos).length();
    float gizmoScale = distanceToCamera * 0.15f;
    
    const float halfPick = kGizmoPickThickness * 0.5f * gizmoScale;
    const float halfLength = kGizmoAxisLength * 0.5f * gizmoScale;

    struct HitResult {
        GizmoAxis axis;
        float t;
    };

    HitResult bestHit { GizmoAxis::None, std::numeric_limits<float>::max() };

    if (m_gizmoMode == GizmoMode::Rotate) {
        // Pick rotation circles
        const float radius = kGizmoAxisLength * gizmoScale;
        const float thickness = kGizmoPickThickness * gizmoScale;
        
        auto testCircle = [&](GizmoAxis axis) {
            // Test distance from ray to circle
            QVector3D planeNormal = axisDirection(axis);
            float denom = QVector3D::dotProduct(planeNormal, ray.direction);
            if (qAbs(denom) > 0.0001f) {
                float t = QVector3D::dotProduct(transform->position - ray.origin, planeNormal) / denom;
                if (t > 0) {
                    QVector3D hitPoint = ray.origin + ray.direction * t;
                    float dist = (hitPoint - transform->position).length();
                    if (qAbs(dist - radius) < thickness && t < bestHit.t) {
                        bestHit.axis = axis;
                        bestHit.t = t;
                    }
                }
            }
        };
        
        testCircle(GizmoAxis::X);
        testCircle(GizmoAxis::Y);
        testCircle(GizmoAxis::Z);
        
    } else {
        // Pick translate/scale axes
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
        
        // Test plane handles for translate mode
        if (m_gizmoMode == GizmoMode::Translate) {
            const float planeSize = kGizmoPlaneSize * gizmoScale;
            auto testPlane = [&](GizmoAxis axis, const QVector3D& planeNormal) {
                float denom = QVector3D::dotProduct(planeNormal, ray.direction);
                if (qAbs(denom) > 0.0001f) {
                    float t = QVector3D::dotProduct(transform->position - ray.origin, planeNormal) / denom;
                    if (t > 0) {
                        QVector3D hitPoint = ray.origin + ray.direction * t;
                        QVector3D localHit = hitPoint - transform->position;
                        
                        if (axis == GizmoAxis::XY) {
                            if (localHit.x() > 0 && localHit.x() < planeSize * 2 &&
                                localHit.y() > 0 && localHit.y() < planeSize * 2 &&
                                qAbs(localHit.z()) < 0.1f && t < bestHit.t) {
                                bestHit.axis = axis;
                                bestHit.t = t;
                            }
                        }
                    }
                }
            };
            
            testPlane(GizmoAxis::XY, QVector3D(0, 0, 1));
            testPlane(GizmoAxis::YZ, QVector3D(1, 0, 0));
            testPlane(GizmoAxis::XZ, QVector3D(0, 1, 0));
        }
        
        // Test center cube for scale mode
        if (m_gizmoMode == GizmoMode::Scale) {
            const float centerSize = kGizmoArrowSize * 0.7f * gizmoScale;
            const QVector3D boxMin = transform->position - QVector3D(centerSize, centerSize, centerSize);
            const QVector3D boxMax = transform->position + QVector3D(centerSize, centerSize, centerSize);
            float t = 0.0f;
            if (intersectRayAabb(ray.origin, ray.direction, boxMin, boxMax, t)) {
                if (t < bestHit.t) {
                    bestHit.axis = GizmoAxis::Center;
                    bestHit.t = t;
                }
            }
        }
    }

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

QMatrix4x4 OpenGLRenderer::getWorldTransform(DabozzEngine::ECS::EntityID entity) const
{
    if (!m_world) {
        return QMatrix4x4();
    }
    
    DabozzEngine::ECS::Transform* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(entity);
    if (!transform) {
        return QMatrix4x4();
    }
    
    // Get local transform
    QMatrix4x4 localMatrix = transform->getModelMatrix();
    
    // Check if entity has a parent
    DabozzEngine::ECS::Hierarchy* hierarchy = m_world->getComponent<DabozzEngine::ECS::Hierarchy>(entity);
    if (hierarchy && hierarchy->parent != 0) {
        // Recursively get parent's world transform and multiply
        QMatrix4x4 parentMatrix = getWorldTransform(hierarchy->parent);
        return parentMatrix * localMatrix;
    }
    
    // No parent, return local transform
    return localMatrix;
}


void OpenGLRenderer::setupSkybox()
{
    m_skyboxShader = new QOpenGLShaderProgram();
    m_skyboxShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/skybox_vertex.glsl");
    m_skyboxShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/skybox_fragment.glsl");
    m_skyboxShader->link();
    
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    
    glGenVertexArrays(1, &m_skyboxVAO);
    glGenBuffers(1, &m_skyboxVBO);
    glBindVertexArray(m_skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void OpenGLRenderer::renderSkybox()
{
    glDepthFunc(GL_LEQUAL);
    m_skyboxShader->bind();
    m_skyboxShader->setUniformValue("view", m_view);
    m_skyboxShader->setUniformValue("projection", m_projection);
    m_skyboxShader->setUniformValue("time", (float)m_rotationAngle);
    m_skyboxShader->setUniformValue("sun_direction", QVector3D(0.5f, 0.5f, -0.5f));
    glBindVertexArray(m_skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    m_skyboxShader->release();
}
