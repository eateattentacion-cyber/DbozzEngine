#include "editor/scripteditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QSplitter>
#include <QLabel>
#include <QRegularExpression>

// C# Syntax Highlighter
CSharpHighlighter::CSharpHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // Keywords
    keywordFormat.setForeground(Qt::blue);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns = {
        "\\babstract\\b", "\\bas\\b", "\\bbase\\b", "\\bbool\\b", "\\bbreak\\b",
        "\\bbyte\\b", "\\bcase\\b", "\\bcatch\\b", "\\bchar\\b", "\\bchecked\\b",
        "\\bclass\\b", "\\bconst\\b", "\\bcontinue\\b", "\\bdecimal\\b", "\\bdefault\\b",
        "\\bdelegate\\b", "\\bdo\\b", "\\bdouble\\b", "\\belse\\b", "\\benum\\b",
        "\\bevent\\b", "\\bexplicit\\b", "\\bextern\\b", "\\bfalse\\b", "\\bfinally\\b",
        "\\bfixed\\b", "\\bfloat\\b", "\\bfor\\b", "\\bforeach\\b", "\\bgoto\\b",
        "\\bif\\b", "\\bimplicit\\b", "\\bin\\b", "\\bint\\b", "\\binterface\\b",
        "\\binternal\\b", "\\bis\\b", "\\block\\b", "\\blong\\b", "\\bnamespace\\b",
        "\\bnew\\b", "\\bnull\\b", "\\bobject\\b", "\\boperator\\b", "\\bout\\b",
        "\\boverride\\b", "\\bparams\\b", "\\bprivate\\b", "\\bprotected\\b", "\\bpublic\\b",
        "\\breadonly\\b", "\\bref\\b", "\\breturn\\b", "\\bsbyte\\b", "\\bsealed\\b",
        "\\bshort\\b", "\\bsizeof\\b", "\\bstackalloc\\b", "\\bstatic\\b", "\\bstring\\b",
        "\\bstruct\\b", "\\bswitch\\b", "\\bthis\\b", "\\bthrow\\b", "\\btrue\\b",
        "\\btry\\b", "\\btypeof\\b", "\\buint\\b", "\\bulong\\b", "\\bunchecked\\b",
        "\\bunsafe\\b", "\\bushort\\b", "\\busing\\b", "\\bvirtual\\b", "\\bvoid\\b",
        "\\bvolatile\\b", "\\bwhile\\b", "\\bvar\\b"
    };
    for (const QString& pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Class names
    classFormat.setForeground(QColor(43, 145, 175));
    classFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\b[A-Z][a-zA-Z0-9_]*\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);

    // Single line comments
    commentFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    // Strings
    stringFormat.setForeground(QColor(163, 21, 21));
    rule.pattern = QRegularExpression("\".*\"");
    rule.format = stringFormat;
    highlightingRules.append(rule);

    // Functions
    functionFormat.setForeground(QColor(255, 127, 0));
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
}

void CSharpHighlighter::highlightBlock(const QString& text)
{
    for (const HighlightingRule& rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

// Script Editor
ScriptEditor::ScriptEditor(QWidget* parent)
    : QWidget(parent)
    , m_editor(new QPlainTextEdit(this))
    , m_highlighter(new CSharpHighlighter(m_editor->document()))
    , m_outputLog(new QTextEdit(this))
    , m_compileProcess(new QProcess(this))
    , m_isModified(false)
{
    setupUI();
    setupToolbar();

    connect(m_editor, &QPlainTextEdit::textChanged, this, &ScriptEditor::onTextChanged);
    connect(m_compileProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ScriptEditor::onCompileFinished);
}

ScriptEditor::~ScriptEditor()
{
}

void ScriptEditor::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Splitter for editor and output
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);

    // Editor
    QFont font("Consolas", 10);
    m_editor->setFont(font);
    m_editor->setTabStopDistance(40);
    splitter->addWidget(m_editor);

    // Output log
    m_outputLog->setReadOnly(true);
    m_outputLog->setMaximumHeight(150);
    m_outputLog->setFont(font);
    splitter->addWidget(m_outputLog);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
}

void ScriptEditor::setupToolbar()
{
    QToolBar* toolbar = new QToolBar(this);
    
    QPushButton* newBtn = new QPushButton("New", this);
    QPushButton* openBtn = new QPushButton("Open", this);
    QPushButton* saveBtn = new QPushButton("Save", this);
    QPushButton* saveAsBtn = new QPushButton("Save As", this);
    QPushButton* compileBtn = new QPushButton("Compile", this);

    connect(newBtn, &QPushButton::clicked, this, &ScriptEditor::newScript);
    connect(openBtn, &QPushButton::clicked, this, [this]() { 
        QString file = QFileDialog::getOpenFileName(this, "Open Script", "Scripts", "C# Files (*.cs)");
        if (!file.isEmpty()) openScript(file);
    });
    connect(saveBtn, &QPushButton::clicked, this, &ScriptEditor::saveScript);
    connect(saveAsBtn, &QPushButton::clicked, this, &ScriptEditor::saveScriptAs);
    connect(compileBtn, &QPushButton::clicked, this, [this]() { compileScript(); });

    toolbar->addWidget(newBtn);
    toolbar->addWidget(openBtn);
    toolbar->addWidget(saveBtn);
    toolbar->addWidget(saveAsBtn);
    toolbar->addSeparator();
    toolbar->addWidget(compileBtn);

    layout()->setMenuBar(toolbar);
}

void ScriptEditor::newScript()
{
    if (m_isModified) {
        auto reply = QMessageBox::question(this, "Unsaved Changes", 
            "Do you want to save changes?", 
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        
        if (reply == QMessageBox::Yes) {
            saveScript();
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    m_editor->setPlainText(getScriptTemplate());
    m_currentFile.clear();
    m_isModified = false;
}

void ScriptEditor::openScript(const QString& filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open file: " + filepath);
        return;
    }

    QTextStream in(&file);
    m_editor->setPlainText(in.readAll());
    file.close();

    setCurrentFile(filepath);
    m_isModified = false;
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
    QString filepath = QFileDialog::getSaveFileName(this, "Save Script As", "Scripts", "C# Files (*.cs)");
    if (filepath.isEmpty()) {
        return;
    }

    setCurrentFile(filepath);
    saveScript();
}

bool ScriptEditor::compileScript()
{
    if (m_currentFile.isEmpty()) {
        m_outputLog->append("ERROR: Save the script before compiling.");
        return false;
    }

    // Auto-save before compile
    saveScript();

    m_outputLog->clear();
    m_outputLog->append("Compiling " + m_currentFile + "...");

    QFileInfo fileInfo(m_currentFile);
    QString outputDll = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".dll";
    QString dabozzDll = QDir::currentPath() + "/Scripts/DabozzEngine.dll";

    // Build csc command
    QStringList arguments;
    arguments << "/target:library";
    arguments << "/reference:" + dabozzDll;
    arguments << "/out:" + outputDll;
    arguments << m_currentFile;

    m_compileProcess->start("csc", arguments);
    m_compileProcess->waitForFinished();

    return true;
}

void ScriptEditor::onCompileFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString output = m_compileProcess->readAllStandardOutput();
    QString errors = m_compileProcess->readAllStandardError();

    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        m_outputLog->append("✓ Compilation successful!");
        if (!output.isEmpty()) {
            m_outputLog->append(output);
        }
        emit compilationFinished(true, output);
    } else {
        m_outputLog->append("✗ Compilation failed!");
        m_outputLog->append(errors);
        m_outputLog->append(output);
        emit compilationFinished(false, errors + "\n" + output);
    }
}

void ScriptEditor::onTextChanged()
{
    m_isModified = true;
}

void ScriptEditor::setCurrentFile(const QString& filepath)
{
    m_currentFile = filepath;
    setWindowTitle(QFileInfo(filepath).fileName() + " - Script Editor");
}

QString ScriptEditor::getScriptTemplate()
{
    return R"(using System;
using DabozzEngine;

public class MyScript : ScriptBehaviour
{
    public override void OnStart()
    {
        Debug.Log("Script started!");
    }

    public override void OnUpdate(float deltaTime)
    {
        // Update logic here
    }

    public override void OnDestroy()
    {
        Debug.Log("Script destroyed!");
    }
}
)";
}
