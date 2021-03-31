#define EV_MODULE_DEFINE
#include <evol/evolmod.h>

#include <physics_api.h>

EV_CONSTRUCTOR { _ev_physics_init(); }
EV_DESTRUCTOR { _ev_physics_deinit(); } 

EV_BINDINGS
{
    EV_NS_BIND_FN(Physics, update, _ev_physics_update);
}