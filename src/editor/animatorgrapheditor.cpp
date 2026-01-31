#include "editor/animatorgrapheditor.h"
#include "editor/graphitems/statenodeitem.h"
#include "editor/graphitems/transitionarrowitem.h"
#include "editor/graphitems/entrynodeitem.h"
#include "ecs/components/animator.h"
#include <QWheelEvent>
#include <QGraphicsSceneMouseEvent>
#include <QInputDialog>
#include <QMessageBox>

AnimatorGraphEditor::AnimatorGraphEditor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();

    m_runtimeTimer = new QTimer(this);
    connect(m_runtimeTimer, &QTimer::timeout, this, &AnimatorGraphEditor::updateRuntimeHighlighting);
    m_runtimeTimer->start(100);
}

void AnimatorGraphEditor::setWorld(DabozzEngine::ECS::World* world)
{
    m_world = world;
}

void AnimatorGraphEditor::setSelectedEntity(DabozzEngine::ECS::EntityID entity)
{
    m_entity = entity;
    m_graph = nullptr;

    if (m_world && entity != 0) {
        auto* animator = m_world->getComponent<DabozzEngine::ECS::Animator>(entity);
        if (animator && animator->graph) {
            m_graph = animator->graph.get();
        }
    }

    rebuildGraph();
}

void AnimatorGraphEditor::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // No graph label + create button
    m_noGraphLabel = new QLabel("Select an entity with an Animator to edit its state graph.");
    m_noGraphLabel->setAlignment(Qt::AlignCenter);
    m_noGraphLabel->setStyleSheet("color: #888; padding: 20px;");

    m_createBtn = new QPushButton("Create Animator Controller");
    connect(m_createBtn, &QPushButton::clicked, this, &AnimatorGraphEditor::onCreateController);

    // Splitter: left panel | graph view
    m_splitter = new QSplitter(Qt::Horizontal);

    // Left panel: parameters
    m_leftPanel = new QWidget();
    m_leftLayout = new QVBoxLayout(m_leftPanel);
    m_leftLayout->setContentsMargins(4, 4, 4, 4);

    auto* paramLabel = new QLabel("Parameters");
    paramLabel->setStyleSheet("font-weight: bold;");
    m_leftLayout->addWidget(paramLabel);

    m_paramList = new QListWidget();
    m_paramList->setMaximumWidth(200);
    connect(m_paramList, &QListWidget::itemChanged, this, &AnimatorGraphEditor::onParameterValueChanged);
    m_leftLayout->addWidget(m_paramList);

    auto* paramBtnLayout = new QHBoxLayout();
    m_paramTypeCombo = new QComboBox();
    m_paramTypeCombo->addItem("Bool");
    m_paramTypeCombo->addItem("Float");
    m_paramTypeCombo->addItem("Int");
    m_paramTypeCombo->addItem("Trigger");
    paramBtnLayout->addWidget(m_paramTypeCombo);

    m_addParamBtn = new QPushButton("+");
    m_addParamBtn->setMaximumWidth(30);
    connect(m_addParamBtn, &QPushButton::clicked, this, &AnimatorGraphEditor::onAddParameter);
    paramBtnLayout->addWidget(m_addParamBtn);

    m_delParamBtn = new QPushButton("-");
    m_delParamBtn->setMaximumWidth(30);
    connect(m_delParamBtn, &QPushButton::clicked, this, &AnimatorGraphEditor::onDeleteParameter);
    paramBtnLayout->addWidget(m_delParamBtn);

    m_leftLayout->addLayout(paramBtnLayout);

    // Transition inspector
    m_transInspector = new QGroupBox("Transition");
    auto* transLayout = new QFormLayout(m_transInspector);
    m_transLabel = new QLabel("");
    transLayout->addRow(m_transLabel);

    m_blendDurationSpin = new QDoubleSpinBox();
    m_blendDurationSpin->setRange(0.0, 5.0);
    m_blendDurationSpin->setSingleStep(0.05);
    m_blendDurationSpin->setValue(0.25);
    connect(m_blendDurationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnimatorGraphEditor::onTransitionPropertyChanged);
    transLayout->addRow("Blend Duration:", m_blendDurationSpin);

    m_hasExitTimeCheck = new QCheckBox();
    m_hasExitTimeCheck->setChecked(true);
    connect(m_hasExitTimeCheck, &QCheckBox::toggled, this, &AnimatorGraphEditor::onTransitionPropertyChanged);
    transLayout->addRow("Has Exit Time:", m_hasExitTimeCheck);

    m_exitTimeSpin = new QDoubleSpinBox();
    m_exitTimeSpin->setRange(0.0, 1.0);
    m_exitTimeSpin->setSingleStep(0.05);
    m_exitTimeSpin->setValue(0.9);
    connect(m_exitTimeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnimatorGraphEditor::onTransitionPropertyChanged);
    transLayout->addRow("Exit Time:", m_exitTimeSpin);

    m_transInspector->setVisible(false);
    m_leftLayout->addWidget(m_transInspector);

    m_leftLayout->addStretch();

    // Graph view
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-2000, -2000, 4000, 4000);
    m_scene->setBackgroundBrush(QColor(35, 35, 35));

    m_graphView = new QGraphicsView(m_scene);
    m_graphView->setRenderHint(QPainter::Antialiasing);
    m_graphView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_graphView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

    m_splitter->addWidget(m_leftPanel);
    m_splitter->addWidget(m_graphView);
    m_splitter->setSizes({200, 600});

    mainLayout->addWidget(m_noGraphLabel);
    mainLayout->addWidget(m_createBtn);
    mainLayout->addWidget(m_splitter);

    m_splitter->setVisible(false);
}

void AnimatorGraphEditor::rebuildGraph()
{
    clearGraph();
    hideTransitionInspector();

    if (!m_graph) {
        m_splitter->setVisible(false);
        m_noGraphLabel->setVisible(true);

        // Show create button only if entity has an Animator
        bool hasAnimator = false;
        if (m_world && m_entity != 0) {
            hasAnimator = m_world->getComponent<DabozzEngine::ECS::Animator>(m_entity) != nullptr;
        }
        m_createBtn->setVisible(hasAnimator);
        if (!hasAnimator) {
            m_noGraphLabel->setText("Select an entity with an Animator to edit its state graph.");
        } else {
            m_noGraphLabel->setText("This Animator has no controller. Create one to set up state transitions.");
        }
        return;
    }

    m_noGraphLabel->setVisible(false);
    m_createBtn->setVisible(false);
    m_splitter->setVisible(true);

    // Create entry node
    m_entryNode = new EntryNodeItem();
    m_entryNode->setPos(-250, 0);
    m_scene->addItem(m_entryNode);

    // Create state nodes
    for (auto& state : m_graph->states) {
        auto* node = new StateNodeItem(state.id, state.name);
        node->setPos(state.editorPosition);
        node->setEntryState(state.id == m_graph->entryStateId);

        connect(node, &StateNodeItem::positionChanged, this, &AnimatorGraphEditor::onNodePositionChanged);
        connect(node, &StateNodeItem::makeTransitionRequested, this, &AnimatorGraphEditor::onMakeTransition);
        connect(node, &StateNodeItem::setEntryRequested, this, &AnimatorGraphEditor::onSetEntryState);
        connect(node, &StateNodeItem::deleteRequested, this, &AnimatorGraphEditor::onDeleteState);

        m_scene->addItem(node);
        m_stateNodes[state.id] = node;
    }

    // Entry arrow
    if (m_graph->entryStateId != -1 && m_stateNodes.count(m_graph->entryStateId)) {
        auto* target = m_stateNodes[m_graph->entryStateId];
        m_entryArrow = new QGraphicsLineItem();
        m_entryArrow->setPen(QPen(QColor(80, 220, 80), 2));
        m_entryArrow->setLine(QLineF(m_entryNode->rightEdge(), target->leftEdge()));
        m_entryArrow->setZValue(-1);
        m_scene->addItem(m_entryArrow);
    }

    // Create transition arrows
    for (auto& transition : m_graph->transitions) {
        if (m_stateNodes.count(transition.sourceStateId) && m_stateNodes.count(transition.destStateId)) {
            auto* source = m_stateNodes[transition.sourceStateId];
            auto* dest = m_stateNodes[transition.destStateId];
            auto* arrow = new TransitionArrowItem(transition.id, source, dest);
            connect(arrow, &TransitionArrowItem::selected, this, &AnimatorGraphEditor::onTransitionSelected);
            m_scene->addItem(arrow);
            m_transitionArrows[transition.id] = arrow;
        }
    }

    // Update parameters list
    m_paramList->clear();
    for (auto& [name, param] : m_graph->parameters) {
        QString display;
        switch (param.type) {
            case DabozzEngine::ECS::AnimParamType::Bool:
                display = QString("[Bool] %1 = %2").arg(name).arg(std::get<bool>(param.value) ? "true" : "false");
                break;
            case DabozzEngine::ECS::AnimParamType::Float:
                display = QString("[Float] %1 = %2").arg(name).arg(std::get<float>(param.value));
                break;
            case DabozzEngine::ECS::AnimParamType::Int:
                display = QString("[Int] %1 = %2").arg(name).arg(std::get<int>(param.value));
                break;
            case DabozzEngine::ECS::AnimParamType::Trigger:
                display = QString("[Trigger] %1").arg(name);
                break;
        }
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, name);
        m_paramList->addItem(item);
    }
}

void AnimatorGraphEditor::clearGraph()
{
    m_stateNodes.clear();
    m_transitionArrows.clear();
    m_entryNode = nullptr;
    m_entryArrow = nullptr;
    m_scene->clear();
}

void AnimatorGraphEditor::onCreateController()
{
    if (!m_world || m_entity == 0) return;

    auto* animator = m_world->getComponent<DabozzEngine::ECS::Animator>(m_entity);
    if (!animator) return;

    animator->graph = std::make_shared<DabozzEngine::ECS::AnimatorGraph>();
    m_graph = animator->graph.get();

    // Auto-create states from existing clips
    float x = 0, y = 0;
    for (auto& [clipName, anim] : animator->animations) {
        m_graph->addState(clipName, clipName, QPointF(x, y));
        y += 80;
    }

    rebuildGraph();
}

void AnimatorGraphEditor::onAddParameter()
{
    if (!m_graph) return;

    bool ok;
    QString name = QInputDialog::getText(this, "Add Parameter", "Parameter name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    if (m_graph->parameters.count(name)) {
        QMessageBox::warning(this, "Duplicate", "A parameter with that name already exists.");
        return;
    }

    DabozzEngine::ECS::AnimParamType type;
    switch (m_paramTypeCombo->currentIndex()) {
        case 0: type = DabozzEngine::ECS::AnimParamType::Bool; break;
        case 1: type = DabozzEngine::ECS::AnimParamType::Float; break;
        case 2: type = DabozzEngine::ECS::AnimParamType::Int; break;
        case 3: type = DabozzEngine::ECS::AnimParamType::Trigger; break;
        default: type = DabozzEngine::ECS::AnimParamType::Bool; break;
    }

    m_graph->parameters[name] = DabozzEngine::ECS::AnimParam(name, type);
    rebuildGraph();
}

void AnimatorGraphEditor::onDeleteParameter()
{
    if (!m_graph || !m_paramList->currentItem()) return;

    QString name = m_paramList->currentItem()->data(Qt::UserRole).toString();
    m_graph->parameters.erase(name);

    // Remove conditions referencing this parameter
    for (auto& t : m_graph->transitions) {
        t.conditions.erase(
            std::remove_if(t.conditions.begin(), t.conditions.end(),
                [&name](const DabozzEngine::ECS::TransitionCondition& c) {
                    return c.paramName == name;
                }),
            t.conditions.end()
        );
    }

    rebuildGraph();
}

void AnimatorGraphEditor::onMakeTransition(int sourceStateId)
{
    if (!m_graph) return;

    // Build a list of target states (excluding source)
    QStringList names;
    std::vector<int> ids;
    for (auto& s : m_graph->states) {
        if (s.id != sourceStateId) {
            names << s.name;
            ids.push_back(s.id);
        }
    }

    if (names.isEmpty()) return;

    bool ok;
    QString selected = QInputDialog::getItem(this, "Make Transition", "Target state:", names, 0, false, &ok);
    if (!ok) return;

    int idx = names.indexOf(selected);
    if (idx >= 0) {
        m_graph->addTransition(sourceStateId, ids[idx]);
        rebuildGraph();
    }
}

void AnimatorGraphEditor::onSetEntryState(int stateId)
{
    if (!m_graph) return;
    m_graph->setEntryState(stateId);
    rebuildGraph();
}

void AnimatorGraphEditor::onDeleteState(int stateId)
{
    if (!m_graph) return;
    m_graph->removeState(stateId);
    rebuildGraph();
}

void AnimatorGraphEditor::onNodePositionChanged(int stateId, QPointF newPos)
{
    if (!m_graph) return;

    auto* state = m_graph->findState(stateId);
    if (state) {
        state->editorPosition = newPos;
    }

    // Update arrows
    for (auto& [id, arrow] : m_transitionArrows) {
        arrow->updatePosition();
    }

    // Update entry arrow
    if (m_entryArrow && m_entryNode && m_graph->entryStateId != -1 && m_stateNodes.count(m_graph->entryStateId)) {
        auto* target = m_stateNodes[m_graph->entryStateId];
        m_entryArrow->setLine(QLineF(m_entryNode->rightEdge(), target->leftEdge()));
    }
}

void AnimatorGraphEditor::onTransitionSelected(int transitionId)
{
    m_selectedTransitionId = transitionId;
    showTransitionInspector(transitionId);
}

void AnimatorGraphEditor::showTransitionInspector(int transitionId)
{
    if (!m_graph) return;

    DabozzEngine::ECS::AnimTransition* t = nullptr;
    for (auto& tr : m_graph->transitions) {
        if (tr.id == transitionId) {
            t = &tr;
            break;
        }
    }

    if (!t) {
        hideTransitionInspector();
        return;
    }

    // Find state names
    auto* src = m_graph->findState(t->sourceStateId);
    auto* dst = m_graph->findState(t->destStateId);
    m_transLabel->setText(QString("%1 -> %2").arg(src ? src->name : "?").arg(dst ? dst->name : "?"));

    m_blendDurationSpin->blockSignals(true);
    m_blendDurationSpin->setValue(t->blendDuration);
    m_blendDurationSpin->blockSignals(false);

    m_hasExitTimeCheck->blockSignals(true);
    m_hasExitTimeCheck->setChecked(t->hasExitTime);
    m_hasExitTimeCheck->blockSignals(false);

    m_exitTimeSpin->blockSignals(true);
    m_exitTimeSpin->setValue(t->exitTime);
    m_exitTimeSpin->blockSignals(false);

    m_transInspector->setVisible(true);
}

void AnimatorGraphEditor::hideTransitionInspector()
{
    m_transInspector->setVisible(false);
    m_selectedTransitionId = -1;
}

void AnimatorGraphEditor::onTransitionPropertyChanged()
{
    if (!m_graph || m_selectedTransitionId == -1) return;

    for (auto& t : m_graph->transitions) {
        if (t.id == m_selectedTransitionId) {
            t.blendDuration = m_blendDurationSpin->value();
            t.hasExitTime = m_hasExitTimeCheck->isChecked();
            t.exitTime = m_exitTimeSpin->value();
            break;
        }
    }
}

void AnimatorGraphEditor::onParameterValueChanged(QListWidgetItem* item)
{
    // For now, parameters are edited by clicking - could add inline editing later
    if (!m_graph || !item) return;

    QString name = item->data(Qt::UserRole).toString();
    auto it = m_graph->parameters.find(name);
    if (it == m_graph->parameters.end()) return;

    auto& param = it->second;

    if (param.type == DabozzEngine::ECS::AnimParamType::Bool) {
        bool current = std::get<bool>(param.value);
        param.value = !current;
        rebuildGraph();
    } else if (param.type == DabozzEngine::ECS::AnimParamType::Trigger) {
        param.value = true;
    } else if (param.type == DabozzEngine::ECS::AnimParamType::Float) {
        bool ok;
        double val = QInputDialog::getDouble(this, "Set Float", QString("Value for %1:").arg(name),
                                             std::get<float>(param.value), -9999, 9999, 2, &ok);
        if (ok) {
            param.value = static_cast<float>(val);
            rebuildGraph();
        }
    } else if (param.type == DabozzEngine::ECS::AnimParamType::Int) {
        bool ok;
        int val = QInputDialog::getInt(this, "Set Int", QString("Value for %1:").arg(name),
                                       std::get<int>(param.value), -9999, 9999, 1, &ok);
        if (ok) {
            param.value = val;
            rebuildGraph();
        }
    }
}

void AnimatorGraphEditor::updateRuntimeHighlighting()
{
    if (!m_graph) return;

    for (auto& [id, node] : m_stateNodes) {
        bool isActive = (id == m_graph->activeStateId && !m_graph->inTransition);
        bool isBlending = m_graph->inTransition &&
                          (id == m_graph->activeStateId || id == m_graph->previousStateId);
        node->setActive(isActive);
        if (isBlending && id == m_graph->previousStateId) {
            node->setBlending(true, m_graph->transitionProgress);
        } else if (isBlending && id == m_graph->activeStateId) {
            node->setBlending(true, 1.0f - m_graph->transitionProgress);
        } else {
            node->setBlending(false);
        }
    }

    // Highlight active transition arrow
    for (auto& [id, arrow] : m_transitionArrows) {
        arrow->setActive(false);
    }
    if (m_graph->inTransition) {
        for (auto& t : m_graph->transitions) {
            if (t.sourceStateId == m_graph->previousStateId && t.destStateId == m_graph->activeStateId) {
                if (m_transitionArrows.count(t.id)) {
                    m_transitionArrows[t.id]->setActive(true);
                }
                break;
            }
        }
    }
}

void AnimatorGraphEditor::wheelEvent(QWheelEvent* event)
{
    if (m_graphView && m_graphView->underMouse()) {
        double scaleFactor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        m_graphView->scale(scaleFactor, scaleFactor);
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}
