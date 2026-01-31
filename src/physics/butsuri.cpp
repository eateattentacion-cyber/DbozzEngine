#include "physics/simplephysics.h"
#include "debug/logger.h"
#include <algorithm>

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
    
    // Calculate AABB
    QVector3D halfSize = size * 0.5f;
    body.bounds.min = position - halfSize;
    body.bounds.max = position + halfSize;
    
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
            
            if (checkAABBCollision(m_bodies[i].bounds, m_bodies[j].bounds)) {
                resolveAABBCollision(m_bodies[i], m_bodies[j]);
                
                // Update AABBs after resolution
                QVector3D sizeI = m_bodies[i].bounds.max - m_bodies[i].bounds.min;
                QVector3D halfSizeI = sizeI * 0.5f;
                m_bodies[i].bounds.min = m_bodies[i].position - halfSizeI;
                m_bodies[i].bounds.max = m_bodies[i].position + halfSizeI;
                
                QVector3D sizeJ = m_bodies[j].bounds.max - m_bodies[j].bounds.min;
                QVector3D halfSizeJ = sizeJ * 0.5f;
                m_bodies[j].bounds.min = m_bodies[j].position - halfSizeJ;
                m_bodies[j].bounds.max = m_bodies[j].position + halfSizeJ;
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
        
        // Update AABB
        QVector3D size = body.bounds.max - body.bounds.min;
        QVector3D halfSize = size * 0.5f;
        body.bounds.min = body.position - halfSize;
        body.bounds.max = body.position + halfSize;
        
        // Hard clamp to prevent falling through floor
        // If bottom of object is below -5 (floor level), snap it back up
        if (body.bounds.min.y() < -4.75f) {
            float correction = -4.75f - body.bounds.min.y();
            body.position.setY(body.position.y() + correction);
            body.bounds.min.setY(-4.75f);
            body.bounds.max.setY(body.bounds.min.y() + size.y());
            body.velocity.setY(0); // Stop downward velocity
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
        float slop = 0.001f; // Small penetration allowance
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
