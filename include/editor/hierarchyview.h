#pragma once
#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QUndoStack>
#include "ecs/world.h"

namespace DabozzEngine {
namespace ECS {
class World;
}
}

class HierarchyView : public QWidget
{
    Q_OBJECT

public:
    HierarchyView(QWidget* parent = nullptr);
    ~HierarchyView();

    void setWorld(DabozzEngine::ECS::World* world);
    void setUndoStack(QUndoStack* undoStack);
    void refreshHierarchy();
    void duplicateSelectedEntity();

signals:
    void entitySelected(DabozzEngine::ECS::EntityID entity);

private slots:
    void onItemSelectionChanged();
    void onContextMenuRequested(const QPoint& pos);
    void onCreateEntity();
    void onCreateCube();
    void onCreateCamera();
    void onDeleteEntity();

private:
    void setupUI();
    void connectSignals();
    void buildTree(DabozzEngine::ECS::EntityID entity,
                   const std::unordered_map<DabozzEngine::ECS::EntityID, std::vector<DabozzEngine::ECS::EntityID>>& hierarchyMap,
                   QTreeWidgetItem* parentItem);
    void keyPressEvent(QKeyEvent* event) override;

    QVBoxLayout* m_layout;
    QTreeWidget* m_treeWidget;
    DabozzEngine::ECS::World* m_world;
    QUndoStack* m_undoStack = nullptr;
};