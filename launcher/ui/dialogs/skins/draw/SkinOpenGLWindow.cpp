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

#include "ui/dialogs/skins/draw/SkinOpenGLWindow.h"

#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QProcessEnvironment>
#include <QVector2D>
#include <QVector3D>
#include <QtMath>

#include "BuildConfig.h"
#include "minecraft/skins/SkinModel.h"
#include "ui/dialogs/skins/draw/Scene.h"

SkinOpenGLWindow::SkinOpenGLWindow(SkinProvider* parent, QColor color)
    : QOpenGLWindow(), QOpenGLFunctions(), m_baseColor(color), m_parent(parent)
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setDepthBufferSize(24);
    setFormat(format);
}

SkinOpenGLWindow::~SkinOpenGLWindow()
{
    // Make sure the context is current when deleting the texture
    // and the buffers.
    makeCurrent();
    // double check if resources were initialized because they are not
    // initialized together with the object
    if (m_scene) {
        delete m_scene;
    }
    if (m_modelProgram) {
        if (m_modelProgram->isLinked()) {
            m_modelProgram->release();
        }
        m_modelProgram->removeAllShaders();
        delete m_modelProgram;
    }
    doneCurrent();
}

void SkinOpenGLWindow::mousePressEvent(QMouseEvent* e)
{
    // Save mouse press position
    m_mousePosition = QVector2D(e->pos());
    m_isMousePressed = true;
}

void SkinOpenGLWindow::mouseMoveEvent(QMouseEvent* event)
{
    // Prevents mouse sticking on Wayland compositors
    if (!(event->buttons() & Qt::MouseButton::LeftButton)) {
        m_isMousePressed = false;
        return;
    }

    if (m_isMousePressed) {
        int dx = event->position().x() - m_mousePosition.x();
        int dy = event->position().y() - m_mousePosition.y();

        m_yaw += dx * 0.5f;
        m_pitch += dy * 0.5f;

        // Normalize yaw to keep it manageable
        if (m_yaw > 360.0f)
            m_yaw -= 360.0f;
        else if (m_yaw < 0.0f)
            m_yaw += 360.0f;

        m_mousePosition = QVector2D(event->pos());
        update();  // Trigger a repaint
    }
}

void SkinOpenGLWindow::mouseReleaseEvent([[maybe_unused]] QMouseEvent* e)
{
    m_isMousePressed = false;
}

void SkinOpenGLWindow::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(m_baseColor.redF(), m_baseColor.greenF(), m_baseColor.blueF(), 1.f);

    initShaders();

    QImage skin, cape;
    bool slim = false;
    if (m_parent) {
        if (auto s = m_parent->getSelectedSkin()) {
            skin = s->getTexture();
            slim = s->getModel() == SkinModel::SLIM;
            cape = m_parent->capes().value(s->getCapeId(), {});
        }
    }

    m_scene = new opengl::Scene(skin, slim, cape);
    glEnable(GL_TEXTURE_2D);
}

void SkinOpenGLWindow::initShaders()
{
    // Skin model shaders
    m_modelProgram = new QOpenGLShaderProgram(this);
    // Compile vertex shader
    if (!m_modelProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vshader_skin_model.glsl"))
        close();

    // Compile fragment shader
    if (!m_modelProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fshader.glsl"))
        close();

    // Link shader pipeline
    if (!m_modelProgram->link())
        close();

    // Bind shader pipeline for use
    if (!m_modelProgram->bind())
        close();
}

void SkinOpenGLWindow::resizeGL(int w, int h)
{
    // Calculate aspect ratio
    qreal aspect = qreal(w) / qreal(h ? h : 1);

    const qreal zNear = 15., fov = 45;

    // Reset projection
    m_projection.setToIdentity();

    // Build the reverse z perspective projection matrix
    double radians = qDegreesToRadians(fov / 2.);
    double sine = std::sin(radians);
    if (sine == 0)
        return;
    double cotan = std::cos(radians) / sine;

    m_projection(0, 0) = cotan / aspect;
    m_projection(1, 1) = cotan;
    m_projection(2, 2) = 0.;
    m_projection(3, 2) = -1.;
    m_projection(2, 3) = zNear;
    m_projection(3, 3) = 0.;
}

void SkinOpenGLWindow::paintGL()
{
    // Adjust the viewport to account for fractional scaling
    qreal dpr = devicePixelRatio();
    if (dpr != 1.f) {
        QSize scaledSize = size() * dpr;
        glViewport(0, 0, scaledSize.width(), scaledSize.height());
    }

    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Calculate model view transformation
    QMatrix4x4 matrix;
    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);
    matrix.lookAt(QVector3D(                                       //
                      m_distance * qCos(pitchRad) * qCos(yawRad),  //
                      m_distance * qSin(pitchRad) - 8,             //
                      m_distance * qCos(pitchRad) * qSin(yawRad)),
                  QVector3D(0, -8, 0), QVector3D(0, 1, 0));

    // Set modelview-projection matrix
    m_modelProgram->bind();
    m_modelProgram->setUniformValue("mvp_matrix", m_projection * matrix);

    m_scene->draw(m_modelProgram);
    m_modelProgram->release();

    // Redraw the first frame; this is necessary because the pixel ratio for Wayland fractional scaling is not negotiated properly on the
    // first frame
    if (m_isFirstFrame) {
        m_isFirstFrame = false;
        update();
    }
}

void SkinOpenGLWindow::updateScene(SkinModel* skin)
{
    if (skin && m_scene) {
        m_scene->setMode(skin->getModel() == SkinModel::SLIM);
        m_scene->setSkin(skin->getTexture());
        update();
    }
}
void SkinOpenGLWindow::updateCape(const QImage& cape)
{
    if (m_scene) {
        m_scene->setCapeVisible(!cape.isNull());
        m_scene->setCape(cape);
        update();
    }
}

void SkinOpenGLWindow::wheelEvent(QWheelEvent* event)
{
    // Adjust distance based on scroll
    int delta = event->angleDelta().y();  // Positive for scroll up, negative for scroll down
    m_distance -= delta * 0.01f;          // Adjust sensitivity factor
    m_distance = qMax(16.f, m_distance);  // Clamp distance
    update();                             // Trigger a repaint
}
void SkinOpenGLWindow::setElytraVisible(bool visible)
{
    if (m_scene) {
        m_scene->setElytraVisible(visible);
        update();
    }
}

bool SkinOpenGLWindow::hasOpenGL()
{
    if (!QProcessEnvironment::systemEnvironment()
             .value(QStringLiteral("%1_DISABLE_GLVULKAN").arg(BuildConfig.LAUNCHER_ENVNAME))
             .isEmpty()) {
        return false;
    }

    QOpenGLContext ctx;
    return ctx.create();
}
