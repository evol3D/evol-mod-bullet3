#define EV_MODULE_DEFINE
#include <evol/evolmod.h>

#include <physics_api.h>

EV_CONSTRUCTOR { return _ev_physics_init(); }
EV_DESTRUCTOR { return _ev_physics_deinit(); } 

EV_BINDINGS
{
    EV_NS_BIND_FN(Physics, update, _ev_physics_update);

    EV_NS_BIND_FN(Physics, createBoxShape, _ev_physics_createboxshape);
    EV_NS_BIND_FN(Physics, createSphereShape, _ev_physics_createsphereshape);

    EV_NS_BIND_FN(Physics, createRigidBody, _ev_physics_createrigidbody);

    EV_NS_BIND_FN(Physics, setRigidBodyPosition, _ev_physics_setrigidbodyposition);
}