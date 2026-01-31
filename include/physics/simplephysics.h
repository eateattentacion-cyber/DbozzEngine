#pragma once
#include <QVector3D>
#include <QQuaternion>
#include <vector>

namespace DabozzEngine::Physics {

struct AABB {
    QVector3D min;
    QVector3D max;
};

struct RigidBodyState {
    QVector3D position;
    QQuaternion rotation;
    QVector3D velocity;
    QVector3D angularVelocity;
    float mass;
    float inverseMass;
    bool isStatic;
    bool isSleeping;
    AABB bounds;
};

class ButsuriEngine {
public:
    ButsuriEngine();
    ~ButsuriEngine();
    
    void initialize();
    void shutdown();
    void update(float deltaTime);
    
    int createBody(const QVector3D& position, const QVector3D& size, float mass, bool isStatic);
    void removeBody(int bodyId);
    
    RigidBodyState* getBody(int bodyId);
    
    void setGravity(const QVector3D& gravity) { m_gravity = gravity; }
    QVector3D getGravity() const { return m_gravity; }
    
    static ButsuriEngine* getInstance();
    
private:
    void integrateVelocities(float deltaTime);
    void detectCollisions();
    void resolveCollisions();
    void integratePositions(float deltaTime);
    
    bool checkAABBCollision(const AABB& a, const AABB& b);
    void resolveAABBCollision(RigidBodyState& a, RigidBodyState& b);
    
    std::vector<RigidBodyState> m_bodies;
    QVector3D m_gravity;
};

}
