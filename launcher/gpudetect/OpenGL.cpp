// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Octol1ttle <l1ttleofficial@outlook.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "OpenGL.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include "Print.h"

void OpenGL::print()
{
    QOpenGLContext ctx;
    if (!ctx.create()) {
        Print::err("OpenGL context creation failed");
        return;
    }

    QOffscreenSurface surface;
    surface.create();
    ctx.makeCurrent(&surface);

    auto* f = ctx.functions();
    f->initializeOpenGLFunctions();

    auto toQString = [](const GLubyte* str) { return QString(reinterpret_cast<const char*>(str)); };  // NOLINT(*-pro-type-reinterpret-cast)
    Print::info("OpenGL driver vendor: " + toQString(f->glGetString(GL_VENDOR)));
    Print::info("OpenGL renderer: " + toQString(f->glGetString(GL_RENDERER)));
    Print::info("OpenGL driver version: " + toQString(f->glGetString(GL_VERSION)));
}
