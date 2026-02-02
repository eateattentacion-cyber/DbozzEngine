/**************************************************************************/
/*  projectmanager.cpp                                                    */
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

#include "projectmanager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QProcess>
#include <QFileInfo>

ProjectManager::ProjectManager(QObject* parent)
    : QObject(parent)
{
    load();
}

QString ProjectManager::configPath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    dir = QDir::homePath() + "/.dabozzengine";
    QDir().mkpath(dir);
    return dir + "/hub.json";
}

void ProjectManager::load() {
    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonObject root = doc.object();

    m_projects.clear();
    QJsonArray projects = root["projects"].toArray();
    for (const auto& val : projects) {
        QJsonObject obj = val.toObject();
        ProjectInfo p;
        p.name = obj["name"].toString();
        p.path = obj["path"].toString();
        p.engineVersion = obj["engineVersion"].toString();
        p.lastOpened = QDateTime::fromString(obj["lastOpened"].toString(), Qt::ISODate);
        m_projects.append(p);
    }

    m_engineVersions.clear();
    QJsonArray versions = root["engineVersions"].toArray();
    for (const auto& val : versions) {
        QJsonObject obj = val.toObject();
        EngineVersion v;
        v.label = obj["label"].toString();
        v.editorPath = obj["editorPath"].toString();
        m_engineVersions.append(v);
    }
}

void ProjectManager::save() {
    QJsonArray projects;
    for (const auto& p : m_projects) {
        QJsonObject obj;
        obj["name"] = p.name;
        obj["path"] = p.path;
        obj["engineVersion"] = p.engineVersion;
        obj["lastOpened"] = p.lastOpened.toString(Qt::ISODate);
        projects.append(obj);
    }

    QJsonArray versions;
    for (const auto& v : m_engineVersions) {
        QJsonObject obj;
        obj["label"] = v.label;
        obj["editorPath"] = v.editorPath;
        versions.append(obj);
    }

    QJsonObject root;
    root["projects"] = projects;
    root["engineVersions"] = versions;

    QFile file(configPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void ProjectManager::addProject(const QString& name, const QString& path, const QString& engineVersion) {
    ProjectInfo p;
    p.name = name;
    p.path = path;
    p.engineVersion = engineVersion;
    p.lastOpened = QDateTime::currentDateTime();
    m_projects.prepend(p);
    save();
}

void ProjectManager::addExistingProject(const QString& path) {
    QFileInfo info(path);
    QString name = info.isDir() ? info.fileName() : info.dir().dirName();

    for (const auto& p : m_projects) {
        if (p.path == path) return;
    }

    ProjectInfo p;
    p.name = name;
    p.path = info.isDir() ? path : info.absolutePath();
    p.engineVersion = m_engineVersions.isEmpty() ? "default" : m_engineVersions.first().label;
    p.lastOpened = QDateTime::currentDateTime();
    m_projects.prepend(p);
    save();
}

void ProjectManager::removeProject(int index) {
    if (index >= 0 && index < m_projects.size()) {
        m_projects.removeAt(index);
        save();
    }
}

void ProjectManager::openProject(int index) {
    if (index < 0 || index >= m_projects.size()) return;

    ProjectInfo& p = m_projects[index];
    p.lastOpened = QDateTime::currentDateTime();
    save();

    QString editorPath;
    for (const auto& v : m_engineVersions) {
        if (v.label == p.engineVersion) {
            editorPath = v.editorPath;
            break;
        }
    }

    if (editorPath.isEmpty() && !m_engineVersions.isEmpty()) {
        editorPath = m_engineVersions.first().editorPath;
    }

    if (!editorPath.isEmpty()) {
        QProcess::startDetached(editorPath, {p.path});
        emit projectLaunched(p.path);
    }
}

void ProjectManager::addEngineVersion(const QString& label, const QString& editorPath) {
    for (const auto& v : m_engineVersions) {
        if (v.label == label) return;
    }

    EngineVersion v;
    v.label = label;
    v.editorPath = editorPath;
    m_engineVersions.append(v);
    save();
}

void ProjectManager::removeEngineVersion(int index) {
    if (index >= 0 && index < m_engineVersions.size()) {
        m_engineVersions.removeAt(index);
        save();
    }
}

bool ProjectManager::createProjectDirectory(const QString& path) {
    QDir dir(path);
    if (!dir.mkpath(".")) return false;

    // Create standard project folders
    dir.mkpath("Scenes");
    dir.mkpath("Assets");
    dir.mkpath("Scripts");

    // Create default scene in Scenes folder
    QFile sceneFile(path + "/Scenes/main.dabozz");
    if (sceneFile.open(QIODevice::WriteOnly)) {
        QJsonObject root;
        root["version"] = 1;
        root["entities"] = QJsonArray();
        sceneFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        sceneFile.close();
    }

    return true;
}
