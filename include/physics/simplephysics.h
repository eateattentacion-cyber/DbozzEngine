#pragma once
#include <QVector3D>
#include <QQuaternion>
#include <vector>

namespace DabozzEngine::Physics {

enum class ColliderType {
    Box,
    Sphere
};

struct AABB {
    QVector3D min;
    QVector3D max;
};

struct Sphere {
    QVector3D center;
    float radius;
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
    ColliderType colliderType;
    AABB bounds;
    Sphere sphere;
};

class ButsuriEngine {
public:
    ButsuriEngine();
    ~ButsuriEngine();
    
    void initialize();
    void shutdown();
    void update(float deltaTime);
    
    int createBody(const QVector3D& position, const QVector3D& size, float mass, bool isStatic);
    int createSphereBody(const QVector3D& position, float radius, float mass, bool isStatic);
    void removeBody(int bodyId);
    
    RigidBodyState* getBody(int bodyId);
    
    struct RaycastHit {
        bool hit;
        QVector3D point;
        QVector3D normal;
        float distance;
        int bodyId;
    };
    
    RaycastHit raycast(const QVector3D& origin, const QVector3D& direction, float maxDistance = 1000.0f);
    
    void setGravity(const QVector3D& gravity) { m_gravity = gravity; }
    QVector3D getGravity() const { return m_gravity; }
    
    static ButsuriEngine* getInstance();
    
private:
    void integrateVelocities(float deltaTime);
    void detectCollisions();
    void resolveCollisions();
    void integratePositions(float deltaTime);
    
    bool checkAABBCollision(const AABB& a, const AABB& b);
    bool checkSphereCollision(const Sphere& a, const Sphere& b);
    bool checkAABBSphereCollision(const AABB& aabb, const Sphere& sphere);
    void resolveAABBCollision(RigidBodyState& a, RigidBodyState& b);
    void resolveSphereCollision(RigidBodyState& a, RigidBodyState& b);
    void resolveAABBSphereCollision(RigidBodyState& box, RigidBodyState& sphere);
    
    bool rayAABBIntersect(const QVector3D& origin, const QVector3D& direction, const AABB& aabb, float& t);
    bool raySphereIntersect(const QVector3D& origin, const QVector3D& direction, const Sphere& sphere, float& t);
    
    std::vector<RigidBodyState> m_bodies;
    QVector3D m_gravity;
};

}
