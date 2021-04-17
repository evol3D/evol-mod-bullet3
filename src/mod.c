#define EV_MODULE_DEFINE
#include <evol/evolmod.h>

#include <physics_api.h>

#define TYPE_MODULE evmod_ecs
#define NAMESPACE_MODULE evmod_ecs
#include <evol/meta/type_import.h>
#include <evol/meta/namespace_import.h>

struct {
  evolmodule_t ecs_mod;
  ECSEntityID rigidbodyComponentID;
} Data;

EV_CONSTRUCTOR 
{ 
  Data.ecs_mod = NULL;
  return _ev_physics_init(); 
}

EV_DESTRUCTOR 
{ 
  if(Data.ecs_mod != NULL) {
    evol_unloadmodule(Data.ecs_mod);
  }
  return _ev_physics_deinit(); 
} 

// ECS stuff
typedef struct {
    RigidbodyHandle rbHandle;
} RigidbodyComponent;

void
_ev_physics_initecs()
{
  Data.ecs_mod = evol_loadmodule("ecs");
  IMPORT_NAMESPACE(ECS, Data.ecs_mod);

  Data.rigidbodyComponentID = ECS->registerComponent("RigidbodyComponent", sizeof(RigidbodyComponent), EV_ALIGNOF(RigidbodyComponent));
}

void
_ev_rigidbody_addtoentity(
        ECSEntityID entt,
        RigidbodyHandle rb)
{
    RigidbodyComponent comp = {
        .rbHandle = rb
    };
    ECS->addComponent(entt, Data.rigidbodyComponentID, sizeof(RigidbodyComponent), &comp);
}

RigidbodyHandle
_ev_rigidbody_getfromentity(
        ECSEntityID entt)
{
    RigidbodyComponent *rbComp = ECS->getComponent(entt, Data.rigidbodyComponentID);
    return rbComp->rbHandle;
}

EV_BINDINGS
{
    EV_NS_BIND_FN(Physics, update, _ev_physics_update);
    EV_NS_BIND_FN(Physics, initECS, _ev_physics_initecs);

    EV_NS_BIND_FN(CollisionShape, newBox, _ev_collisionshape_newbox);
    EV_NS_BIND_FN(CollisionShape, newSphere, _ev_collisionshape_newsphere);

    EV_NS_BIND_FN(Rigidbody, new, _ev_rigidbody_new);
    EV_NS_BIND_FN(Rigidbody, setPosition, _ev_rigidbody_setposition);
    EV_NS_BIND_FN(Rigidbody, getPosition, _ev_rigidbody_getposition);
    EV_NS_BIND_FN(Rigidbody, addToEntity, _ev_rigidbody_addtoentity);
    EV_NS_BIND_FN(Rigidbody, getFromEntity, _ev_rigidbody_getfromentity);
    EV_NS_BIND_FN(Rigidbody, addForce, _ev_rigidbody_addforce);
}
