/**************************************************************************/
/* assetbrowser.cpp                                                       */
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
#include "editor/assetbrowser.h"
#include <QHeaderView>

AssetBrowser::AssetBrowser(QWidget* parent)
 : QWidget(parent)
 {
    setupUI();
 }

 void AssetBrowser::setupUI()
 {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(4);

    m_searchBar = new QLineEdit();
    m_searchBar->setText("Search Assets...."); // will come back to this later.(make it work.)
    m_layout->addWidget(m_searchBar);

    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setNameFilters({
        "*.obj", "*.fbx", "*.gltf", "*.glb", "*.dae",
          "*.png", "*.jpg", "*.bmp", "*.tga",
          "*.dabozz", "*.cs"    
        });
        m_fileModel->setNameFilterDisables(false);

        m_treeView = new QTreeView();
        m_treeView->setModel(m_fileModel);
        m_treeView->setAnimated(true);
        m_treeView->setSortingEnabled(true);
        m_treeView->header()->setStretchLastSection(true);
        m_treeView->setColumnWidth(0, 250);
        m_layout->addWidget(m_treeView);

        connect(m_treeView, &QTreeView::doubleClicked, this,
            &AssetBrowser::onItemDoubleClicked);
            connect(m_searchBar, &QLineEdit::textChanged, this,
                &AssetBrowser::onFilterChanged);
}

void AssetBrowser::setRootPath(const QString& path)
{
    m_fileModel->setRootPath(path);
    m_treeView->setRootIndex(m_fileModel->index(path));
}

void AssetBrowser::onItemDoubleClicked(const QModelIndex& index)
{
    QString filePath = m_fileModel->filePath(index);
    if (!m_fileModel->isDir(index)) {
        emit assetDoubleClicked(filePath);
    }
}

void AssetBrowser::onFilterChanged(const QString& text)
{
    if (text.isEmpty()) {
        m_fileModel->setNameFilters({
        "*.obj", "*.fbx", "*.gltf", "*.glb", "*.dae",
          "*.png", "*.jpg", "*.bmp", "*.tga",
          "*.dabozz", "*.cs"    
        });
    } else {
        m_fileModel->setNameFilters({QString("*%1*").arg(text)});
    }
}