/* Copyright 2013-2015 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gui/Platform.h>
#include <QtX11Extras/QX11Info>
#include <xcb/xcb.h>

static QByteArray WM_CLASS = "MultiMC5\0MultiMC5";

template <typename... ArgTypes, typename... ArgTypes2>
static inline unsigned int XcbCallVoid(xcb_void_cookie_t (*func)(xcb_connection_t *, ArgTypes...), ArgTypes2... args...)
{
    return func(QX11Info::connection(), args...).sequence;
}

static void getAtoms(size_t n, xcb_atom_t *atoms, const char *const names[], bool create)
{
    xcb_connection_t *conn = QX11Info::connection();
    xcb_intern_atom_cookie_t *cookies = (xcb_intern_atom_cookie_t *)malloc(sizeof(xcb_intern_atom_cookie_t) * 2);
    for (size_t i = 0; i < n; ++i)
        cookies[i] = xcb_intern_atom(conn, create, strlen(names[i]), names[i]);
    memset(atoms, 0, sizeof(xcb_atom_t) * n);
    for (size_t i = 0; i < n; ++i)
    {
        xcb_intern_atom_reply_t *r = xcb_intern_atom_reply(conn, cookies[i], 0);
        if (r)
        {
            atoms[i] = r->atom;
            free(r);
        }
    }
    free(cookies);
}

static inline xcb_atom_t getAtom(const char *name, bool create=false)
{
    xcb_atom_t atom;
    getAtoms(1, &atom, &name, create);
    return atom;
}

void MultiMCPlatform::fixWM_CLASS(QWidget *widget)
{
    static const xcb_atom_t atom = getAtom("WM_CLASS");
    XcbCallVoid(xcb_change_property, XCB_PROP_MODE_REPLACE,
                widget->winId(), atom, XCB_ATOM_STRING, 8, WM_CLASS.count(),
                WM_CLASS.constData());
}
