#include "editor/hierarchyview.h"
#include "ecs/components/name.h"
#include "ecs/components/hierarchy.h"
#include "ecs/components/transform.h"
#include "ecs/components/mesh.h"
#include "ecs/components/firstpersoncontroller.h"
#include <QHeaderView>
#include <QMenu>
#include <QInputDialog>

HierarchyView::HierarchyView(QWidget* parent)
    : QWidget(parent)
    , m_world(nullptr)
{
    setupUI();
    connectSignals();
}

HierarchyView::~HierarchyView()
{
}

void HierarchyView::setupUI()
{
    m_layout = new QVBoxLayout(this);
    
    m_treeWidget = new QTreeWidget();
    m_treeWidget->setHeaderLabel("Scene Objects");
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    
    m_layout->addWidget(m_treeWidget);
    setLayout(m_layout);
}

void HierarchyView::connectSignals()
{
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &HierarchyView::onItemSelectionChanged);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &HierarchyView::onContextMenuRequested);
}

void HierarchyView::setWorld(DabozzEngine::ECS::World* world)
{
    m_world = world;
    refreshHierarchy();
}

void HierarchyView::refreshHierarchy()
{
    if (!m_world) return;
    
    m_treeWidget->clear();
    
    // Build a map of parent -> children
    std::unordered_map<DabozzEngine::ECS::EntityID, std::vector<DabozzEngine::ECS::EntityID>> hierarchyMap;
    std::unordered_map<DabozzEngine::ECS::EntityID, DabozzEngine::ECS::EntityID> parentMap;
    
    for (DabozzEngine::ECS::EntityID entity : m_world->getEntities()) {
        DabozzEngine::ECS::Hierarchy* hierarchy = m_world->getComponent<DabozzEngine::ECS::Hierarchy>(entity);
        if (hierarchy) {
            if (hierarchy->parent != 0) {
                hierarchyMap[hierarchy->parent].push_back(entity);
                parentMap[entity] = hierarchy->parent;
            }
        }
    }
    
    // Find root entities (no parent)
    std::vector<DabozzEngine::ECS::EntityID> rootEntities;
    for (DabozzEngine::ECS::EntityID entity : m_world->getEntities()) {
        if (parentMap.find(entity) == parentMap.end()) {
            rootEntities.push_back(entity);
        }
    }
    
    // Build tree recursively
    for (DabozzEngine::ECS::EntityID root : rootEntities) {
        buildTree(root, hierarchyMap, nullptr);
    }
}

void HierarchyView::buildTree(DabozzEngine::ECS::EntityID entity, 
                              const std::unordered_map<DabozzEngine::ECS::EntityID, std::vector<DabozzEngine::ECS::EntityID>>& hierarchyMap,
                              QTreeWidgetItem* parentItem)
{
    DabozzEngine::ECS::Name* nameComponent = m_world->getComponent<DabozzEngine::ECS::Name>(entity);
    QString name = nameComponent ? nameComponent->name : QString("Entity %1").arg(entity);
    
    QTreeWidgetItem* item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(m_treeWidget);
    item->setText(0, name);
    item->setData(0, Qt::UserRole, QVariant::fromValue(entity));
    
    if (!parentItem) {
        m_treeWidget->addTopLevelItem(item);
    }
    
    auto it = hierarchyMap.find(entity);
    if (it != hierarchyMap.end()) {
        for (DabozzEngine::ECS::EntityID child : it->second) {
            buildTree(child, hierarchyMap, item);
        }
    }
}

void HierarchyView::onItemSelectionChanged()
{
    QTreeWidgetItem* item = m_treeWidget->currentItem();
    if (item) {
        DabozzEngine::ECS::EntityID entity = item->data(0, Qt::UserRole).value<DabozzEngine::ECS::EntityID>();
        emit entitySelected(entity);
    }
}

void HierarchyView::onContextMenuRequested(const QPoint& pos)
{
    QMenu menu(this);
    
    QAction* createEntityAction = menu.addAction("Create Empty Entity");
    QAction* createCubeAction = menu.addAction("Create Cube");
    QAction* createCameraAction = menu.addAction("Create Camera");
    menu.addSeparator();
    QAction* deleteAction = menu.addAction("Delete Entity");
    
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    if (!item) {
        deleteAction->setEnabled(false);
    }
    
    QAction* selectedAction = menu.exec(m_treeWidget->mapToGlobal(pos));
    
    if (selectedAction == createEntityAction) {
        onCreateEntity();
    } else if (selectedAction == createCubeAction) {
        onCreateCube();
    } else if (selectedAction == createCameraAction) {
        onCreateCamera();
    } else if (selectedAction == deleteAction && item) {
        onDeleteEntity();
    }
}

void HierarchyView::onCreateEntity()
{
    if (!m_world) return;
    
    DabozzEngine::ECS::EntityID entity = m_world->createEntity();
    m_world->addComponent<DabozzEngine::ECS::Name>(entity, "New Entity");
    m_world->addComponent<DabozzEngine::ECS::Transform>(entity);
    m_world->addComponent<DabozzEngine::ECS::Hierarchy>(entity);
    refreshHierarchy();
}

void HierarchyView::onCreateCube()
{
    if (!m_world) return;
    
    DabozzEngine::ECS::EntityID entity = m_world->createEntity();
    m_world->addComponent<DabozzEngine::ECS::Name>(entity, "Cube");
    m_world->addComponent<DabozzEngine::ECS::Transform>(entity);
    m_world->addComponent<DabozzEngine::ECS::Hierarchy>(entity);
    
    // Create cube mesh with actual geometry
    auto* meshComponent = m_world->addComponent<DabozzEngine::ECS::Mesh>(entity);
    
    // Cube vertices (position + normal + texcoord)
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
    
    unsigned int indices[] = {
        0,  1,  2,  2,  3,  0,   // Front
        4,  5,  6,  6,  7,  4,   // Back
        8,  9, 10, 10, 11,  8,   // Top
        12, 13, 14, 14, 15, 12,  // Bottom
        16, 17, 18, 18, 19, 16,  // Right
        20, 21, 22, 22, 23, 20   // Left
    };
    
    // Extract positions, normals, texcoords
    for (int i = 0; i < 24; i++) {
        meshComponent->vertices.push_back(vertices[i * 8 + 0]);
        meshComponent->vertices.push_back(vertices[i * 8 + 1]);
        meshComponent->vertices.push_back(vertices[i * 8 + 2]);
        
        meshComponent->normals.push_back(vertices[i * 8 + 3]);
        meshComponent->normals.push_back(vertices[i * 8 + 4]);
        meshComponent->normals.push_back(vertices[i * 8 + 5]);
        
        meshComponent->texCoords.push_back(vertices[i * 8 + 6]);
        meshComponent->texCoords.push_back(vertices[i * 8 + 7]);
    }
    
    for (int i = 0; i < 36; i++) {
        meshComponent->indices.push_back(indices[i]);
    }
    
    refreshHierarchy();
}

void HierarchyView::onCreateCamera()
{
    if (!m_world) return;
    
    DabozzEngine::ECS::EntityID entity = m_world->createEntity();
    m_world->addComponent<DabozzEngine::ECS::Name>(entity, "Camera");
    m_world->addComponent<DabozzEngine::ECS::Transform>(entity);
    m_world->addComponent<DabozzEngine::ECS::FirstPersonController>(entity);
    m_world->addComponent<DabozzEngine::ECS::Hierarchy>(entity);
    refreshHierarchy();
}

void HierarchyView::onDeleteEntity()
{
    if (!m_world) return;
    
    QTreeWidgetItem* item = m_treeWidget->currentItem();
    if (item) {
        DabozzEngine::ECS::EntityID entity = item->data(0, Qt::UserRole).value<DabozzEngine::ECS::EntityID>();
        m_world->destroyEntity(entity);
        refreshHierarchy();
    }
}