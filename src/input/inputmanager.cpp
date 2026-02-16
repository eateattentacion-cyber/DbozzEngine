#include "input/inputmanager.h"

namespace DabozzEngine {
namespace Input {

InputManager& InputManager::getInstance()
{
    static InputManager instance;
    return instance;
}

void InputManager::update()
{
    m_keysPressed.clear();
    m_keysReleased.clear();
    m_mouseButtonsPressed.clear();
    m_mouseButtonsReleased.clear();
    
    m_mouseDelta = m_mousePosition - m_lastMousePosition;
    m_lastMousePosition = m_mousePosition;
    m_mouseScrollDelta = 0;
}

void InputManager::reset()
{
    m_keysDown.clear();
    m_keysPressed.clear();
    m_keysReleased.clear();
    m_mouseButtonsDown.clear();
    m_mouseButtonsPressed.clear();
    m_mouseButtonsReleased.clear();
    m_mousePosition = QPoint(0, 0);
    m_lastMousePosition = QPoint(0, 0);
    m_mouseDelta = QPoint(0, 0);
    m_mouseScrollDelta = 0;
}

void InputManager::keyPressed(int key)
{
    if (!m_keysDown.contains(key)) {
        m_keysPressed.insert(key);
        m_keysDown.insert(key);
    }
}

void InputManager::keyReleased(int key)
{
    m_keysReleased.insert(key);
    m_keysDown.remove(key);
}

void InputManager::mousePressed(Qt::MouseButton button)
{
    if (!m_mouseButtonsDown.contains(button)) {
        m_mouseButtonsPressed.insert(button);
        m_mouseButtonsDown.insert(button);
    }
}

void InputManager::mouseReleased(Qt::MouseButton button)
{
    m_mouseButtonsReleased.insert(button);
    m_mouseButtonsDown.remove(button);
}

void InputManager::mouseMoved(const QPoint& pos)
{
    m_mousePosition = pos;
}

void InputManager::mouseScrolled(int delta)
{
    m_mouseScrollDelta = delta;
}

bool InputManager::isKeyDown(int key) const
{
    return m_keysDown.contains(key);
}

bool InputManager::isKeyPressed(int key) const
{
    return m_keysPressed.contains(key);
}

bool InputManager::isKeyReleased(int key) const
{
    return m_keysReleased.contains(key);
}

bool InputManager::isMouseButtonDown(Qt::MouseButton button) const
{
    return m_mouseButtonsDown.contains(button);
}

bool InputManager::isMouseButtonPressed(Qt::MouseButton button) const
{
    return m_mouseButtonsPressed.contains(button);
}

bool InputManager::isMouseButtonReleased(Qt::MouseButton button) const
{
    return m_mouseButtonsReleased.contains(button);
}

}
}
