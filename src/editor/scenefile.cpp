/**************************************************************************/
/*  scenefile.cpp                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                           DABOZZ ENGINE                                */
/**************************************************************************/
/* Copyright (c) 2026-present DabozzEngine contributors.                  */
/**************************************************************************/

#include "editor/scenefile.h"
#include "ecs/components/transform.h"
#include "ecs/components/name.h"
#include "ecs/components/hierarchy.h"
#include "ecs/components/mesh.h"
#include "ecs/components/rigidbody.h"
#include "ecs/components/boxcollider.h"
#include "ecs/components/spherecollider.h"
#include "ecs/components/firstpersoncontroller.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

bool SceneFile::saveScene(DabozzEngine::ECS::World* world, const QString& path)
{
    if (!world) return false;

    QJsonObject root;
    root["version"] = 1;

    QJsonArray entitiesArray;

    for (DabozzEngine::ECS::EntityID entity : world->getEntities()) {
        QJsonObject entityObj;
        entityObj["id"] = static_cast<int>(entity);

        QJsonObject components;

        auto* name = world->getComponent<DabozzEngine::ECS::Name>(entity);
        if (name) {
            QJsonObject nameObj;
            nameObj["name"] = name->name;
            components["Name"] = nameObj;
        }

        auto* transform = world->getComponent<DabozzEngine::ECS::Transform>(entity);
        if (transform) {
            QJsonObject tObj;
            QJsonArray pos = { transform->position.x(), transform->position.y(), transform->position.z() };
            QJsonArray rot = { transform->rotation.scalar(), transform->rotation.x(),
                               transform->rotation.y(), transform->rotation.z() };
            QJsonArray scl = { transform->scale.x(), transform->scale.y(), transform->scale.z() };
            tObj["position"] = pos;
            tObj["rotation"] = rot;
            tObj["scale"] = scl;
            components["Transform"] = tObj;
        }

        auto* hierarchy = world->getComponent<DabozzEngine::ECS::Hierarchy>(entity);
        if (hierarchy) {
            QJsonObject hObj;
            hObj["parent"] = static_cast<int>(hierarchy->parent);
            QJsonArray childArray;
            for (auto c : hierarchy->children) {
                childArray.append(static_cast<int>(c));
            }
            hObj["children"] = childArray;
            components["Hierarchy"] = hObj;
        }

        auto* rb = world->getComponent<DabozzEngine::ECS::RigidBody>(entity);
        if (rb) {
            QJsonObject rbObj;
            rbObj["mass"] = rb->mass;
            rbObj["isStatic"] = rb->isStatic;
            rbObj["useGravity"] = rb->useGravity;
            rbObj["drag"] = rb->drag;
            rbObj["angularDrag"] = rb->angularDrag;
            components["RigidBody"] = rbObj;
        }

        auto* bc = world->getComponent<DabozzEngine::ECS::BoxCollider>(entity);
        if (bc) {
            QJsonObject bcObj;
            QJsonArray sz = { bc->size.x(), bc->size.y(), bc->size.z() };
            bcObj["size"] = sz;
            bcObj["isTrigger"] = bc->isTrigger;
            components["BoxCollider"] = bcObj;
        }

        auto* sc = world->getComponent<DabozzEngine::ECS::SphereCollider>(entity);
        if (sc) {
            QJsonObject scObj;
            scObj["radius"] = sc->radius;
            scObj["isTrigger"] = sc->isTrigger;
            components["SphereCollider"] = scObj;
        }

        if (world->hasComponent<DabozzEngine::ECS::FirstPersonController>(entity)) {
            auto* fpc = world->getComponent<DabozzEngine::ECS::FirstPersonController>(entity);
            QJsonObject fpcObj;
            fpcObj["moveSpeed"] = fpc->moveSpeed;
            fpcObj["lookSpeed"] = fpc->lookSpeed;
            components["FirstPersonController"] = fpcObj;
        }

        auto* mesh = world->getComponent<DabozzEngine::ECS::Mesh>(entity);
        if (mesh) {
            QJsonObject meshObj;
            meshObj["modelPath"] = QString::fromStdString(mesh->modelPath);
            meshObj["texturePath"] = QString::fromStdString(mesh->texturePath);
            meshObj["hasTexture"] = mesh->hasTexture;
            meshObj["hasAnimation"] = mesh->hasAnimation;

            /* Store vertex data for procedural meshes (cubes, floors) that
               don't have a modelPath to reload from. */
            if (mesh->modelPath.empty()) {
                QJsonArray verts, norms, texcs, idxs;
                for (float v : mesh->vertices) verts.append(v);
                for (float n : mesh->normals) norms.append(n);
                for (float t : mesh->texCoords) texcs.append(t);
                for (unsigned int i : mesh->indices) idxs.append(static_cast<int>(i));
                meshObj["vertices"] = verts;
                meshObj["normals"] = norms;
                meshObj["texCoords"] = texcs;
                meshObj["indices"] = idxs;
            }
            components["Mesh"] = meshObj;
        }

        entityObj["components"] = components;
        entitiesArray.append(entityObj);
    }

    root["entities"] = entitiesArray;

    QJsonDocument doc(root);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool SceneFile::loadScene(DabozzEngine::ECS::World* world, const QString& path)
{
    if (!world) return false;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) return false;

    QJsonObject root = doc.object();

    /* Clear the world - destroy all existing entities */
    auto entities = world->getEntities();
    for (auto it = entities.rbegin(); it != entities.rend(); ++it) {
        world->destroyEntity(*it);
    }

    QJsonArray entitiesArray = root["entities"].toArray();

    /* First pass: create all entities with their IDs */
    std::unordered_map<int, DabozzEngine::ECS::EntityID> idMap;

    for (const auto& val : entitiesArray) {
        QJsonObject entityObj = val.toObject();
        int savedId = entityObj["id"].toInt();
        DabozzEngine::ECS::EntityID entity = world->createEntity();
        idMap[savedId] = entity;
    }

    /* Second pass: add components */
    for (const auto& val : entitiesArray) {
        QJsonObject entityObj = val.toObject();
        int savedId = entityObj["id"].toInt();
        DabozzEngine::ECS::EntityID entity = idMap[savedId];
        QJsonObject components = entityObj["components"].toObject();

        if (components.contains("Name")) {
            QJsonObject nameObj = components["Name"].toObject();
            world->addComponent<DabozzEngine::ECS::Name>(entity, nameObj["name"].toString());
        }

        if (components.contains("Transform")) {
            QJsonObject tObj = components["Transform"].toObject();
            auto* t = world->addComponent<DabozzEngine::ECS::Transform>(entity);
            QJsonArray pos = tObj["position"].toArray();
            QJsonArray rot = tObj["rotation"].toArray();
            QJsonArray scl = tObj["scale"].toArray();
            t->position = QVector3D(pos[0].toDouble(), pos[1].toDouble(), pos[2].toDouble());
            t->rotation = QQuaternion(rot[0].toDouble(), rot[1].toDouble(), rot[2].toDouble(), rot[3].toDouble());
            t->scale = QVector3D(scl[0].toDouble(), scl[1].toDouble(), scl[2].toDouble());
        }

        if (components.contains("Hierarchy")) {
            QJsonObject hObj = components["Hierarchy"].toObject();
            auto* h = world->addComponent<DabozzEngine::ECS::Hierarchy>(entity);
            int parentSavedId = hObj["parent"].toInt();
            h->parent = (parentSavedId != 0 && idMap.count(parentSavedId)) ? idMap[parentSavedId] : 0;
            QJsonArray childArray = hObj["children"].toArray();
            for (const auto& c : childArray) {
                int childSavedId = c.toInt();
                if (idMap.count(childSavedId)) {
                    h->children.push_back(idMap[childSavedId]);
                }
            }
        }

        if (components.contains("RigidBody")) {
            QJsonObject rbObj = components["RigidBody"].toObject();
            auto* rb = world->addComponent<DabozzEngine::ECS::RigidBody>(
                entity, rbObj["mass"].toDouble(), rbObj["isStatic"].toBool(), rbObj["useGravity"].toBool());
            rb->drag = rbObj["drag"].toDouble();
            rb->angularDrag = rbObj["angularDrag"].toDouble();
        }

        if (components.contains("BoxCollider")) {
            QJsonObject bcObj = components["BoxCollider"].toObject();
            QJsonArray sz = bcObj["size"].toArray();
            QVector3D size(sz[0].toDouble(), sz[1].toDouble(), sz[2].toDouble());
            world->addComponent<DabozzEngine::ECS::BoxCollider>(entity, size, bcObj["isTrigger"].toBool());
        }

        if (components.contains("SphereCollider")) {
            QJsonObject scObj = components["SphereCollider"].toObject();
            world->addComponent<DabozzEngine::ECS::SphereCollider>(
                entity, scObj["radius"].toDouble(), scObj["isTrigger"].toBool());
        }

        if (components.contains("FirstPersonController")) {
            QJsonObject fpcObj = components["FirstPersonController"].toObject();
            auto* fpc = world->addComponent<DabozzEngine::ECS::FirstPersonController>(entity);
            fpc->moveSpeed = fpcObj["moveSpeed"].toDouble();
            fpc->lookSpeed = fpcObj["lookSpeed"].toDouble();
        }

        if (components.contains("Mesh")) {
            QJsonObject meshObj = components["Mesh"].toObject();
            auto* mesh = world->addComponent<DabozzEngine::ECS::Mesh>(entity);
            mesh->modelPath = meshObj["modelPath"].toString().toStdString();
            mesh->texturePath = meshObj["texturePath"].toString().toStdString();
            mesh->hasTexture = meshObj["hasTexture"].toBool();
            mesh->hasAnimation = meshObj["hasAnimation"].toBool();

            if (meshObj.contains("vertices")) {
                mesh->vertices.clear();
                mesh->normals.clear();
                mesh->texCoords.clear();
                mesh->indices.clear();
                for (const auto& v : meshObj["vertices"].toArray()) mesh->vertices.push_back(v.toDouble());
                for (const auto& n : meshObj["normals"].toArray()) mesh->normals.push_back(n.toDouble());
                for (const auto& t : meshObj["texCoords"].toArray()) mesh->texCoords.push_back(t.toDouble());
                for (const auto& i : meshObj["indices"].toArray()) mesh->indices.push_back(i.toInt());
            }
        }
    }

    return true;
}
