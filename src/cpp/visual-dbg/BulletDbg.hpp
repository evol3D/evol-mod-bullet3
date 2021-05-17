#pragma once

#include <btBulletCollisionCommon.h>

#include <evol/evol.h>

#define TYPE_MODULE evmod_glfw
#include <evol/meta/type_import.h>

class BulletDbg: public btIDebugDraw
{
private:
    evolmodule_t window_module;
    evolmodule_t game_module;
    WindowHandle dbgWindow;

public:
    I32 windowDestroyed;

public:
    void startFrame();
    void endFrame();

    BulletDbg();
    ~BulletDbg();

    void drawLine( const btVector3& from, const btVector3& to, const btVector3& color) override;
    void drawContactPoint( const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
    void reportErrorWarning( const char* warningString) override;
    void draw3dText( const btVector3& location, const char* textString) override;
    void setDebugMode( int debugMode) override;
    int getDebugMode() const override;
};
