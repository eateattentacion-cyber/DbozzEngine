#include "editor/componentinspector.h"
#include "ecs/components/transform.h"
#include "ecs/components/name.h"
#include "ecs/components/rigidbody.h"
#include "ecs/components/boxcollider.h"
#include "ecs/components/spherecollider.h"
#include "ecs/components/mesh.h"
#include "ecs/components/firstpersoncontroller.h"
#include "ecs/components/animator.h"
#include "ecs/components/hierarchy.h"
#include "ecs/components/audiosource.h"
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QMenu>
#include <QDoubleValidator>
#include <QSignalBlocker>
#include <typeinfo>
#include <QHBoxLayout>
#include <QFileDialog>
#include "editor/undostack.h"

ComponentInspector::ComponentInspector(QWidget* parent)
    : QWidget(parent)
    , m_world(nullptr)
    , m_selectedEntity(0)
    , m_componentsWidget(nullptr)
{
    setupUI();
}

ComponentInspector::~ComponentInspector()
{
}

void ComponentInspector::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    QLabel* titleLabel = new QLabel("Inspector");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #a0c4ff; margin-bottom: 8px; padding: 4px 0;");
    m_mainLayout->addWidget(titleLabel);
    
    createTransformSection();
    createPropertiesSection();
    createComponentsSection();
    
    m_addComponentButton = new QPushButton("+ Add Component");
    m_addComponentButton->setStyleSheet(
        "QPushButton { background-color: #2563eb; color: white; border: none; border-radius: 4px; padding: 8px 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #1d4ed8; }"
        "QPushButton:disabled { background-color: #333; color: #666; }"
    );
    connect(m_addComponentButton, &QPushButton::clicked, this, &ComponentInspector::onAddComponentClicked);
    m_mainLayout->addWidget(m_addComponentButton);
    m_mainLayout->addStretch();
    
    setLayout(m_mainLayout);
    updateUI();
}

void ComponentInspector::createTransformSection()
{
    QGroupBox* transformGroup = new QGroupBox("Transform");
    m_transformLayout = new QFormLayout(transformGroup);

    auto createFloatField = [this](const QString& value) {
        QLineEdit* field = new QLineEdit(value);
        auto* validator = new QDoubleValidator(-99999.0, 99999.0, 4, field);
        validator->setNotation(QDoubleValidator::StandardNotation);
        field->setValidator(validator);
        field->setStyleSheet("QLineEdit[hasAcceptableInput=false] { border: 1px solid red; }");
        connect(field, &QLineEdit::editingFinished, this, &ComponentInspector::onTransformChanged);
        return field;
    };
    
    m_positionX = createFloatField("0.0");
    m_positionY = createFloatField("0.0");
    m_positionZ = createFloatField("0.0");
    
    m_transformLayout->addRow("Position X:", m_positionX);
    m_transformLayout->addRow("Position Y:", m_positionY);
    m_transformLayout->addRow("Position Z:", m_positionZ);
    
    m_rotationX = createFloatField("0.0");
    m_rotationY = createFloatField("0.0");
    m_rotationZ = createFloatField("0.0");
    
    m_transformLayout->addRow("Rotation X:", m_rotationX);
    m_transformLayout->addRow("Rotation Y:", m_rotationY);
    m_transformLayout->addRow("Rotation Z:", m_rotationZ);
    
    m_scaleX = createFloatField("1.0");
    m_scaleY = createFloatField("1.0");
    m_scaleZ = createFloatField("1.0");
    
    m_transformLayout->addRow("Scale X:", m_scaleX);
    m_transformLayout->addRow("Scale Y:", m_scaleY);
    m_transformLayout->addRow("Scale Z:", m_scaleZ);
    
    m_mainLayout->addWidget(transformGroup);
}

void ComponentInspector::createPropertiesSection()
{
    QGroupBox* propertiesGroup = new QGroupBox("Properties");
    m_propertiesLayout = new QFormLayout(propertiesGroup);
    
    m_nameEdit = new QLineEdit("GameObject");
    connect(m_nameEdit, &QLineEdit::editingFinished, this, &ComponentInspector::onNameChanged);
    m_propertiesLayout->addRow("Name:", m_nameEdit);
    
    m_activeCheck = new QCheckBox("Active");
    m_activeCheck->setChecked(true);
    m_activeCheck->setEnabled(false);
    m_activeCheck->setToolTip("Active state not wired yet.");
    m_propertiesLayout->addRow("", m_activeCheck);
    
    m_mainLayout->addWidget(propertiesGroup);
}

void ComponentInspector::createComponentsSection()
{
    m_componentsWidget = new QWidget();
    m_componentsLayout = new QVBoxLayout(m_componentsWidget);
    m_componentsLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->addWidget(m_componentsWidget);
}

void ComponentInspector::setWorld(DabozzEngine::ECS::World* world)
{
    m_world = world;
}

void ComponentInspector::setUndoStack(QUndoStack* undoStack)
{
    m_undoStack = undoStack;
}

void ComponentInspector::setSelectedEntity(DabozzEngine::ECS::EntityID entity)
{
    m_selectedEntity = entity;

    if (m_world && entity != 0) {
        auto* t = m_world->getComponent<DabozzEngine::ECS::Transform>(entity);
        if (t) {
            m_prevPosition = t->position;
            m_prevRotation = t->rotation;
            m_prevScale = t->scale;
        }
        auto* n = m_world->getComponent<DabozzEngine::ECS::Name>(entity);
        if (n) m_prevName = n->name;
    }

    updateUI();
}

void ComponentInspector::clearSelection()
{
    m_selectedEntity = 0;
    updateUI();
}

void ComponentInspector::refreshSelectedEntity(DabozzEngine::ECS::EntityID entity)
{
    if (entity == m_selectedEntity) {
        updateUI();
    }
}

void ComponentInspector::onTransformChanged()
{
    if (!m_world || m_selectedEntity == 0) return;

    DabozzEngine::ECS::Transform* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(m_selectedEntity);
    if (!transform) return;

    if (!m_positionX->hasAcceptableInput() || !m_positionY->hasAcceptableInput() || !m_positionZ->hasAcceptableInput() ||
        !m_rotationX->hasAcceptableInput() || !m_rotationY->hasAcceptableInput() || !m_rotationZ->hasAcceptableInput() ||
        !m_scaleX->hasAcceptableInput() || !m_scaleY->hasAcceptableInput() || !m_scaleZ->hasAcceptableInput())
        return;

    const float positionX = m_positionX->text().toFloat();
    const float positionY = m_positionY->text().toFloat();
    const float positionZ = m_positionZ->text().toFloat();
    const float rotationX = m_rotationX->text().toFloat();
    const float rotationY = m_rotationY->text().toFloat();
    const float rotationZ = m_rotationZ->text().toFloat();
    const float scaleX = m_scaleX->text().toFloat();
    const float scaleY = m_scaleY->text().toFloat();
    const float scaleZ = m_scaleZ->text().toFloat();

    QVector3D newPos(positionX, positionY, positionZ);
    QQuaternion newRot = QQuaternion::fromEulerAngles(rotationX, rotationY, rotationZ);
    QVector3D newScale(scaleX, scaleY, scaleZ);

    if (m_undoStack) {
        auto refreshCb = [this]() { updateUI(); };
        m_undoStack->push(new TransformChangeCommand(
            m_world, m_selectedEntity,
            m_prevPosition, m_prevRotation, m_prevScale,
            newPos, newRot, newScale,
            refreshCb));
    } else {
        transform->position = newPos;
        transform->rotation = newRot;
        transform->scale = newScale;
    }

    m_prevPosition = newPos;
    m_prevRotation = newRot;
    m_prevScale = newScale;
}

void ComponentInspector::onNameChanged()
{
    if (!m_world || m_selectedEntity == 0) return;

    const QString nameText = m_nameEdit->text();

    if (m_undoStack) {
        auto refreshCb = [this]() { updateUI(); };
        m_undoStack->push(new NameChangeCommand(
            m_world, m_selectedEntity,
            m_prevName, nameText,
            refreshCb));
    } else {
        DabozzEngine::ECS::Name* nameComponent = m_world->getComponent<DabozzEngine::ECS::Name>(m_selectedEntity);
        if (nameComponent) {
            nameComponent->name = nameText;
        } else {
            m_world->addComponent<DabozzEngine::ECS::Name>(m_selectedEntity, nameText);
        }
    }

    m_prevName = nameText;
}

void ComponentInspector::onAddComponentClicked()
{
    if (!m_world || m_selectedEntity == 0) return;
    
    QMenu menu(this);
    
    QAction* rigidBodyAction = menu.addAction("RigidBody");
    QAction* boxColliderAction = menu.addAction("BoxCollider");
    QAction* sphereColliderAction = menu.addAction("SphereCollider");
    QAction* meshAction = menu.addAction("Mesh");
    QAction* fpControllerAction = menu.addAction("FirstPersonController");
    QAction* audioSourceAction = menu.addAction("AudioSource");

    rigidBodyAction->setEnabled(!m_world->hasComponent<DabozzEngine::ECS::RigidBody>(m_selectedEntity));
    boxColliderAction->setEnabled(!m_world->hasComponent<DabozzEngine::ECS::BoxCollider>(m_selectedEntity));
    sphereColliderAction->setEnabled(!m_world->hasComponent<DabozzEngine::ECS::SphereCollider>(m_selectedEntity));
    meshAction->setEnabled(!m_world->hasComponent<DabozzEngine::ECS::Mesh>(m_selectedEntity));
    fpControllerAction->setEnabled(!m_world->hasComponent<DabozzEngine::ECS::FirstPersonController>(m_selectedEntity));
    audioSourceAction->setEnabled(!m_world->hasComponent<DabozzEngine::ECS::AudioSource>(m_selectedEntity));

    QAction* selectedAction = menu.exec(QCursor::pos());
    
    if (selectedAction == rigidBodyAction) {
        m_world->addComponent<DabozzEngine::ECS::RigidBody>(m_selectedEntity);
    } else if (selectedAction == boxColliderAction) {
        m_world->addComponent<DabozzEngine::ECS::BoxCollider>(m_selectedEntity);
    } else if (selectedAction == sphereColliderAction) {
        m_world->addComponent<DabozzEngine::ECS::SphereCollider>(m_selectedEntity);
    } else if (selectedAction == meshAction) {
        m_world->addComponent<DabozzEngine::ECS::Mesh>(m_selectedEntity);
    } else if (selectedAction == fpControllerAction) {
        m_world->addComponent<DabozzEngine::ECS::FirstPersonController>(m_selectedEntity);
    } else if (selectedAction == audioSourceAction) {
        m_world->addComponent<DabozzEngine::ECS::AudioSource>(m_selectedEntity);
    }
    
    updateUI();
}

void ComponentInspector::updateUI()
{
    const bool hasSelection = (m_world && m_selectedEntity != 0);

    m_addComponentButton->setEnabled(hasSelection);
    m_nameEdit->setEnabled(hasSelection);
    m_positionX->setEnabled(hasSelection);
    m_positionY->setEnabled(hasSelection);
    m_positionZ->setEnabled(hasSelection);
    m_rotationX->setEnabled(hasSelection);
    m_rotationY->setEnabled(hasSelection);
    m_rotationZ->setEnabled(hasSelection);
    m_scaleX->setEnabled(hasSelection);
    m_scaleY->setEnabled(hasSelection);
    m_scaleZ->setEnabled(hasSelection);

    if (!hasSelection) {
        const QSignalBlocker nameBlocker(m_nameEdit);
        const QSignalBlocker positionXBlocker(m_positionX);
        const QSignalBlocker positionYBlocker(m_positionY);
        const QSignalBlocker positionZBlocker(m_positionZ);
        const QSignalBlocker rotationXBlocker(m_rotationX);
        const QSignalBlocker rotationYBlocker(m_rotationY);
        const QSignalBlocker rotationZBlocker(m_rotationZ);
        const QSignalBlocker scaleXBlocker(m_scaleX);
        const QSignalBlocker scaleYBlocker(m_scaleY);
        const QSignalBlocker scaleZBlocker(m_scaleZ);

        m_nameEdit->setText("");
        m_positionX->setText("");
        m_positionY->setText("");
        m_positionZ->setText("");
        m_rotationX->setText("");
        m_rotationY->setText("");
        m_rotationZ->setText("");
        m_scaleX->setText("");
        m_scaleY->setText("");
        m_scaleZ->setText("");
        updateComponentsList();
        return;
    }

    const QSignalBlocker nameBlocker(m_nameEdit);
    const QSignalBlocker positionXBlocker(m_positionX);
    const QSignalBlocker positionYBlocker(m_positionY);
    const QSignalBlocker positionZBlocker(m_positionZ);
    const QSignalBlocker rotationXBlocker(m_rotationX);
    const QSignalBlocker rotationYBlocker(m_rotationY);
    const QSignalBlocker rotationZBlocker(m_rotationZ);
    const QSignalBlocker scaleXBlocker(m_scaleX);
    const QSignalBlocker scaleYBlocker(m_scaleY);
    const QSignalBlocker scaleZBlocker(m_scaleZ);

    DabozzEngine::ECS::Name* nameComponent = m_world->getComponent<DabozzEngine::ECS::Name>(m_selectedEntity);
    if (nameComponent) {
        m_nameEdit->setText(nameComponent->name);
    } else {
        m_nameEdit->setText(QString("Entity %1").arg(m_selectedEntity));
    }

    DabozzEngine::ECS::Transform* transform = m_world->getComponent<DabozzEngine::ECS::Transform>(m_selectedEntity);
    if (transform) {
        const QVector3D euler = transform->rotation.toEulerAngles();
        m_positionX->setText(QString::number(transform->position.x()));
        m_positionY->setText(QString::number(transform->position.y()));
        m_positionZ->setText(QString::number(transform->position.z()));
        m_rotationX->setText(QString::number(euler.x()));
        m_rotationY->setText(QString::number(euler.y()));
        m_rotationZ->setText(QString::number(euler.z()));
        m_scaleX->setText(QString::number(transform->scale.x()));
        m_scaleY->setText(QString::number(transform->scale.y()));
        m_scaleZ->setText(QString::number(transform->scale.z()));
    } else {
        m_positionX->setText("");
        m_positionY->setText("");
        m_positionZ->setText("");
        m_rotationX->setText("");
        m_rotationY->setText("");
        m_rotationZ->setText("");
        m_scaleX->setText("");
        m_scaleY->setText("");
        m_scaleZ->setText("");
    }

    updateComponentsList();
}

void ComponentInspector::updateComponentsList()
{
    // Clear existing component widgets
    QLayoutItem* item;
    while ((item = m_componentsLayout->takeAt(0))) {
        delete item->widget();
        delete item;
    }
    
    if (!m_world || m_selectedEntity == 0) return;
    
    // List all components on the selected entity
    auto components = m_world->getComponents(m_selectedEntity);
    if (!components) return;
    
    for (const auto& [typeId, component] : *components) {
        QString displayName;
        if (typeId == typeid(DabozzEngine::ECS::Transform)) {
            displayName = "Transform";
        } else if (typeId == typeid(DabozzEngine::ECS::Name)) {
            displayName = "Name";
        } else if (typeId == typeid(DabozzEngine::ECS::RigidBody)) {
            displayName = "RigidBody";
        } else if (typeId == typeid(DabozzEngine::ECS::BoxCollider)) {
            displayName = "BoxCollider";
        } else if (typeId == typeid(DabozzEngine::ECS::SphereCollider)) {
            displayName = "SphereCollider";
        } else if (typeId == typeid(DabozzEngine::ECS::Mesh)) {
            displayName = "Mesh";
        } else if (typeId == typeid(DabozzEngine::ECS::FirstPersonController)) {
            displayName = "FirstPersonController";
        } else if (typeId == typeid(DabozzEngine::ECS::Animator)) {
            displayName = "Animator";
        } else if (typeId == typeid(DabozzEngine::ECS::AudioSource)) {
            displayName = "AudioSource";
        } else {
            displayName = QString::fromStdString(typeId.name());
        }

        QGroupBox* componentGroup = new QGroupBox(displayName);
        QVBoxLayout* componentLayout = new QVBoxLayout(componentGroup);

        bool isCoreComponent = (typeId == typeid(DabozzEngine::ECS::Transform) ||
                                typeId == typeid(DabozzEngine::ECS::Name) ||
                                typeId == typeid(DabozzEngine::ECS::Hierarchy));

        if (!isCoreComponent) {
            QPushButton* removeBtn = new QPushButton("X");
            removeBtn->setFixedSize(20, 20);
            removeBtn->setStyleSheet("QPushButton { color: red; font-weight: bold; border: none; }");
            removeBtn->setToolTip("Remove " + displayName);

            std::type_index capturedType = typeId;
            DabozzEngine::ECS::EntityID entity = m_selectedEntity;
            connect(removeBtn, &QPushButton::clicked, this, [this, capturedType, entity]() {
                if (!m_world || entity == 0) return;
                if (capturedType == typeid(DabozzEngine::ECS::RigidBody))
                    m_world->removeComponent<DabozzEngine::ECS::RigidBody>(entity);
                else if (capturedType == typeid(DabozzEngine::ECS::BoxCollider))
                    m_world->removeComponent<DabozzEngine::ECS::BoxCollider>(entity);
                else if (capturedType == typeid(DabozzEngine::ECS::SphereCollider))
                    m_world->removeComponent<DabozzEngine::ECS::SphereCollider>(entity);
                else if (capturedType == typeid(DabozzEngine::ECS::Mesh))
                    m_world->removeComponent<DabozzEngine::ECS::Mesh>(entity);
                else if (capturedType == typeid(DabozzEngine::ECS::FirstPersonController))
                    m_world->removeComponent<DabozzEngine::ECS::FirstPersonController>(entity);
                else if (capturedType == typeid(DabozzEngine::ECS::Animator))
                    m_world->removeComponent<DabozzEngine::ECS::Animator>(entity);
                else if (capturedType == typeid(DabozzEngine::ECS::AudioSource))
                    m_world->removeComponent<DabozzEngine::ECS::AudioSource>(entity);
                updateUI();
            });

            QHBoxLayout* headerLayout = new QHBoxLayout();
            headerLayout->addStretch();
            headerLayout->addWidget(removeBtn);
            componentLayout->addLayout(headerLayout);
        }

        if (typeId == typeid(DabozzEngine::ECS::RigidBody)) {
            DabozzEngine::ECS::RigidBody* rigidBody = static_cast<DabozzEngine::ECS::RigidBody*>(component.get());
            if (rigidBody) {
                componentLayout->addWidget(new QLabel(QString("Mass: %1").arg(rigidBody->mass)));
                componentLayout->addWidget(new QLabel(QString("Static: %1").arg(rigidBody->isStatic ? "Yes" : "No")));
            }
        } else if (typeId == typeid(DabozzEngine::ECS::BoxCollider)) {
            DabozzEngine::ECS::BoxCollider* boxCollider = static_cast<DabozzEngine::ECS::BoxCollider*>(component.get());
            if (boxCollider) {
                componentLayout->addWidget(new QLabel(QString("Size: %1, %2, %3")
                    .arg(boxCollider->size.x()).arg(boxCollider->size.y()).arg(boxCollider->size.z())));
            }
        } else if (typeId == typeid(DabozzEngine::ECS::SphereCollider)) {
            DabozzEngine::ECS::SphereCollider* sphereCollider = static_cast<DabozzEngine::ECS::SphereCollider*>(component.get());
            if (sphereCollider) {
                componentLayout->addWidget(new QLabel(QString("Radius: %1").arg(sphereCollider->radius)));
            }
        } else if (typeId == typeid(DabozzEngine::ECS::Mesh)) {
            DabozzEngine::ECS::Mesh* mesh = static_cast<DabozzEngine::ECS::Mesh*>(component.get());
            if (mesh) {
                componentLayout->addWidget(new QLabel("Mesh Componet"));
            }
        } else if (typeId == typeid(DabozzEngine::ECS::Name)) {
            DabozzEngine::ECS::Name* nameComponent = static_cast<DabozzEngine::ECS::Name*>(component.get());
            if (nameComponent) {
                componentLayout->addWidget(new QLabel(QString("Name: %1").arg(nameComponent->name)));
            }
        } else if (typeId == typeid(DabozzEngine::ECS::Transform)) {
            componentLayout->addWidget(new QLabel("Edit in Transform section above."));
        } else if (typeId == typeid(DabozzEngine::ECS::FirstPersonController)) {
            componentLayout->addWidget(new QLabel("First Person Controller"));
        } else if (typeId == typeid(DabozzEngine::ECS::Animator)) {
            DabozzEngine::ECS::Animator* animator = static_cast<DabozzEngine::ECS::Animator*>(component.get());
            if (animator) {
                componentLayout->addWidget(new QLabel(QString("Clips: %1").arg(animator->animations.size())));
                componentLayout->addWidget(new QLabel(QString("Playing: %1").arg(animator->isPlaying ? "Yes" : "No")));
                componentLayout->addWidget(new QLabel(QString("Loop: %1").arg(animator->loop ? "Yes" : "No")));
                componentLayout->addWidget(new QLabel(QString("Speed: %1").arg(animator->playbackSpeed)));

                if (!animator->animations.empty()) {
                    QComboBox* clipCombo = new QComboBox();
                    for (auto& [name, anim] : animator->animations) {
                        clipCombo->addItem(name);
                    }
                    clipCombo->setCurrentText(animator->currentClipName);

                    DabozzEngine::ECS::EntityID entity = m_selectedEntity;
                    connect(clipCombo, &QComboBox::currentTextChanged, this, [this, entity](const QString& clipName) {
                        if (!m_world) return;
                        DabozzEngine::ECS::Animator* anim = m_world->getComponent<DabozzEngine::ECS::Animator>(entity);
                        if (anim) {
                            anim->playAnimation(clipName);
                        }
                    });

                    QFormLayout* formLayout = new QFormLayout();
                    formLayout->addRow("Active Clip:", clipCombo);
                    componentLayout->addLayout(formLayout);
                }
            }
        } else if (typeId == typeid(DabozzEngine::ECS::AudioSource)) {
            DabozzEngine::ECS::AudioSource* audio = static_cast<DabozzEngine::ECS::AudioSource*>(component.get());
            if (audio) {
                componentLayout->addWidget(new QLabel(QString("File: %1").arg(audio->filePath.isEmpty() ? "None" : audio->filePath)));
                componentLayout->addWidget(new QLabel(QString("Volume: %1").arg(audio->volume)));
                componentLayout->addWidget(new QLabel(QString("Pitch: %1").arg(audio->pitch)));
                componentLayout->addWidget(new QLabel(QString("Loop: %1").arg(audio->loop ? "Yes" : "No")));
                componentLayout->addWidget(new QLabel(QString("Playing: %1").arg(audio->isPlaying ? "Yes" : "No")));

                QPushButton* browseBtn = new QPushButton("Browse WAV...");
                DabozzEngine::ECS::EntityID entity = m_selectedEntity;
                connect(browseBtn, &QPushButton::clicked, this, [this, entity]() {
                    if (!m_world) return;
                    DabozzEngine::ECS::AudioSource* a = m_world->getComponent<DabozzEngine::ECS::AudioSource>(entity);
                    if (!a) return;
                    QString path = QFileDialog::getOpenFileName(this, "Select Audio File", QString(), "WAV Files (*.wav)");
                    if (!path.isEmpty()) {
                        a->filePath = path;
                        a->isLoaded = false;
                        updateUI();
                    }
                });
                componentLayout->addWidget(browseBtn);
            }
        } else {
            componentLayout->addWidget(new QLabel(displayName));
        }

        m_componentsLayout->addWidget(componentGroup);
    }
}