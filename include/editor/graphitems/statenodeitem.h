#pragma once

#include <QGraphicsObject>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>

class StateNodeItem : public QGraphicsObject
{
    Q_OBJECT

public:
    StateNodeItem(int stateId, const QString& name, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    int stateId() const { return m_stateId; }
    QString name() const { return m_name; }
    void setName(const QString& name);
    void setActive(bool active);
    void setEntryState(bool entry);
    void setBlending(bool blending, float progress = 0.0f);

    QPointF centerPos() const;
    QPointF rightEdge() const;
    QPointF leftEdge() const;

signals:
    void positionChanged(int stateId, QPointF newPos);
    void makeTransitionRequested(int sourceStateId);
    void setEntryRequested(int stateId);
    void deleteRequested(int stateId);

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    static constexpr float WIDTH = 180.0f;
    static constexpr float HEIGHT = 50.0f;
    static constexpr float RADIUS = 8.0f;

    int m_stateId;
    QString m_name;
    bool m_isActive = false;
    bool m_isEntry = false;
    bool m_isBlending = false;
    float m_blendProgress = 0.0f;
};
