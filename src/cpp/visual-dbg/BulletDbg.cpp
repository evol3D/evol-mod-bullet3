#pragma once

#include "BulletDbg.hpp"
#include <evol/common/ev_log.h>

#define NAMESPACE_MODULE evmod_glfw
#include <evol/meta/namespace_import.h>

#define TYPE_MODULE evmod_game
#include <evol/meta/type_import.h>
#define NAMESPACE_MODULE evmod_game
#include <evol/meta/namespace_import.h>

BulletDbg::BulletDbg()
{
    window_module = evol_loadmodule("window");
    game_module = evol_loadmodule_weak("game");
    DEBUG_ASSERT(window_module);
    DEBUG_ASSERT(game_module);
    imports(window_module, (Window, DbgWindow, imGL));
    imports(game_module, (Camera, Scene));

    dbgWindow = DbgWindow->create(800, 600, "Physics Debug");

    windowDestroyed = 0;
}

BulletDbg::~BulletDbg()
{
    DbgWindow->destroy(dbgWindow);
    evol_unloadmodule(window_module);
}

#define bt2evVec3(v) {{v.x(), v.y(), v.z()}}

void 
BulletDbg::drawLine(
    const btVector3& from, 
    const btVector3& to, 
    const btVector3& color)
{
    imGL->setColor3f(1.0, color.y(), color.z());
    imGL->drawLine(bt2evVec3(from), bt2evVec3(to));
}

void 
BulletDbg::drawContactPoint(
    const btVector3& PointOnB, 
    const btVector3& normalOnB, 
    btScalar distance, 
    int lifeTime, 
    const btVector3& color)
{
    UNIMPLEMENTED();
}

void 
BulletDbg::reportErrorWarning(
    const char* warningString)
{
    ev_log_error("Bullet Error: %s", warningString);
}

void 
BulletDbg::draw3dText(
    const btVector3& location, 
    const char* textString)
{
    UNIMPLEMENTED();
}

void 
BulletDbg::setDebugMode(
    int debugMode)
{
    UNIMPLEMENTED();
}

int 
BulletDbg::getDebugMode() const
{
    return DBG_DrawWireframe | DBG_DrawAabb;
}

void 
BulletDbg::startFrame()
{
    I32 width, height;
    Window->getSize(dbgWindow, &width, &height);
    DbgWindow->startFrame(dbgWindow);

    GameObject activeCamera = Scene->getActiveCamera(NULL);

    Matrix4x4 projectionMat;
    Matrix4x4 viewMat;
    Camera->getViewMat(NULL, activeCamera, viewMat);
    Camera->getProjectionMat(NULL, activeCamera, projectionMat);

    imGL->setViewport(0, 0, width, height);
    imGL->setCameraViewMat(viewMat);
    imGL->setCameraProjMat(projectionMat);

    imGL->setClearColor({{ 0.0, 0.0, 0.0, 1.0 }});
    imGL->clearBuffers();
}

void 
BulletDbg::endFrame()
{
    DbgWindow->endFrame(dbgWindow);
    windowDestroyed = DbgWindow->update(dbgWindow);
}
