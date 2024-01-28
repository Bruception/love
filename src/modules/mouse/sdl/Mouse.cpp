/**
 * Copyright (c) 2006-2023 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

// LOVE
#include "Mouse.h"
#include "window/sdl/Window.h"

// SDL
#include <SDL_mouse.h>

namespace love
{
namespace mouse
{
namespace sdl
{

// SDL reports mouse coordinates in the window coordinate system in OS X, but
// we want them in pixel coordinates (may be different with high-DPI enabled.)
static void windowToDPICoords(double *x, double *y)
{
	auto window = Module::getInstance<window::Window>(Module::M_WINDOW);
	if (window)
		window->windowToDPICoords(x, y);
}

// And vice versa for setting mouse coordinates.
static void DPIToWindowCoords(double *x, double *y)
{
	auto window = Module::getInstance<window::Window>(Module::M_WINDOW);
	if (window)
		window->DPIToWindowCoords(x, y);
}

static void clampToWindow(double *x, double *y)
{
	auto window = Module::getInstance<window::Window>(Module::M_WINDOW);
	if (window)
		window->clampPositionInWindow(x, y);
}

const char *Mouse::getName() const
{
	return "love.mouse.sdl";
}

Mouse::Mouse()
	: curCursor(nullptr)
{
	// SDL may need the video subsystem in order to clean up the cursor when
	// quitting. Subsystems are reference-counted.
	SDL_InitSubSystem(SDL_INIT_VIDEO);
}

Mouse::~Mouse()
{
	if (curCursor.get())
		setCursor();

	for (auto &c : systemCursors)
		c.second->release();

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

love::mouse::Cursor *Mouse::newCursor(love::image::ImageData *data, int hotx, int hoty)
{
	return new Cursor(data, hotx, hoty);
}

love::mouse::Cursor *Mouse::getSystemCursor(Cursor::SystemCursor cursortype)
{
	Cursor *cursor = nullptr;
	auto it = systemCursors.find(cursortype);

	if (it != systemCursors.end())
		cursor = it->second;
	else
	{
		cursor = new Cursor(cursortype);
		systemCursors[cursortype] = cursor;
	}

	return cursor;
}

void Mouse::setCursor(love::mouse::Cursor *cursor)
{
	curCursor.set(cursor);
	SDL_SetCursor((SDL_Cursor *) cursor->getHandle());
}

void Mouse::setCursor()
{
	curCursor.set(nullptr);
	SDL_SetCursor(SDL_GetDefaultCursor());
}

love::mouse::Cursor *Mouse::getCursor() const
{
	return curCursor.get();
}


bool Mouse::isCursorSupported() const
{
	return SDL_GetDefaultCursor() != nullptr;
}

void Mouse::getPosition(double &x, double &y) const
{
	int mx, my;
	SDL_GetMouseState(&mx, &my);

	x = (double) mx;
	y = (double) my;

	// SDL reports mouse coordinates outside the window bounds when click-and-
	// dragging. For compatibility we clamp instead since user code may not be
	// able to handle out-of-bounds coordinates. SDL has a hint to turn off
	// auto capture, but it doesn't report the mouse's position at the edge of
	// the window if the mouse moves fast enough when it's off.
	clampToWindow(&x, &y);

	windowToDPICoords(&x, &y);
}

void Mouse::setPosition(double x, double y)
{
	auto window = Module::getInstance<window::Window>(Module::M_WINDOW);

	SDL_Window *handle = nullptr;
	if (window)
		handle = (SDL_Window *) window->getHandle();

	DPIToWindowCoords(&x, &y);
	SDL_WarpMouseInWindow(handle, (int) x, (int) y);

	// SDL_WarpMouse doesn't directly update SDL's internal mouse state in Linux
	// and Windows, so we call SDL_PumpEvents now to make sure the next
	// getPosition call always returns the updated state.
	SDL_PumpEvents();
}

void Mouse::getGlobalPosition(double &x, double &y, int &displayindex) const
{
	int globalx, globaly;
	SDL_GetGlobalMouseState(&globalx, &globaly);

	int mx = globalx;
	int my = globaly;

	int displaycount = SDL_GetNumVideoDisplays();

	for (displayindex = 0; displayindex < displaycount; displayindex++)
	{
		SDL_Rect rect = {};
		SDL_GetDisplayBounds(displayindex, &rect);

		mx -= rect.x;
		my -= rect.y;

		SDL_Point p = { globalx, globaly };
		if (SDL_PointInRect(&p, &rect))
			break;
	}

	if (displayindex >= displaycount)
		displayindex = 0;

	x = (double)mx;
	y = (double)my;
}

void Mouse::setVisible(bool visible)
{
	SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

bool Mouse::isDown(const std::vector<int> &buttons) const
{
	Uint32 buttonstate = SDL_GetMouseState(nullptr, nullptr);

	for (int button : buttons)
	{
		if (button <= 0)
			continue;

		// We use button index 2 to represent the right mouse button, but SDL
		// uses 2 to represent the middle mouse button.
		switch (button)
		{
		case 2:
			button = SDL_BUTTON_RIGHT;
			break;
		case 3:
			button = SDL_BUTTON_MIDDLE;
			break;
		}

		if (buttonstate & SDL_BUTTON(button))
			return true;
	}

	return false;
}

bool Mouse::isVisible() const
{
	return SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE;
}

void Mouse::setGrabbed(bool grab)
{
	auto window = Module::getInstance<window::Window>(Module::M_WINDOW);
	if (window)
		window->setMouseGrab(grab);
}

bool Mouse::isGrabbed() const
{
	auto window = Module::getInstance<window::Window>(Module::M_WINDOW);
	if (window)
		return window->isMouseGrabbed();
	else
		return false;
}

bool Mouse::setRelativeMode(bool relative)
{
	return SDL_SetRelativeMouseMode(relative ? SDL_TRUE : SDL_FALSE) == 0;
}

bool Mouse::getRelativeMode() const
{
	return SDL_GetRelativeMouseMode() != SDL_FALSE;
}

} // sdl
} // mouse
} // love
