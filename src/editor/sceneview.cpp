#include "editor/sceneview.h"
#include <QMouseEvent>

SceneView::SceneView(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

SceneView::~SceneView()
{
}

void SceneView::setWorld(DabozzEngine::ECS::World* world)
{
    m_renderer->setWorld(world);
}

void SceneView::setSelectedEntity(DabozzEngine::ECS::EntityID entity)
{
    if (m_renderer) {
        m_renderer->setSelectedEntity(entity);
    }
}

void SceneView::setModeLabel(const QString& text)
{
    m_modeLabel->setText(text);
}

void SceneView::mousePressEvent(QMouseEvent* event)
{
    m_renderer->setFocus();
    QWidget::mousePressEvent(event);
}

void SceneView::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_toolbarLayout = new QHBoxLayout();
    m_toolbarLayout->setContentsMargins(2, 2, 2, 2);
    m_toolbarLayout->setSpacing(2);

    m_playButton = new QPushButton("Play");
    m_pauseButton = new QPushButton("Pause");
    m_stopButton = new QPushButton("Stop");
    m_modeLabel = new QLabel("Scene View - Edit Mode");

    QString btnStyle = "QPushButton { padding: 2px 8px; }";
    m_playButton->setStyleSheet(btnStyle);
    m_pauseButton->setStyleSheet(btnStyle);
    m_stopButton->setStyleSheet(btnStyle);
    m_modeLabel->setStyleSheet("font-weight: bold; padding-left: 4px;");

    m_toolbarLayout->addWidget(m_playButton);
    m_toolbarLayout->addWidget(m_pauseButton);
    m_toolbarLayout->addWidget(m_stopButton);
    m_toolbarLayout->addStretch();
    m_toolbarLayout->addWidget(m_modeLabel);

    m_renderer = new OpenGLRenderer(this);
    m_renderer->setMinimumSize(400, 300);

    m_mainLayout->addLayout(m_toolbarLayout);
    m_mainLayout->addWidget(m_renderer, 1);
    
    setLayout(m_mainLayout);
    
    // Connect play mode buttons
    connect(m_playButton, &QPushButton::clicked, this, &SceneView::playClicked);
    connect(m_pauseButton, &QPushButton::clicked, this, &SceneView::pauseClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &SceneView::stopClicked);
}