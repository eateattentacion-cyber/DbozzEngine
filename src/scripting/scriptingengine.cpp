#include "scripting/scriptingengine.h"
#include "debug/logger.h"
#include <mono/metadata/mono-config.h>

namespace DabozzEngine {
namespace Scripting {

ScriptingEngine::ScriptingEngine()
    : m_domain(nullptr)
    , m_assembly(nullptr)
    , m_image(nullptr)
{
}

ScriptingEngine::~ScriptingEngine()
{
    shutdown();
}

bool ScriptingEngine::initialize()
{
    DEBUG_LOG << "Initializing Mono scripting engine..." << std::endl;
    
    // Set Mono directories
    mono_set_dirs("C:/Program Files/Mono/lib", "C:/Program Files/Mono/etc");
    
    // Initialize Mono
    m_domain = mono_jit_init("DabozzEngine");
    if (!m_domain) {
        DEBUG_LOG << "ERROR: Failed to initialize Mono domain" << std::endl;
        return false;
    }
    
    DEBUG_LOG << "Mono scripting engine initialized successfully" << std::endl;
    return true;
}

void ScriptingEngine::shutdown()
{
    if (m_domain) {
        mono_jit_cleanup(m_domain);
        m_domain = nullptr;
    }
}

bool ScriptingEngine::loadAssembly(const std::string& assemblyPath)
{
    DEBUG_LOG << "Loading assembly: " << assemblyPath << std::endl;
    
    m_assembly = mono_domain_assembly_open(m_domain, assemblyPath.c_str());
    if (!m_assembly) {
        DEBUG_LOG << "ERROR: Failed to load assembly: " << assemblyPath << std::endl;
        return false;
    }
    
    m_image = mono_assembly_get_image(m_assembly);
    if (!m_image) {
        DEBUG_LOG << "ERROR: Failed to get assembly image" << std::endl;
        return false;
    }
    
    DEBUG_LOG << "Assembly loaded successfully" << std::endl;
    return true;
}

MonoObject* ScriptingEngine::createScriptInstance(const std::string& namespaceName, const std::string& className)
{
    std::string fullName = namespaceName + "." + className;
    MonoClass* klass = mono_class_from_name(m_image, namespaceName.c_str(), className.c_str());
    
    if (!klass) {
        DEBUG_LOG << "ERROR: Failed to find class: " << fullName << std::endl;
        return nullptr;
    }
    
    MonoObject* instance = mono_object_new(m_domain, klass);
    if (!instance) {
        DEBUG_LOG << "ERROR: Failed to create instance of: " << fullName << std::endl;
        return nullptr;
    }
    
    mono_runtime_object_init(instance);
    return instance;
}

}
}
