#pragma once

#include <btBulletCollisionCommon.h>

#include <evol/evol.h>

#define TYPE_MODULE evmod_glfw
#include <evol/meta/type_import.h>

#define NAMESPACE_MODULE evmod_glfw
#include <evol/meta/namespace_import.h>

class BulletDbg: public btIDebugDraw
{
private:
    evolmodule_t window_module;
    WindowHandle dbgWindow;

public:
    I32 windowDestroyed;

public:
    BulletDbg()
    {
        window_module = evol_loadmodule("window");
        DEBUG_ASSERT(window_module);
        IMPORT_NAMESPACE(Window, window_module);
        IMPORT_NAMESPACE(DbgWindow, window_module);
        IMPORT_NAMESPACE(imGL, window_module);

        dbgWindow = DbgWindow->create(800, 600, "Physics Debug");

        windowDestroyed = 0;
    }

    ~BulletDbg()
    {
        DbgWindow->destroy(dbgWindow);
        evol_unloadmodule(window_module);
    }

	inline void 
    drawLine(
        const btVector3& from, 
        const btVector3& to, 
        const btVector3& color) override
    {
        imGL->setColor3f(1.0, color.y(), color.z());
        imGL->drawLine(
            from.x(), from.y(), from.z(),
            to.x(), to.y(), to.z());
    }

	inline void 
    drawContactPoint(
        const btVector3& PointOnB, 
        const btVector3& normalOnB, 
        btScalar distance, 
        int lifeTime, 
        const btVector3& color) override
    {
        UNIMPLEMENTED();
    }

	inline void 
    reportErrorWarning(
        const char* warningString) override
    {
        ev_log_error("Bullet Error: %s", warningString);
    }

	inline void 
    draw3dText(
        const btVector3& location, 
        const char* textString) override
    {
        UNIMPLEMENTED();
    }

	inline void 
    setDebugMode(
        int debugMode) override
    {
        UNIMPLEMENTED();
    }

	inline int 
    getDebugMode() const override
    {
        return DBG_DrawWireframe | DBG_DrawAabb;
    }

    inline void 
    startFrame()
    {
        U32 width, height;
        Window->getSize(dbgWindow, &width, &height);
        DbgWindow->startFrame(dbgWindow);

        imGL->setViewport(0, 0, width, height);
        imGL->projPersp(120, (F32)width/(F32)height, 0.1, 1000.0);
        imGL->setCameraView(0.0, 0.0, 0.0, 0.0, 0.0, -1.0);

        imGL->setClearColor(0.0, 0.0, 0.0, 1.0);
        imGL->clearBuffers();
    }

    inline void 
    endFrame()
    {
        DbgWindow->endFrame(dbgWindow);
        windowDestroyed = DbgWindow->update(dbgWindow);
    }
};
