#pragma once

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <string>

namespace DabozzEngine {
namespace Scripting {

class ScriptingEngine {
public:
    ScriptingEngine();
    ~ScriptingEngine();
    
    bool initialize();
    void shutdown();
    
    bool loadAssembly(const std::string& assemblyPath);
    MonoObject* createScriptInstance(const std::string& namespaceName, const std::string& className);
    
    MonoDomain* getDomain() const { return m_domain; }
    MonoAssembly* getAssembly() const { return m_assembly; }
    
private:
    MonoDomain* m_domain;
    MonoAssembly* m_assembly;
    MonoImage* m_image;
};

}
}
