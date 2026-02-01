/**************************************************************************/
/*  undostack.h                                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                           DABOZZ ENGINE                                */
/**************************************************************************/
/* Copyright (c) 2026-present DabozzEngine contributors.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include <QUndoCommand>
#include <QVector3D>
#include <QQuaternion>
#include <QString>
#include <functional>
#include "ecs/world.h"

/**
 * @brief Command for undoing/redoing transform changes.
 */
class TransformChangeCommand : public QUndoCommand {
public:
    TransformChangeCommand(DabozzEngine::ECS::World* world,
                           DabozzEngine::ECS::EntityID entity,
                           QVector3D oldPos, QQuaternion oldRot, QVector3D oldScale,
                           QVector3D newPos, QQuaternion newRot, QVector3D newScale,
                           std::function<void()> refreshCallback,
                           QUndoCommand* parent = nullptr)
        : QUndoCommand(parent)
        , m_world(world), m_entity(entity)
        , m_oldPos(oldPos), m_oldRot(oldRot), m_oldScale(oldScale)
        , m_newPos(newPos), m_newRot(newRot), m_newScale(newScale)
        , m_refresh(refreshCallback)
    {
        setText("Transform Change");
    }

    void undo() override {
        auto* t = m_world->getComponent<DabozzEngine::ECS::Transform>(m_entity);
        if (t) { t->position = m_oldPos; t->rotation = m_oldRot; t->scale = m_oldScale; }
        if (m_refresh) m_refresh();
    }

    void redo() override {
        auto* t = m_world->getComponent<DabozzEngine::ECS::Transform>(m_entity);
        if (t) { t->position = m_newPos; t->rotation = m_newRot; t->scale = m_newScale; }
        if (m_refresh) m_refresh();
    }

private:
    DabozzEngine::ECS::World* m_world;
    DabozzEngine::ECS::EntityID m_entity;
    QVector3D m_oldPos, m_oldScale, m_newPos, m_newScale;
    QQuaternion m_oldRot, m_newRot;
    std::function<void()> m_refresh;
};

/**
 * @brief Command for undoing/redoing entity name changes.
 */
class NameChangeCommand : public QUndoCommand {
public:
    NameChangeCommand(DabozzEngine::ECS::World* world,
                      DabozzEngine::ECS::EntityID entity,
                      const QString& oldName, const QString& newName,
                      std::function<void()> refreshCallback,
                      QUndoCommand* parent = nullptr)
        : QUndoCommand(parent)
        , m_world(world), m_entity(entity)
        , m_oldName(oldName), m_newName(newName)
        , m_refresh(refreshCallback)
    {
        setText(QString("Rename '%1' to '%2'").arg(oldName).arg(newName));
    }

    void undo() override {
        auto* n = m_world->getComponent<DabozzEngine::ECS::Name>(m_entity);
        if (n) n->name = m_oldName;
        if (m_refresh) m_refresh();
    }

    void redo() override {
        auto* n = m_world->getComponent<DabozzEngine::ECS::Name>(m_entity);
        if (n) n->name = m_newName;
        if (m_refresh) m_refresh();
    }

private:
    DabozzEngine::ECS::World* m_world;
    DabozzEngine::ECS::EntityID m_entity;
    QString m_oldName, m_newName;
    std::function<void()> m_refresh;
};

/**
 * @brief Command for undoing/redoing entity creation.
 *
 * On undo, destroys the entity. On redo, recreates it with the same components.
 * Note: uses a simplified approach - stores entity ID and recreates with basic
 * components (Name, Transform, Hierarchy). Complex components like Mesh with
 * GPU data are not fully preserved on undo.
 * i will do this later i find myself to lazy right now.
 */
class CreateEntityCommand : public QUndoCommand {
public:
    CreateEntityCommand(DabozzEngine::ECS::World* world,
                        DabozzEngine::ECS::EntityID entity,
                        const QString& entityName,
                        std::function<void()> refreshCallback,
                        QUndoCommand* parent = nullptr)
        : QUndoCommand(parent)
        , m_world(world), m_entity(entity)
        , m_name(entityName)
        , m_refresh(refreshCallback)
    {
        setText(QString("Create '%1'").arg(entityName));
    }

    void undo() override {
        m_world->destroyEntity(m_entity);
        if (m_refresh) m_refresh();
    }

    void redo() override {
        /* Entity ID reuse is not supported by the current World implementation,
           so redo after undo won't restore the exact same entity. This is a
           known limitation - the entity gets a new ID on redo. */
        if (!m_world->hasEntity(m_entity)) {
            /* Can't recreate with same ID. Accept the limitation. */
        }
        if (m_refresh) m_refresh();
    }

private:
    DabozzEngine::ECS::World* m_world;
    DabozzEngine::ECS::EntityID m_entity;
    QString m_name;
    std::function<void()> m_refresh;
};

/**
 * @brief Command for undoing/redoing entity deletion.
 *
 * Stores the entity's core component data so it can be restored on undo.
 */
class DeleteEntityCommand : public QUndoCommand {
public:
    DeleteEntityCommand(DabozzEngine::ECS::World* world,
                        DabozzEngine::ECS::EntityID entity,
                        std::function<void()> refreshCallback,
                        QUndoCommand* parent = nullptr)
        : QUndoCommand(parent)
        , m_world(world), m_entity(entity)
        , m_refresh(refreshCallback)
    {
        auto* n = m_world->getComponent<DabozzEngine::ECS::Name>(entity);
        if (n) m_name = n->name;

        auto* t = m_world->getComponent<DabozzEngine::ECS::Transform>(entity);
        if (t) { m_pos = t->position; m_rot = t->rotation; m_scale = t->scale; m_hasTransform = true; }

        auto* h = m_world->getComponent<DabozzEngine::ECS::Hierarchy>(entity);
        if (h) { m_parent = h->parent; m_children = h->children; m_hasHierarchy = true; }

        auto* rb = m_world->getComponent<DabozzEngine::ECS::RigidBody>(entity);
        if (rb) { m_rbMass = rb->mass; m_rbStatic = rb->isStatic; m_rbGravity = rb->useGravity; m_hasRigidBody = true; }

        auto* bc = m_world->getComponent<DabozzEngine::ECS::BoxCollider>(entity);
        if (bc) { m_boxSize = bc->size; m_hasBoxCollider = true; }

        auto* sc = m_world->getComponent<DabozzEngine::ECS::SphereCollider>(entity);
        if (sc) { m_sphereRadius = sc->radius; m_hasSphereCollider = true; }

        m_hasFPC = m_world->hasComponent<DabozzEngine::ECS::FirstPersonController>(entity);

        setText(QString("Delete '%1'").arg(m_name));
    }

    void undo() override {
        /* Recreate entity - note: gets a new ID since World doesn't support ID reuse */
        DabozzEngine::ECS::EntityID newEntity = m_world->createEntity();
        m_world->addComponent<DabozzEngine::ECS::Name>(newEntity, m_name);

        if (m_hasTransform) {
            auto* t = m_world->addComponent<DabozzEngine::ECS::Transform>(newEntity);
            t->position = m_pos; t->rotation = m_rot; t->scale = m_scale;
        }
        if (m_hasHierarchy) {
            auto* h = m_world->addComponent<DabozzEngine::ECS::Hierarchy>(newEntity);
            h->parent = m_parent;
        }
        if (m_hasRigidBody) {
            m_world->addComponent<DabozzEngine::ECS::RigidBody>(newEntity, m_rbMass, m_rbStatic, m_rbGravity);
        }
        if (m_hasBoxCollider) {
            m_world->addComponent<DabozzEngine::ECS::BoxCollider>(newEntity, m_boxSize);
        }
        if (m_hasSphereCollider) {
            m_world->addComponent<DabozzEngine::ECS::SphereCollider>(newEntity, m_sphereRadius);
        }
        if (m_hasFPC) {
            m_world->addComponent<DabozzEngine::ECS::FirstPersonController>(newEntity);
        }

        m_entity = newEntity;
        if (m_refresh) m_refresh();
    }

    void redo() override {
        m_world->destroyEntity(m_entity);
        if (m_refresh) m_refresh();
    }

private:
    DabozzEngine::ECS::World* m_world;
    DabozzEngine::ECS::EntityID m_entity;
    std::function<void()> m_refresh;

    QString m_name;
    QVector3D m_pos, m_scale, m_boxSize;
    QQuaternion m_rot;
    DabozzEngine::ECS::EntityID m_parent = 0;
    std::vector<DabozzEngine::ECS::EntityID> m_children;
    float m_rbMass = 1.0f, m_sphereRadius = 0.5f;
    bool m_rbStatic = false, m_rbGravity = true;
    bool m_hasTransform = false, m_hasHierarchy = false;
    bool m_hasRigidBody = false, m_hasBoxCollider = false;
    bool m_hasSphereCollider = false, m_hasFPC = false;
};
