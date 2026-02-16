#include "physics/simplephysics.h"
#include "debug/logger.h"
#include <algorithm>
#include <limits>
#include <cmath>

namespace DabozzEngine::Physics {

static ButsuriEngine* g_instance = nullptr;

ButsuriEngine::ButsuriEngine()
    : m_gravity(0.0f, -9.81f, 0.0f)
{
    g_instance = this;
}

ButsuriEngine::~ButsuriEngine()
{
    shutdown();
    g_instance = nullptr;
}

void ButsuriEngine::initialize()
{
    DEBUG_LOG << "Butsuri Engine initialized" << std::endl;
    m_bodies.clear();
}

void ButsuriEngine::shutdown()
{
    m_bodies.clear();
}

void ButsuriEngine::update(float deltaTime)
{
    integrateVelocities(deltaTime);
    integratePositions(deltaTime);
    
    // Multiple collision resolution iterations to prevent sinking
    for (int i = 0; i < 4; i++) {
        detectCollisions();
        resolveCollisions();
    }
}

int ButsuriEngine::createBody(const QVector3D& position, const QVector3D& size, float mass, bool isStatic)
{
    RigidBodyState body;
    body.position = position;
    body.rotation = QQuaternion();
    body.velocity = QVector3D(0, 0, 0);
    body.angularVelocity = QVector3D(0, 0, 0);
    body.mass = mass;
    body.inverseMass = isStatic ? 0.0f : (mass > 0.0f ? 1.0f / mass : 0.0f);
    body.isStatic = isStatic;
    body.isSleeping = false;
    body.colliderType = ColliderType::Box;
    
    // Calculate AABB
    QVector3D halfSize = size * 0.5f;
    body.bounds.min = position - halfSize;
    body.bounds.max = position + halfSize;
    
    m_bodies.push_back(body);
    return m_bodies.size() - 1;
}

int ButsuriEngine::createSphereBody(const QVector3D& position, float radius, float mass, bool isStatic)
{
    RigidBodyState body;
    body.position = position;
    body.rotation = QQuaternion();
    body.velocity = QVector3D(0, 0, 0);
    body.angularVelocity = QVector3D(0, 0, 0);
    body.mass = mass;
    body.inverseMass = isStatic ? 0.0f : (mass > 0.0f ? 1.0f / mass : 0.0f);
    body.isStatic = isStatic;
    body.isSleeping = false;
    body.colliderType = ColliderType::Sphere;
    
    // Setup sphere
    body.sphere.center = position;
    body.sphere.radius = radius;
    
    // Also create AABB for broad phase
    body.bounds.min = position - QVector3D(radius, radius, radius);
    body.bounds.max = position + QVector3D(radius, radius, radius);
    
    m_bodies.push_back(body);
    return m_bodies.size() - 1;
}

void ButsuriEngine::removeBody(int bodyId)
{
    if (bodyId >= 0 && bodyId < (int)m_bodies.size()) {
        m_bodies.erase(m_bodies.begin() + bodyId);
    }
}

RigidBodyState* ButsuriEngine::getBody(int bodyId)
{
    if (bodyId >= 0 && bodyId < (int)m_bodies.size()) {
        return &m_bodies[bodyId];
    }
    return nullptr;
}

ButsuriEngine* ButsuriEngine::getInstance()
{
    return g_instance;
}

void ButsuriEngine::integrateVelocities(float deltaTime)
{
    for (auto& body : m_bodies) {
        if (body.isStatic) continue;
        
        // Apply gravity
        body.velocity += m_gravity * deltaTime;
    }
}

void ButsuriEngine::detectCollisions()
{
    // Simple broad phase - check all pairs
    // TODO: Add spatial partitioning for better performance
}

void ButsuriEngine::resolveCollisions()
{
    // Check all body pairs
    for (size_t i = 0; i < m_bodies.size(); i++) {
        for (size_t j = i + 1; j < m_bodies.size(); j++) {
            if (m_bodies[i].isStatic && m_bodies[j].isStatic) continue;
            
            // Broad phase - AABB check first
            if (!checkAABBCollision(m_bodies[i].bounds, m_bodies[j].bounds)) continue;
            
            // Narrow phase - check actual collider types
            bool colliding = false;
            if (m_bodies[i].colliderType == ColliderType::Box && m_bodies[j].colliderType == ColliderType::Box) {
                colliding = true;
                resolveAABBCollision(m_bodies[i], m_bodies[j]);
            } else if (m_bodies[i].colliderType == ColliderType::Sphere && m_bodies[j].colliderType == ColliderType::Sphere) {
                if (checkSphereCollision(m_bodies[i].sphere, m_bodies[j].sphere)) {
                    resolveSphereCollision(m_bodies[i], m_bodies[j]);
                    colliding = true;
                }
            } else {
                // One box, one sphere
                RigidBodyState& box = (m_bodies[i].colliderType == ColliderType::Box) ? m_bodies[i] : m_bodies[j];
                RigidBodyState& sphere = (m_bodies[i].colliderType == ColliderType::Sphere) ? m_bodies[i] : m_bodies[j];
                if (checkAABBSphereCollision(box.bounds, sphere.sphere)) {
                    resolveAABBSphereCollision(box, sphere);
                    colliding = true;
                }
            }
            
            if (colliding) {
                // Update AABBs after resolution
                if (m_bodies[i].colliderType == ColliderType::Box) {
                    QVector3D sizeI = m_bodies[i].bounds.max - m_bodies[i].bounds.min;
                    QVector3D halfSizeI = sizeI * 0.5f;
                    m_bodies[i].bounds.min = m_bodies[i].position - halfSizeI;
                    m_bodies[i].bounds.max = m_bodies[i].position + halfSizeI;
                } else {
                    m_bodies[i].sphere.center = m_bodies[i].position;
                    float r = m_bodies[i].sphere.radius;
                    m_bodies[i].bounds.min = m_bodies[i].position - QVector3D(r, r, r);
                    m_bodies[i].bounds.max = m_bodies[i].position + QVector3D(r, r, r);
                }
                
                if (m_bodies[j].colliderType == ColliderType::Box) {
                    QVector3D sizeJ = m_bodies[j].bounds.max - m_bodies[j].bounds.min;
                    QVector3D halfSizeJ = sizeJ * 0.5f;
                    m_bodies[j].bounds.min = m_bodies[j].position - halfSizeJ;
                    m_bodies[j].bounds.max = m_bodies[j].position + halfSizeJ;
                } else {
                    m_bodies[j].sphere.center = m_bodies[j].position;
                    float r = m_bodies[j].sphere.radius;
                    m_bodies[j].bounds.min = m_bodies[j].position - QVector3D(r, r, r);
                    m_bodies[j].bounds.max = m_bodies[j].position + QVector3D(r, r, r);
                }
            }
        }
    }
}

void ButsuriEngine::integratePositions(float deltaTime)
{
    for (auto& body : m_bodies) {
        if (body.isStatic) continue;
        
        // Update position
        body.position += body.velocity * deltaTime;
        
        // Update collider and AABB based on type
        if (body.colliderType == ColliderType::Box) {
            QVector3D size = body.bounds.max - body.bounds.min;
            QVector3D halfSize = size * 0.5f;
            body.bounds.min = body.position - halfSize;
            body.bounds.max = body.position + halfSize;
            
            // Hard clamp to prevent falling through floor
            if (body.bounds.min.y() < -4.75f) {
                float correction = -4.75f - body.bounds.min.y();
                body.position.setY(body.position.y() + correction);
                body.bounds.min.setY(-4.75f);
                body.bounds.max.setY(body.bounds.min.y() + size.y());
                body.velocity.setY(0);
            }
        } else {
            // Sphere
            body.sphere.center = body.position;
            float r = body.sphere.radius;
            body.bounds.min = body.position - QVector3D(r, r, r);
            body.bounds.max = body.position + QVector3D(r, r, r);
            
            // Hard clamp for sphere
            if (body.position.y() - r < -4.75f) {
                body.position.setY(-4.75f + r);
                body.sphere.center = body.position;
                body.bounds.min = body.position - QVector3D(r, r, r);
                body.bounds.max = body.position + QVector3D(r, r, r);
                body.velocity.setY(0);
            }
        }
    }
}

bool ButsuriEngine::checkAABBCollision(const AABB& a, const AABB& b)
{
    return (a.min.x() <= b.max.x() && a.max.x() >= b.min.x()) &&
           (a.min.y() <= b.max.y() && a.max.y() >= b.min.y()) &&
           (a.min.z() <= b.max.z() && a.max.z() >= b.min.z());
}

void ButsuriEngine::resolveAABBCollision(RigidBodyState& a, RigidBodyState& b)
{
    // Calculate center-to-center vector
    QVector3D centerA = (a.bounds.min + a.bounds.max) * 0.5f;
    QVector3D centerB = (b.bounds.min + b.bounds.max) * 0.5f;
    QVector3D delta = centerB - centerA;
    
    // Calculate overlap on each axis
    float overlapX = std::min(a.bounds.max.x() - b.bounds.min.x(), b.bounds.max.x() - a.bounds.min.x());
    float overlapY = std::min(a.bounds.max.y() - b.bounds.min.y(), b.bounds.max.y() - a.bounds.min.y());
    float overlapZ = std::min(a.bounds.max.z() - b.bounds.min.z(), b.bounds.max.z() - a.bounds.min.z());
    
    // Find minimum overlap axis
    QVector3D normal;
    float penetration;
    
    if (overlapX < overlapY && overlapX < overlapZ) {
        penetration = overlapX;
        normal = QVector3D(delta.x() > 0 ? 1.0f : -1.0f, 0, 0);
    } else if (overlapY < overlapZ) {
        penetration = overlapY;
        normal = QVector3D(0, delta.y() > 0 ? 1.0f : -1.0f, 0);
    } else {
        penetration = overlapZ;
        normal = QVector3D(0, 0, delta.z() > 0 ? 1.0f : -1.0f);
    }
    
    // Separate bodies more aggressively
    float totalInverseMass = a.inverseMass + b.inverseMass;
    if (totalInverseMass > 0.0f) {
        float percent = 1.0f; // Full penetration correction
        float slop = 0.001f;
        QVector3D correction = normal * std::max(penetration - slop, 0.0f) * percent;
        
        if (!a.isStatic) {
            a.position -= correction * (a.inverseMass / totalInverseMass);
        }
        if (!b.isStatic) {
            b.position += correction * (b.inverseMass / totalInverseMass);
        }
    }
    
    // Apply impulse with friction
    float restitution = 0.2f; // Low bounciness
    QVector3D relativeVelocity = b.velocity - a.velocity;
    float velocityAlongNormal = QVector3D::dotProduct(relativeVelocity, normal);
    
    // Only resolve if objects are moving towards each other
    if (velocityAlongNormal < 0) {
        float j = -(1.0f + restitution) * velocityAlongNormal;
        j /= totalInverseMass;
        
        QVector3D impulse = normal * j;
        
        if (!a.isStatic) {
            a.velocity -= impulse * a.inverseMass;
            // Stop small velocities to prevent jitter
            if (a.velocity.length() < 0.05f) {
                a.velocity = QVector3D(0, 0, 0);
            }
        }
        if (!b.isStatic) {
            b.velocity += impulse * b.inverseMass;
            if (b.velocity.length() < 0.05f) {
                b.velocity = QVector3D(0, 0, 0);
            }
        }
    }
}

}

bool DabozzEngine::Physics::ButsuriEngine::checkSphereCollision(const Sphere& a, const Sphere& b)
{
    float distance = (a.center - b.center).length();
    return distance < (a.radius + b.radius);
}

bool DabozzEngine::Physics::ButsuriEngine::checkAABBSphereCollision(const AABB& aabb, const Sphere& sphere)
{
    // Find closest point on AABB to sphere center
    QVector3D closest;
    closest.setX(std::max(aabb.min.x(), std::min(sphere.center.x(), aabb.max.x())));
    closest.setY(std::max(aabb.min.y(), std::min(sphere.center.y(), aabb.max.y())));
    closest.setZ(std::max(aabb.min.z(), std::min(sphere.center.z(), aabb.max.z())));
    
    float distance = (closest - sphere.center).length();
    return distance < sphere.radius;
}

void DabozzEngine::Physics::ButsuriEngine::resolveSphereCollision(RigidBodyState& a, RigidBodyState& b)
{
    QVector3D delta = b.position - a.position;
    float distance = delta.length();
    float overlap = (a.sphere.radius + b.sphere.radius) - distance;
    
    if (overlap <= 0) return;
    
    QVector3D normal = delta.normalized();
    
    // Separate spheres
    float totalInverseMass = a.inverseMass + b.inverseMass;
    if (totalInverseMass > 0.0f) {
        QVector3D correction = normal * overlap;
        if (!a.isStatic) {
            a.position -= correction * (a.inverseMass / totalInverseMass);
        }
        if (!b.isStatic) {
            b.position += correction * (b.inverseMass / totalInverseMass);
        }
    }
    
    // Apply impulse
    float restitution = 0.3f;
    QVector3D relativeVelocity = b.velocity - a.velocity;
    float velocityAlongNormal = QVector3D::dotProduct(relativeVelocity, normal);
    
    if (velocityAlongNormal < 0) {
        float j = -(1.0f + restitution) * velocityAlongNormal;
        j /= totalInverseMass;
        
        QVector3D impulse = normal * j;
        
        if (!a.isStatic) {
            a.velocity -= impulse * a.inverseMass;
        }
        if (!b.isStatic) {
            b.velocity += impulse * b.inverseMass;
        }
    }
}

void DabozzEngine::Physics::ButsuriEngine::resolveAABBSphereCollision(RigidBodyState& box, RigidBodyState& sphere)
{
    // Find closest point on box to sphere
    QVector3D closest;
    closest.setX(std::max(box.bounds.min.x(), std::min(sphere.position.x(), box.bounds.max.x())));
    closest.setY(std::max(box.bounds.min.y(), std::min(sphere.position.y(), box.bounds.max.y())));
    closest.setZ(std::max(box.bounds.min.z(), std::min(sphere.position.z(), box.bounds.max.z())));
    
    QVector3D delta = sphere.position - closest;
    float distance = delta.length();
    float overlap = sphere.sphere.radius - distance;
    
    if (overlap <= 0) return;
    
    QVector3D normal = (distance > 0.0001f) ? delta.normalized() : QVector3D(0, 1, 0);
    
    // Separate
    float totalInverseMass = box.inverseMass + sphere.inverseMass;
    if (totalInverseMass > 0.0f) {
        QVector3D correction = normal * overlap;
        if (!box.isStatic) {
            box.position -= correction * (box.inverseMass / totalInverseMass);
        }
        if (!sphere.isStatic) {
            sphere.position += correction * (sphere.inverseMass / totalInverseMass);
        }
    }
    
    // Apply impulse
    float restitution = 0.3f;
    QVector3D relativeVelocity = sphere.velocity - box.velocity;
    float velocityAlongNormal = QVector3D::dotProduct(relativeVelocity, normal);
    
    if (velocityAlongNormal < 0) {
        float j = -(1.0f + restitution) * velocityAlongNormal;
        j /= totalInverseMass;
        
        QVector3D impulse = normal * j;
        
        if (!box.isStatic) {
            box.velocity -= impulse * box.inverseMass;
        }
        if (!sphere.isStatic) {
            sphere.velocity += impulse * sphere.inverseMass;
        }
    }
}

DabozzEngine::Physics::ButsuriEngine::RaycastHit DabozzEngine::Physics::ButsuriEngine::raycast(const QVector3D& origin, const QVector3D& direction, float maxDistance)
{
    RaycastHit result;
    result.hit = false;
    result.distance = maxDistance;
    result.bodyId = -1;
    
    QVector3D dir = direction.normalized();
    
    for (size_t i = 0; i < m_bodies.size(); i++) {
        float t = 0.0f;
        bool hit = false;
        
        if (m_bodies[i].colliderType == ColliderType::Box) {
            hit = rayAABBIntersect(origin, dir, m_bodies[i].bounds, t);
        } else {
            hit = raySphereIntersect(origin, dir, m_bodies[i].sphere, t);
        }
        
        if (hit && t < result.distance && t >= 0) {
            result.hit = true;
            result.distance = t;
            result.point = origin + dir * t;
            result.bodyId = i;
            
            // Calculate normal (simplified)
            if (m_bodies[i].colliderType == ColliderType::Sphere) {
                result.normal = (result.point - m_bodies[i].sphere.center).normalized();
            } else {
                // Box normal approximation
                QVector3D center = (m_bodies[i].bounds.min + m_bodies[i].bounds.max) * 0.5f;
                QVector3D delta = result.point - center;
                QVector3D absD(std::abs(delta.x()), std::abs(delta.y()), std::abs(delta.z()));
                
                if (absD.x() > absD.y() && absD.x() > absD.z()) {
                    result.normal = QVector3D(delta.x() > 0 ? 1 : -1, 0, 0);
                } else if (absD.y() > absD.z()) {
                    result.normal = QVector3D(0, delta.y() > 0 ? 1 : -1, 0);
                } else {
                    result.normal = QVector3D(0, 0, delta.z() > 0 ? 1 : -1);
                }
            }
        }
    }
    
    return result;
}

bool DabozzEngine::Physics::ButsuriEngine::rayAABBIntersect(const QVector3D& origin, const QVector3D& direction, const AABB& aabb, float& t)
{
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::max();
    
    for (int i = 0; i < 3; i++) {
        float o = (i == 0) ? origin.x() : (i == 1) ? origin.y() : origin.z();
        float d = (i == 0) ? direction.x() : (i == 1) ? direction.y() : direction.z();
        float bmin = (i == 0) ? aabb.min.x() : (i == 1) ? aabb.min.y() : aabb.min.z();
        float bmax = (i == 0) ? aabb.max.x() : (i == 1) ? aabb.max.y() : aabb.max.z();
        
        if (std::abs(d) < 0.0001f) {
            if (o < bmin || o > bmax) return false;
        } else {
            float t1 = (bmin - o) / d;
            float t2 = (bmax - o) / d;
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return false;
        }
    }
    
    t = tmin;
    return true;
}

bool DabozzEngine::Physics::ButsuriEngine::raySphereIntersect(const QVector3D& origin, const QVector3D& direction, const Sphere& sphere, float& t)
{
    QVector3D oc = origin - sphere.center;
    float a = QVector3D::dotProduct(direction, direction);
    float b = 2.0f * QVector3D::dotProduct(oc, direction);
    float c = QVector3D::dotProduct(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) return false;
    
    t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    return t >= 0;
}
