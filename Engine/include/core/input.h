#pragma once

#include <glm/vec2.hpp>
#include <memory>

namespace Core
{
	class Engine;
	class Input
	{
	public:
		/// <summary>
		/// An enum listing all supported keyboard keys.
		/// This uses the same numbering as in GLFW input, so a GLFW-based implementation can use it directly without any further
		/// mapping.
		/// </summary>
		enum class KeyboardKey
		{
			Space = 32,
			Apostrophe = 39,
			Comma = 44,
			Minus = 45,
			Period = 46,
			Slash = 47,
			Digit0 = 48,
			Digit1 = 49,
			Digit2 = 50,
			Digit3 = 51,
			Digit4 = 52,
			Digit5 = 53,
			Digit6 = 54,
			Digit7 = 55,
			Digit8 = 56,
			Digit9 = 57,
			Semicolon = 59,
			Equal = 61,
			A = 65,
			B = 66,
			C = 67,
			D = 68,
			E = 69,
			F = 70,
			G = 71,
			H = 72,
			I = 73,
			J = 74,
			K = 75,
			L = 76,
			M = 77,
			N = 78,
			O = 79,
			P = 80,
			Q = 81,
			R = 82,
			S = 83,
			T = 84,
			U = 85,
			V = 86,
			W = 87,
			X = 88,
			Y = 89,
			Z = 90,
			LeftBracket = 91,
			Backslash = 92,
			RightBracket = 93,
			GraveAccent = 96,
			World1 = 161,
			World2 = 162,
			Escape = 256,
			Enter = 257,
			Tab = 258,
			Backspace = 259,
			Insert = 260,
			Delete = 261,
			ArrowRight = 262,
			ArrowLeft = 263,
			ArrowDown = 264,
			ArrowUp = 265,
			PageUp = 266,
			PageDown = 267,
			Home = 268,
			End = 269,
			CapsLock = 280,
			ScrollLock = 281,
			NumLock = 282,
			PrintScreen = 283,
			Pause = 284,
			F1 = 290,
			F2 = 291,
			F3 = 292,
			F4 = 293,
			F5 = 294,
			F6 = 295,
			F7 = 296,
			F8 = 297,
			F9 = 298,
			F10 = 299,
			F11 = 300,
			F12 = 301,
			F13 = 302,
			F14 = 303,
			F15 = 304,
			F16 = 305,
			F17 = 306,
			F18 = 307,
			F19 = 308,
			F20 = 309,
			F21 = 310,
			F22 = 311,
			F23 = 312,
			F24 = 313,
			F25 = 314,
			Numpad0 = 320,
			Numpad1 = 321,
			Numpad2 = 322,
			Numpad3 = 323,
			Numpad4 = 324,
			Numpad5 = 325,
			Numpad6 = 326,
			Numpad7 = 327,
			Numpad8 = 328,
			Numpad9 = 329,
			NumpadDecimal = 330,
			NumpadDivide = 331,
			NumpadMultiply = 332,
			NumpadSubtract = 333,
			NumpadAdd = 334,
			NumpadEnter = 335,
			NumpadEqual = 336,
			LeftShift = 340,
			LeftControl = 341,
			LeftAlt = 342,
			LeftSuper = 343,
			RightShift = 344,
			RightControl = 345,
			RightAlt = 346,
			RightSuper = 347,
			Menu = 348
		};

		/// <summary>
		/// An enum listing all supported mouse buttons.
		/// This uses the same numbering as in GLFW input, so a GLFW-based implementation can use it directly without any further
		/// mapping.
		/// </summary>
		enum class MouseButton
		{
			Left = 0,
			Right = 1,
			Middle = 2
		};

		/// Checks and returns whether a mouse is currently available for input.
		bool IsMouseAvailable() const;

		/// Checks and returns whether a given mouse button is being held down in the current frame.
		bool GetMouseButton(MouseButton button) const;

		/// Checks and returns whether a given mouse button is being pressed in the current frame without having been pressed in the
		/// previous frame.
		bool GetMouseButtonOnce(MouseButton button) const;

		/// Gets the screen position of the mouse in pixel coordinates, relative to the top-left corner of the screen.
		glm::vec2 GetMousePosition() const;

		/// Gets the screen position of the mouse in viewport coordinates, 
		/// where (-1,-1) is the bottom left and (1,1) is the top right of the viewport that currently displays the game.
		glm::vec2 GetMousePositionInViewport() const;

		/// Gets the mouse wheel, relative to the initial value when starting the game.
		float GetMouseWheel() const;

		/// Checks and returns whether a keyboard is currently available for input.
		bool IsKeyboardAvailable() const;

		/// Checks and returns whether a given keyboard key is being held down in the current frame.
		bool GetKeyboardKey(KeyboardKey button) const;

		/// Checks and returns whether a given keyboard key is being pressed in the current frame without having been pressed in the
		/// previous frame.
		bool GetKeyboardKeyOnce(KeyboardKey button) const;

	private:
		friend class Engine;
		friend void InputDelFunc(Input* p);

		Input();
		~Input();
		void Update();
	};

	void InputDelFunc(Input* p);
}
