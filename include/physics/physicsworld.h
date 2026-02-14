#pragma once

namespace DabozzEngine::Physics {

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();
    
    void initialize();
    void shutdown();
    void update(float deltaTime);
    
    static PhysicsWorld* getInstance();
    
private:
};

}
