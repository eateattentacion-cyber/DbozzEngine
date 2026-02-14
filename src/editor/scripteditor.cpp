/////////////////////////////////////////////////////////////////////////////
// scripteditor.cpp                                                        //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#include "editor/scripteditor.h"
#include "scripting/scriptengine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>

ScriptHighlighter::ScriptHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
    , m_language(Lua)
{
    keywordFormat.setForeground(QColor(86, 156, 214));
    keywordFormat.setFontWeight(QFont::Bold);

    classFormat.setForeground(QColor(78, 201, 176));
    classFormat.setFontWeight(QFont::Bold);

    commentFormat.setForeground(QColor(106, 153, 85));
    commentFormat.setFontItalic(true);

    stringFormat.setForeground(QColor(206, 145, 120));

    functionFormat.setForeground(QColor(220, 220, 170));

    numberFormat.setForeground(QColor(181, 206, 168));

    setupLuaRules();
}

void ScriptHighlighter::setLanguage(Language lang)
{
    m_language = lang;
    highlightingRules.clear();

    if (m_language == Lua) {
        setupLuaRules();
    } else {
        setupAngelScriptRules();
    }

    rehighlight();
}

void ScriptHighlighter::setupLuaRules()
{
    QStringList luaKeywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for",
        "function", "if", "in", "local", "nil", "not", "or", "repeat",
        "return", "then", "true", "until", "while"
    };

    for (const QString& keyword : luaKeywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    HighlightingRule rule;
    rule.pattern = QRegularExpression("--[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\".*?\"|'.*?'");
    rule.format = stringFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
}

void ScriptHighlighter::setupAngelScriptRules()
{
    QStringList asKeywords = {
        "void", "int", "float", "bool", "string", "class", "interface",
        "if", "else", "for", "while", "do", "switch", "case", "default",
        "break", "continue", "return", "true", "false", "null", "const",
        "private", "protected", "public", "namespace", "import", "from"
    };

    for (const QString& keyword : asKeywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    HighlightingRule rule;
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("/\\*.*?\\*/");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\".*?\"|'.*?'");
    rule.format = stringFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*f?\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\\b[A-Z][A-Za-z0-9_]*\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\\b[a-z_][A-Za-z0-9_]*(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
}

void ScriptHighlighter::highlightBlock(const QString& text)
{
    for (const HighlightingRule& rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

ScriptEditor::ScriptEditor(QWidget* parent)
    : QWidget(parent)
    , m_isModified(false)
    , m_currentLanguage(ScriptHighlighter::Lua)
    , m_scriptEngine(nullptr)
{
    setupUI();
    
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(5000);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &ScriptEditor::autoSave);
    m_autoSaveTimer->start();
    
    m_scriptEngine = new DabozzEngine::Scripting::ScriptEngine();
    m_scriptEngine->initialize();
}

ScriptEditor::~ScriptEditor()
{
    if (m_scriptEngine) {
        m_scriptEngine->shutdown();
        delete m_scriptEngine;
    }
}

void ScriptEditor::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    setupToolbar();

    QHBoxLayout* editorLayout = new QHBoxLayout();

    m_editor = new QPlainTextEdit(this);
    m_editor->setFont(QFont("Consolas", 10));
    m_editor->setTabStopDistance(40);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    connect(m_editor, &QPlainTextEdit::textChanged, this, &ScriptEditor::onTextChanged);

    m_highlighter = new ScriptHighlighter(m_editor->document());

    editorLayout->addWidget(m_editor, 3);

    m_outputLog = new QTextEdit(this);
    m_outputLog->setReadOnly(true);
    m_outputLog->setMaximumHeight(150);
    m_outputLog->setFont(QFont("Consolas", 9));

    mainLayout->addLayout(editorLayout);
    mainLayout->addWidget(m_outputLog);
}

void ScriptEditor::setupToolbar()
{
    QHBoxLayout* toolbarLayout = new QHBoxLayout();

    QPushButton* newBtn = new QPushButton("New", this);
    connect(newBtn, &QPushButton::clicked, this, &ScriptEditor::newScript);
    toolbarLayout->addWidget(newBtn);

    QPushButton* openBtn = new QPushButton("Open", this);
    connect(openBtn, &QPushButton::clicked, this, [this]() { openScript(""); });
    toolbarLayout->addWidget(openBtn);

    QPushButton* saveBtn = new QPushButton("Save", this);
    connect(saveBtn, &QPushButton::clicked, this, &ScriptEditor::saveScript);
    toolbarLayout->addWidget(saveBtn);

    QPushButton* saveAsBtn = new QPushButton("Save As", this);
    connect(saveAsBtn, &QPushButton::clicked, this, &ScriptEditor::saveScriptAs);
    toolbarLayout->addWidget(saveAsBtn);

    toolbarLayout->addSpacing(20);

    QLabel* langLabel = new QLabel("Language:", this);
    toolbarLayout->addWidget(langLabel);

    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem("Lua", ScriptHighlighter::Lua);
    m_languageCombo->addItem("AngelScript", ScriptHighlighter::AngelScript);
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScriptEditor::onLanguageChanged);
    toolbarLayout->addWidget(m_languageCombo);

    toolbarLayout->addSpacing(20);

    QPushButton* runBtn = new QPushButton("Run Script", this);
    connect(runBtn, &QPushButton::clicked, this, &ScriptEditor::runScript);
    toolbarLayout->addWidget(runBtn);

    toolbarLayout->addStretch();

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!mainLayout) {
        mainLayout = new QVBoxLayout(this);
    }
    mainLayout->insertLayout(0, toolbarLayout);
}

void ScriptEditor::newScript()
{
    ensureScriptsFolderExists();
    
    QString extension = (m_currentLanguage == ScriptHighlighter::Lua) ? ".lua" : ".as";
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString filename = "script_" + timestamp + extension;
    
    m_currentFile = getDefaultScriptPath() + "/" + filename;
    m_editor->clear();
    m_isModified = false;

    if (m_currentLanguage == ScriptHighlighter::Lua) {
        m_editor->setPlainText(getLuaTemplate());
    } else {
        m_editor->setPlainText(getAngelScriptTemplate());
    }
    
    saveScript();
    m_outputLog->append("Created new script: " + filename);
}

void ScriptEditor::openScript(const QString& filepath)
{
    QString path = filepath;
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(this, "Open Script",
            "", "Script Files (*.lua *.as);;Lua Scripts (*.lua);;AngelScript (*.as);;All Files (*)");
    }

    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open file: " + path);
        return;
    }

    QTextStream in(&file);
    m_editor->setPlainText(in.readAll());
    file.close();

    setCurrentFile(path);
    detectLanguageFromExtension(path);
}

void ScriptEditor::saveScript()
{
    if (m_currentFile.isEmpty()) {
        saveScriptAs();
        return;
    }

    QFile file(m_currentFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not save file: " + m_currentFile);
        return;
    }

    QTextStream out(&file);
    out << m_editor->toPlainText();
    file.close();

    m_isModified = false;
    m_outputLog->append("Saved: " + m_currentFile);
}

void ScriptEditor::saveScriptAs()
{
    ensureScriptsFolderExists();
    
    QString filter;
    QString defaultPath = getDefaultScriptPath();
    
    if (m_currentLanguage == ScriptHighlighter::Lua) {
        filter = "Lua Scripts (*.lua);;All Files (*)";
        defaultPath += "/script.lua";
    } else {
        filter = "AngelScript (*.as);;All Files (*)";
        defaultPath += "/script.as";
    }

    QString path = QFileDialog::getSaveFileName(this, "Save Script As", defaultPath, filter);
    if (path.isEmpty()) return;

    m_currentFile = path;
    saveScript();
}

void ScriptEditor::runScript()
{
    m_outputLog->clear();
    m_outputLog->append("=== Running Script ===");
    
    if (!m_scriptEngine) {
        m_outputLog->append("[ERROR] Script engine not initialized!");
        return;
    }
    
    QString code = m_editor->toPlainText();
    if (code.isEmpty()) {
        m_outputLog->append("[WARNING] No code to execute!");
        return;
    }
    
    bool success = false;
    
    if (m_currentLanguage == ScriptHighlighter::Lua) {
        m_outputLog->append("[Lua] Executing...");
        success = m_scriptEngine->executeLuaString(code.toStdString());
    } else {
        m_outputLog->append("[AngelScript] Executing...");
        success = m_scriptEngine->executeAngelScriptString(code.toStdString());
    }
    
    if (success) {
        m_outputLog->append("[SUCCESS] Script executed successfully!");
        emit scriptExecuted(true, "Script executed successfully");
    } else {
        m_outputLog->append("[ERROR] Script execution failed!");
        emit scriptExecuted(false, "Script execution failed");
    }
}

void ScriptEditor::onTextChanged()
{
    m_isModified = true;
}

void ScriptEditor::onLanguageChanged(int index)
{
    m_currentLanguage = static_cast<ScriptHighlighter::Language>(
        m_languageCombo->itemData(index).toInt());
    m_highlighter->setLanguage(m_currentLanguage);
}

void ScriptEditor::setCurrentFile(const QString& filepath)
{
    m_currentFile = filepath;
    m_isModified = false;
}

void ScriptEditor::detectLanguageFromExtension(const QString& filepath)
{
    if (filepath.endsWith(".lua", Qt::CaseInsensitive)) {
        m_languageCombo->setCurrentIndex(0);
        m_currentLanguage = ScriptHighlighter::Lua;
        m_highlighter->setLanguage(ScriptHighlighter::Lua);
    } else if (filepath.endsWith(".as", Qt::CaseInsensitive)) {
        m_languageCombo->setCurrentIndex(1);
        m_currentLanguage = ScriptHighlighter::AngelScript;
        m_highlighter->setLanguage(ScriptHighlighter::AngelScript);
    }
}

QString ScriptEditor::getLuaTemplate()
{
    return R"(-- Lua Script Template
-- DabozzEngine

function Start()
    print("Script started!")
end

function Update(deltaTime)
    -- Update logic here
end
)";
}

QString ScriptEditor::getAngelScriptTemplate()
{
    return R"(// AngelScript Template
// DabozzEngine

void Start()
{
    print("Script started!");
}

void Update(float deltaTime)
{
    // Update logic here
}
)";
}

void ScriptEditor::setProjectPath(const QString& projectPath)
{
    m_projectPath = projectPath;
}

QString ScriptEditor::getDefaultScriptPath()
{
    if (m_projectPath.isEmpty()) {
        return QDir::currentPath() + "/Scripts";
    }
    return m_projectPath + "/Scripts";
}

void ScriptEditor::ensureScriptsFolderExists()
{
    QString scriptsPath = getDefaultScriptPath();
    QDir dir;
    if (!dir.exists(scriptsPath)) {
        dir.mkpath(scriptsPath);
    }
}

void ScriptEditor::autoSave()
{
    if (m_isModified && !m_currentFile.isEmpty()) {
        saveScript();
        m_outputLog->append("[Auto-saved]");
    }
}
