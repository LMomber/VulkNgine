#include "inputHandler.h"

#include "renderComponents.h"

#include "transform.h"

#include "engine.h"
#include "input.h"

using namespace Core;

void InputHandler::Update(float deltaTime)
{
    static float constant = 30.f;
    float speed = constant * deltaTime;
    float rotationSpeed = speed * 2.5f;

    entt::entity cameraEntity = Core::engine.GetRegistry().view<Camera>().front();

    Transform& cameraTransform = Core::engine.GetRegistry().get<Transform>(cameraEntity);

    const Input& input = Core::engine.GetInput();

    // Translation input
    {
        if (input.GetKeyboardKey(Input::KeyboardKey::A))
        {
            glm::vec3 cameraPosition = cameraTransform.GetTranslation();

            const glm::quat cameraRotation = cameraTransform.GetRotation();

            const glm::vec3 localForward = glm::vec3(0, 0, 1);
            const glm::vec3 forward = cameraRotation * localForward;

            const glm::vec3 right = normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
            glm::vec3 change = -right * speed;
            cameraPosition += change;

            cameraTransform.SetTranslation(cameraPosition);
        }
        if (input.GetKeyboardKey(Input::KeyboardKey::D))
        {
            glm::vec3 cameraPosition = cameraTransform.GetTranslation();
            const glm::quat cameraRotation = cameraTransform.GetRotation();

            const glm::vec3 localForward = glm::vec3(0, 0, 1);
            const glm::vec3 forward = cameraRotation * localForward;

            const glm::vec3 right = normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
            glm::vec3 change = right * speed;
            cameraPosition += change;

            cameraTransform.SetTranslation(cameraPosition);
        }
        if (input.GetKeyboardKey(Input::KeyboardKey::W))
        {
            glm::vec3 cameraPosition = cameraTransform.GetTranslation();
            const glm::quat cameraRotation = cameraTransform.GetRotation();

            const glm::vec3 localForward = glm::vec3(0, 0, 1);
            const glm::vec3 forward = cameraRotation * localForward;

            glm::vec3 change = forward * speed;
            cameraPosition += change;

            cameraTransform.SetTranslation(cameraPosition);
        }
        if (input.GetKeyboardKey(Input::KeyboardKey::S))
        {
            glm::vec3 cameraPosition = cameraTransform.GetTranslation();
            const glm::quat cameraRotation = cameraTransform.GetRotation();

            const glm::vec3 localForward = glm::vec3(0, 0, 1);
            const glm::vec3 forward = cameraRotation * localForward;

            glm::vec3 change = -forward * speed;
            cameraPosition += change;

            cameraTransform.SetTranslation(cameraPosition);
        }
        if (input.GetKeyboardKey(Input::KeyboardKey::Space))
        {
            glm::vec3 translation = cameraTransform.GetTranslation();
            glm::vec3 change = glm::vec3(0.0f, 1.0f, 0.0f) * speed;
            translation += change;

            cameraTransform.SetTranslation(translation);
        }
        if (input.GetKeyboardKey(Input::KeyboardKey::LeftShift))
        {
            glm::vec3 translation = cameraTransform.GetTranslation();
            glm::vec3 change = glm::vec3(0.0f, -1.0f, 0.0f) * speed;
            translation += change;

            cameraTransform.SetTranslation(translation);
        }
    }

    // Rotation input
    {
        if (input.GetKeyboardKey(Input::KeyboardKey::ArrowLeft))
        {
            glm::quat rotation = cameraTransform.GetRotation();
            glm::quat change = glm::angleAxis(glm::radians(rotationSpeed), glm::vec3(0, 1, 0));
            rotation = change * rotation;

            cameraTransform.SetRotation(rotation);
        }
        if (input.GetKeyboardKey(Input::KeyboardKey::ArrowRight))
        {
            glm::quat rotation = cameraTransform.GetRotation();
            glm::quat change = glm::angleAxis(glm::radians(-rotationSpeed), glm::vec3(0, 1, 0));
            rotation = change * rotation;

            cameraTransform.SetRotation(rotation);
        }
        if (input.GetKeyboardKey(Input::KeyboardKey::ArrowUp))
        {
            glm::quat rotation = cameraTransform.GetRotation();
            glm::quat change = glm::angleAxis(glm::radians(-rotationSpeed), glm::vec3(1, 0, 0));
            rotation *= change;

            cameraTransform.SetRotation(rotation);
        }
        if (input.GetKeyboardKey(Input::KeyboardKey::ArrowDown))
        {
            glm::quat rotation = cameraTransform.GetRotation();
            glm::quat change = glm::angleAxis(glm::radians(rotationSpeed), glm::vec3(1, 0, 0));
            rotation *= change;

            cameraTransform.SetRotation(rotation);
        }
    }

    // Mouse input
    {
        static float lastMouse = 0;
        if (input.GetMouseWheel() != lastMouse)
        {
            float value = input.GetMouseWheel() - lastMouse;
            lastMouse = input.GetMouseWheel();
            constant += value;
        }
    }
}
