/**************************************************************************/
/*  scenefile.h                                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                           DABOZZ ENGINE                                */
/**************************************************************************/
/* Copyright (c) 2026-present DabozzEngine contributors.                  */
/**************************************************************************/

#pragma once

#include <QString>
#include "ecs/world.h"

/**
 * @brief Handles scene serialization to/from JSON (.dabozz files).
 */
class SceneFile {
public:
    /**
     * @brief Save all entities and their components to a JSON file.
     * @param world The ECS world to serialize.
     * @param path Output file path.
     * @return True on success.
     */
    static bool saveScene(DabozzEngine::ECS::World* world, const QString& path);

    /**
     * @brief Load entities and components from a JSON file into the world.
     * @param world The ECS world to populate (will be cleared first).
     * @param path Input file path.
     * @return True on success.
     */
    static bool loadScene(DabozzEngine::ECS::World* world, const QString& path);
};
