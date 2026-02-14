#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QComboBox>

namespace DabozzEngine {
namespace Scripting {
    class ScriptEngine;
}
}

class ScriptHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    enum Language {
        Lua,
        AngelScript
    };

    explicit ScriptHighlighter(QTextDocument* parent = nullptr);
    void setLanguage(Language lang);

protected:
    void highlightBlock(const QString& text) override;

private:
    void setupLuaRules();
    void setupAngelScriptRules();

    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat numberFormat;

    Language m_language;
};

class ScriptEditor : public QWidget {
    Q_OBJECT
public:
    explicit ScriptEditor(QWidget* parent = nullptr);
    ~ScriptEditor();

    void newScript();
    void openScript(const QString& filepath);
    void saveScript();
    void saveScriptAs();
    void runScript();
    void setProjectPath(const QString& projectPath);

signals:
    void scriptExecuted(bool success, const QString& output);

private slots:
    void onTextChanged();
    void onLanguageChanged(int index);
    void autoSave();

private:
    void setupUI();
    void setupToolbar();
    void setCurrentFile(const QString& filepath);
    void detectLanguageFromExtension(const QString& filepath);
    QString getLuaTemplate();
    QString getAngelScriptTemplate();
    QString getDefaultScriptPath();
    void ensureScriptsFolderExists();

    QPlainTextEdit* m_editor;
    ScriptHighlighter* m_highlighter;
    QTextEdit* m_outputLog;
    QComboBox* m_languageCombo;
    QTimer* m_autoSaveTimer;
    DabozzEngine::Scripting::ScriptEngine* m_scriptEngine;
    
    QString m_currentFile;
    QString m_projectPath;
    bool m_isModified;
    ScriptHighlighter::Language m_currentLanguage;
};
