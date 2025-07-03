#include "input.h"

#include "engine.h"
#include "vkDevice.h"

using namespace Core;

enum KeyAction
{
    Release = 0,
    Press = 1,
    None = 2
};

constexpr int nr_keys = 350;
bool keys_down[nr_keys];
bool prev_keys_down[nr_keys];
KeyAction keys_action[nr_keys];

constexpr int nr_mousebuttons = 8;
bool mousebuttons_down[nr_mousebuttons];
bool prev_mousebuttons_down[nr_mousebuttons];
KeyAction mousebuttons_action[nr_mousebuttons];

glm::vec2 mousepos;
float mousewheel = 0;

void cursor_position_callback(GLFWwindow*, double xpos, double ypos)
{
    mousepos.x = (float)xpos;
    mousepos.y = (float)ypos;
}

void scroll_callback(GLFWwindow*, double, double yoffset) { mousewheel += (float)yoffset; }

void key_callback(GLFWwindow*, int key, int, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_RELEASE) keys_action[key] = static_cast<KeyAction>(action);
}

void mousebutton_callback(GLFWwindow*, int button, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_RELEASE) mousebuttons_action[button] = static_cast<KeyAction>(action);
}

Input::Input()
{
    auto* window = Core::engine.GetDevice().GetWindow();

    // glfwSetJoystickCallback(joystick_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mousebutton_callback);
    glfwSetScrollCallback(window, scroll_callback);

    Update();
}

Input::~Input()
{
    auto* window = Core::engine.GetDevice().GetWindow();

    glfwSetCursorPosCallback(window, nullptr);
}

void Input::Update()
{
    glfwPollEvents();

    // update keyboard key states
    for (int i = 0; i < nr_keys; ++i)
    {
        prev_keys_down[i] = keys_down[i];

        if (keys_action[i] == KeyAction::Press)
            keys_down[i] = true;
        else if (keys_action[i] == KeyAction::Release)
            keys_down[i] = false;

        keys_action[i] = KeyAction::None;
    }

    // update mouse button states
    for (int i = 0; i < nr_mousebuttons; ++i)
    {
        prev_mousebuttons_down[i] = mousebuttons_down[i];

        if (mousebuttons_action[i] == KeyAction::Press)
            mousebuttons_down[i] = true;
        else if (mousebuttons_action[i] == KeyAction::Release)
            mousebuttons_down[i] = false;

        mousebuttons_action[i] = KeyAction::None;
    }
}

bool Input::IsMouseAvailable() const { return true; }

bool Input::GetMouseButton(MouseButton button) const
{
    int b = static_cast<int>(button);
    return mousebuttons_down[b];
}

bool Input::GetMouseButtonOnce(MouseButton button) const
{
    int b = static_cast<int>(button);
    return mousebuttons_down[b] && !prev_mousebuttons_down[b];
}

glm::vec2 Input::GetMousePosition() const { return mousepos; }

glm::vec2 Input::GetMousePositionInViewport() const
{
    glm::vec2 mousePosRelative(GetMousePosition());

    VkExtent2D extent = Core::engine.GetDevice().GetExtent();
    glm::vec2 viewportSize(extent.width, extent.height);

//#ifdef BEE_INSPECTOR
//    if (Engine.Inspector().GetVisible())
//    {
//        // translate/scale with respect to the game view in the inspector
//        const auto& viewport = Engine.Inspector().GetGameViewportBounds();
//        mousePosRelative -= viewport.GetMin();
//        viewportSize = viewport.ComputeSize();
//    }
//#endif

    return glm::vec2(2 * mousePosRelative.x / viewportSize.x - 1, -(2 * mousePosRelative.y / viewportSize.y - 1));
}

float Input::GetMouseWheel() const { return mousewheel; }

bool Input::IsKeyboardAvailable() const { return true; }

bool Input::GetKeyboardKey(KeyboardKey key) const
{
    int k = static_cast<int>(key);
    assert(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k];
}

bool Input::GetKeyboardKeyOnce(KeyboardKey key) const
{
    int k = static_cast<int>(key);
    assert(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k] && !prev_keys_down[k];
}

void Core::InputDelFunc(Input* p)
{
    delete p;
}
