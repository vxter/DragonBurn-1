#pragma once
#include <windows.h>
#include <cstdint>
#include <queue>
#include <string>

namespace InputMgr
{
	// Game input command IDs that mimic in-game key presses
	enum class InputCommand : uint8_t
	{
		FORWARD = 0,		// W key
		BACKWARD = 1,		// S key
		STRAFE_LEFT = 2,	// A key
		STRAFE_RIGHT = 3,	// D key
		JUMP = 4,			// Space key
		CROUCH = 5,			// Ctrl key
		ROTATE_LEFT = 6,	// Turn left (yaw -10)
		ROTATE_RIGHT = 7	// Turn right (yaw +10)
	};

	// State tracking for each input
	struct InputState
	{
		bool forward = false;
		bool backward = false;
		bool strafe_left = false;
		bool strafe_right = false;
		bool jump = false;
		bool crouch = false;
		float rotation = 0.0f;  // Additional rotation in degrees

		// Reset all inputs to default state
		void Reset()
		{
			forward = false;
			backward = false;
			strafe_left = false;
			strafe_right = false;
			jump = false;
			crouch = false;
			rotation = 0.0f;
		}

		// Apply an input command
		void ApplyCommand(InputCommand cmd, bool pressed)
		{
			switch (cmd)
			{
			case InputCommand::FORWARD:
				forward = pressed;
				break;
			case InputCommand::BACKWARD:
				backward = pressed;
				break;
			case InputCommand::STRAFE_LEFT:
				strafe_left = pressed;
				break;
			case InputCommand::STRAFE_RIGHT:
				strafe_right = pressed;
				break;
			case InputCommand::JUMP:
				jump = pressed;
				break;
			case InputCommand::CROUCH:
				crouch = pressed;
				break;
			case InputCommand::ROTATE_LEFT:
				rotation -= pressed ? 10.0f : 0.0f;
				break;
			case InputCommand::ROTATE_RIGHT:
				rotation += pressed ? 10.0f : 0.0f;
				break;
			}
		}
	};

	// Global input state
	static InputState CurrentInput;

	// Simulate a game input without affecting host keyboard/mouse
	// Uses keyboard messages directly like bunny hop does
	inline void SimulateGameInput(InputCommand cmd, bool pressed)
	{
		CurrentInput.ApplyCommand(cmd, pressed);

		// Map input command to virtual key code
		int vk = 0;
		switch (cmd)
		{
		case InputCommand::FORWARD:     vk = 'W'; break;
		case InputCommand::BACKWARD:    vk = 'S'; break;
		case InputCommand::STRAFE_LEFT: vk = 'A'; break;
		case InputCommand::STRAFE_RIGHT: vk = 'D'; break;
		case InputCommand::JUMP:        vk = VK_SPACE; break;
		case InputCommand::CROUCH:      vk = VK_CONTROL; break;
		case InputCommand::ROTATE_LEFT:
		case InputCommand::ROTATE_RIGHT:
			// View angle adjustments - handled elsewhere
			return;
		}

		if (vk == 0)
			return;

		// Get the CS2 game window
		HWND hwndGame = FindWindowA(NULL, "Counter-Strike 2");
		if (hwndGame == NULL)
		{
			// Try alternative search
			hwndGame = FindWindowA("UnityWndClass", NULL);
		}
		
		if (hwndGame == NULL)
			return;

		// Send keyboard message directly to the game window
		// Using SendMessage like the existing bunny hop feature
		if (pressed)
		{
			SendMessage(hwndGame, WM_KEYDOWN, vk, 0);
		}
		else
		{
			SendMessage(hwndGame, WM_KEYUP, vk, 0);
		}
	}

	// Get the current movement vector based on active inputs
	inline void GetMovementVector(float& x, float& y)
	{
		x = 0.0f;
		y = 0.0f;

		if (CurrentInput.forward) y += 1.0f;
		if (CurrentInput.backward) y -= 1.0f;
		if (CurrentInput.strafe_right) x += 1.0f;
		if (CurrentInput.strafe_left) x -= 1.0f;

		// Normalize diagonal movement
		if (x != 0.0f && y != 0.0f)
		{
			x *= 0.707f;  // 1/sqrt(2)
			y *= 0.707f;
		}
	}

	// Check if any movement is active
	inline bool IsMoving()
	{
		return CurrentInput.forward || CurrentInput.backward || 
			   CurrentInput.strafe_left || CurrentInput.strafe_right;
	}

	// Check if jumping
	inline bool IsJumping()
	{
		return CurrentInput.jump;
	}

	// Check if crouching
	inline bool IsCrouching()
	{
		return CurrentInput.crouch;
	}

	// Check individual key states
	inline bool IsForwardPressed()
	{
		return CurrentInput.forward;
	}

	inline bool IsBackwardPressed()
	{
		return CurrentInput.backward;
	}

	inline bool IsLeftPressed()
	{
		return CurrentInput.strafe_left;
	}

	inline bool IsRightPressed()
	{
		return CurrentInput.strafe_right;
	}

	// Check if jump is active (from either UI buttons OR physical space key)
	inline bool ShouldJump()
	{
		// Include both physical key and UI input
		return CurrentInput.jump || ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0);
	}

	// Check if any movement is requested
	inline bool ShouldMoveForward()
	{
		return CurrentInput.forward || ((GetAsyncKeyState('W') & 0x8000) != 0);
	}

	inline bool ShouldMoveBackward()
	{
		return CurrentInput.backward || ((GetAsyncKeyState('S') & 0x8000) != 0);
	}

	inline bool ShouldStrafeLeft()
	{
		return CurrentInput.strafe_left || ((GetAsyncKeyState('A') & 0x8000) != 0);
	}

	inline bool ShouldStrafeRight()
	{
		return CurrentInput.strafe_right || ((GetAsyncKeyState('D') & 0x8000) != 0);
	}

	inline bool ShouldCrouch()
	{
		return CurrentInput.crouch || ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
	}

	// Get rotation delta
	inline float GetRotationDelta()
	{
		return CurrentInput.rotation;
	}

	// Reset rotation after applying
	inline void ClearRotation()
	{
		CurrentInput.rotation = 0.0f;
	}
}
