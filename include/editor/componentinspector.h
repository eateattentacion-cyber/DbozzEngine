#pragma once
#include <QWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include "ecs/world.h"

namespace DabozzEngine {
namespace ECS {
class World;
}
}

class ComponentInspector : public QWidget
{
    Q_OBJECT

public:
    ComponentInspector(QWidget* parent = nullptr);
    ~ComponentInspector();

    void setWorld(DabozzEngine::ECS::World* world);
    void setSelectedEntity(DabozzEngine::ECS::EntityID entity);
    void clearSelection();

public slots:
    void refreshSelectedEntity(DabozzEngine::ECS::EntityID entity);

private slots:
    void onTransformChanged();
    void onNameChanged();
    void onAddComponentClicked();

public:
    void updateUI();

private:
    void setupUI();
    void createTransformSection();
    void createPropertiesSection();
    void createComponentsSection();
    void updateComponentsList();
    
    QVBoxLayout* m_mainLayout;
    QFormLayout* m_transformLayout;
    QFormLayout* m_propertiesLayout;
    QVBoxLayout* m_componentsLayout;
    QWidget* m_componentsWidget;
    
    QLineEdit* m_positionX;
    QLineEdit* m_positionY;
    QLineEdit* m_positionZ;
    
    QLineEdit* m_rotationX;
    QLineEdit* m_rotationY;
    QLineEdit* m_rotationZ;
    
    QLineEdit* m_scaleX;
    QLineEdit* m_scaleY;
    QLineEdit* m_scaleZ;
    
    QLineEdit* m_nameEdit;
    QCheckBox* m_activeCheck;
    
    QPushButton* m_addComponentButton;
    
    DabozzEngine::ECS::World* m_world;
    DabozzEngine::ECS::EntityID m_selectedEntity;
};