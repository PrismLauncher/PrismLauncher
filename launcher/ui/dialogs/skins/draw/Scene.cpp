
// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
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

#include "ui/dialogs/skins/draw/Scene.h"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWindow>

namespace opengl {
Scene::Scene(const QImage& skin, bool slim, const QImage& cape) : QOpenGLFunctions(), m_slim(slim), m_capeVisible(!cape.isNull())
{
    initializeOpenGLFunctions();
    m_staticComponents = {
        // head
        new opengl::BoxGeometry(QVector3D(8, 8, 8), QVector3D(0, 4, 0), QPoint(0, 0), QVector3D(8, 8, 8)),
        // body
        new opengl::BoxGeometry(QVector3D(8, 12, 4), QVector3D(0, -6, 0), QPoint(16, 16), QVector3D(8, 12, 4)),
        // right leg
        new opengl::BoxGeometry(QVector3D(4, 12, 4), QVector3D(-1.9, -18, -0.1), QPoint(0, 16), QVector3D(4, 12, 4)),
        // left leg
        new opengl::BoxGeometry(QVector3D(4, 12, 4), QVector3D(1.9, -18, -0.1), QPoint(16, 48), QVector3D(4, 12, 4)),
    };

    m_staticComponentsOverlay = {
        // head
        new opengl::BoxGeometry(QVector3D(9, 9, 9), QVector3D(0, 4, 0), QPoint(32, 0), QVector3D(8, 8, 8)),
        // body
        new opengl::BoxGeometry(QVector3D(8.5, 12.5, 4.5), QVector3D(0, -6, 0), QPoint(16, 32), QVector3D(8, 12, 4)),
        // right leg
        new opengl::BoxGeometry(QVector3D(4.5, 12.5, 4.5), QVector3D(-1.9, -18, -0.1), QPoint(0, 32), QVector3D(4, 12, 4)),
        // left leg
        new opengl::BoxGeometry(QVector3D(4.5, 12.5, 4.5), QVector3D(1.9, -18, -0.1), QPoint(0, 48), QVector3D(4, 12, 4)),
    };

    m_normalArms = {
        // Right Arm
        new opengl::BoxGeometry(QVector3D(4, 12, 4), QVector3D(-6, -6, 0), QPoint(40, 16), QVector3D(4, 12, 4)),
        // Left Arm
        new opengl::BoxGeometry(QVector3D(4, 12, 4), QVector3D(6, -6, 0), QPoint(32, 48), QVector3D(4, 12, 4)),
    };

    m_normalArmsOverlay = {
        // Right Arm
        new opengl::BoxGeometry(QVector3D(4.5, 12.5, 4.5), QVector3D(-6, -6, 0), QPoint(40, 32), QVector3D(4, 12, 4)),
        // Left Arm
        new opengl::BoxGeometry(QVector3D(4.5, 12.5, 4.5), QVector3D(6, -6, 0), QPoint(48, 48), QVector3D(4, 12, 4)),
    };

    m_slimArms = {
        // Right Arm
        new opengl::BoxGeometry(QVector3D(3, 12, 4), QVector3D(-5.5, -6, 0), QPoint(40, 16), QVector3D(3, 12, 4)),
        // Left Arm
        new opengl::BoxGeometry(QVector3D(3, 12, 4), QVector3D(5.5, -6, 0), QPoint(32, 48), QVector3D(3, 12, 4)),
    };

    m_slimArmsOverlay = {
        // Right Arm
        new opengl::BoxGeometry(QVector3D(3.5, 12.5, 4.5), QVector3D(-5.5, -6, 0), QPoint(40, 32), QVector3D(3, 12, 4)),
        // Left Arm
        new opengl::BoxGeometry(QVector3D(3.5, 12.5, 4.5), QVector3D(5.5, -6, 0), QPoint(48, 48), QVector3D(3, 12, 4)),
    };

    m_cape = new opengl::BoxGeometry(QVector3D(10, 16, 1), QVector3D(0, -8, 2.5), QPoint(0, 0), QVector3D(10, 16, 1), QSize(64, 32));
    m_cape->rotate(10.8, QVector3D(1, 0, 0));
    m_cape->rotate(180, QVector3D(0, 1, 0));

    auto leftWing =
        new opengl::BoxGeometry(QVector3D(12, 22, 4), QVector3D(0, -13, -2), QPoint(22, 0), QVector3D(10, 20, 2), QSize(64, 32));
    leftWing->rotate(15, QVector3D(1, 0, 0));
    leftWing->rotate(15, QVector3D(0, 0, 1));
    leftWing->rotate(1, QVector3D(1, 0, 0));
    auto rightWing =
        new opengl::BoxGeometry(QVector3D(12, 22, 4), QVector3D(0, -13, -2), QPoint(22, 0), QVector3D(10, 20, 2), QSize(64, 32));
    rightWing->scale(QVector3D(-1, 1, 1));
    rightWing->rotate(15, QVector3D(1, 0, 0));
    rightWing->rotate(15, QVector3D(0, 0, 1));
    rightWing->rotate(1, QVector3D(1, 0, 0));
    m_elytra << leftWing << rightWing;

    // texture init
    m_skinTexture = new QOpenGLTexture(skin.mirrored());
    m_skinTexture->setMinificationFilter(QOpenGLTexture::Nearest);
    m_skinTexture->setMagnificationFilter(QOpenGLTexture::Nearest);

    m_capeTexture = new QOpenGLTexture(cape.mirrored());
    m_capeTexture->setMinificationFilter(QOpenGLTexture::Nearest);
    m_capeTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
}
Scene::~Scene()
{
    for (auto array :
         { m_staticComponents, m_normalArms, m_slimArms, m_elytra, m_staticComponentsOverlay, m_normalArmsOverlay, m_slimArmsOverlay }) {
        for (auto g : array) {
            delete g;
        }
    }
    delete m_cape;

    m_skinTexture->destroy();
    delete m_skinTexture;

    m_capeTexture->destroy();
    delete m_capeTexture;
}

void Scene::draw(QOpenGLShaderProgram* program)
{
    m_skinTexture->bind();
    program->setUniformValue("texture", 0);
    for (auto toDraw : { m_staticComponents, m_slim ? m_slimArms : m_normalArms, m_staticComponentsOverlay,
                         m_slim ? m_slimArmsOverlay : m_normalArmsOverlay }) {
        for (auto g : toDraw) {
            g->draw(program);
        }
    }
    m_skinTexture->release();
    if (m_capeVisible) {
        m_capeTexture->bind();
        program->setUniformValue("texture", 0);
        if (!m_elytraVisible) {
            m_cape->draw(program);
        } else {
            glDisable(GL_CULL_FACE);
            for (auto e : m_elytra) {
                e->draw(program);
            }
            glEnable(GL_CULL_FACE);
        }
        m_capeTexture->release();
    }
}

void updateTexture(QOpenGLTexture* texture, const QImage& img)
{
    if (texture) {
        if (texture->isBound())
            texture->release();
        texture->destroy();
        texture->create();
        texture->setSize(img.width(), img.height());
        texture->setData(img);
        texture->setMinificationFilter(QOpenGLTexture::Nearest);
        texture->setMagnificationFilter(QOpenGLTexture::Nearest);
    }
}

void Scene::setSkin(const QImage& skin)
{
    updateTexture(m_skinTexture, skin.mirrored());
}

void Scene::setMode(bool slim)
{
    m_slim = slim;
}
void Scene::setCape(const QImage& cape)
{
    updateTexture(m_capeTexture, cape.mirrored());
}
void Scene::setCapeVisible(bool visible)
{
    m_capeVisible = visible;
}
void Scene::setElytraVisible(bool elytraVisible)
{
    m_elytraVisible = elytraVisible;
}
}  // namespace opengl
