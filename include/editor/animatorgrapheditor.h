#pragma once

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QTimer>
#include <QGraphicsLineItem>
#include <map>
#include "ecs/world.h"
#include "ecs/components/animatorgraph.h"

class StateNodeItem;
class TransitionArrowItem;
class EntryNodeItem;

class AnimatorGraphEditor : public QWidget
{
    Q_OBJECT

public:
    AnimatorGraphEditor(QWidget* parent = nullptr);

    void setWorld(DabozzEngine::ECS::World* world);

public slots:
    void setSelectedEntity(DabozzEngine::ECS::EntityID entity);

private slots:
    void onCreateController();
    void onAddParameter();
    void onDeleteParameter();
    void onMakeTransition(int sourceStateId);
    void onSetEntryState(int stateId);
    void onDeleteState(int stateId);
    void onNodePositionChanged(int stateId, QPointF newPos);
    void onTransitionSelected(int transitionId);
    void onTransitionPropertyChanged();
    void onParameterValueChanged(QListWidgetItem* item);
    void updateRuntimeHighlighting();

protected:
    void wheelEvent(QWheelEvent* event) override;

private:
    void setupUI();
    void rebuildGraph();
    void clearGraph();
    void showTransitionInspector(int transitionId);
    void hideTransitionInspector();

    // Scene interaction for drag-to-create transitions
    bool m_isDraggingTransition = false;
    int m_dragSourceStateId = -1;
    QGraphicsLineItem* m_dragLine = nullptr;

    // Core
    DabozzEngine::ECS::World* m_world = nullptr;
    DabozzEngine::ECS::EntityID m_entity = 0;
    DabozzEngine::ECS::AnimatorGraph* m_graph = nullptr;

    // Layout
    QSplitter* m_splitter;
    QWidget* m_leftPanel;
    QVBoxLayout* m_leftLayout;

    // Parameters panel
    QListWidget* m_paramList;
    QComboBox* m_paramTypeCombo;
    QPushButton* m_addParamBtn;
    QPushButton* m_delParamBtn;

    // Graph view
    QGraphicsScene* m_scene;
    QGraphicsView* m_graphView;

    // Transition inspector (right side or overlay)
    QGroupBox* m_transInspector;
    QDoubleSpinBox* m_blendDurationSpin;
    QCheckBox* m_hasExitTimeCheck;
    QDoubleSpinBox* m_exitTimeSpin;
    QLabel* m_transLabel;
    int m_selectedTransitionId = -1;

    // Create controller button (shown when no graph)
    QPushButton* m_createBtn;
    QLabel* m_noGraphLabel;

    // Node tracking
    std::map<int, StateNodeItem*> m_stateNodes;
    std::map<int, TransitionArrowItem*> m_transitionArrows;
    EntryNodeItem* m_entryNode = nullptr;
    QGraphicsLineItem* m_entryArrow = nullptr;

    // Runtime update timer
    QTimer* m_runtimeTimer;
};
