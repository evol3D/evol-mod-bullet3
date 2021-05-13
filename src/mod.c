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

void 
init_scripting_api();

EV_CONSTRUCTOR 
{ 
  Data.ecs_mod = NULL;
  init_scripting_api();
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

RigidbodyComponent
_ev_rigidbody_getcomponentfromentity(
    ECSEntityID entt)
{
  return *(RigidbodyComponent*)ECS->getComponent(entt, Data.rigidbodyComponentID);
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

// Initializing the scripting API
#define TYPE_MODULE evmod_script
#define NAMESPACE_MODULE evmod_script
#include <evol/meta/type_import.h>
#include <evol/meta/namespace_import.h>

void
_ev_rigidbody_getfromentity_wrapper(
    RigidbodyHandle *out, 
    ECSEntityID *entt)
{
  *out = _ev_rigidbody_getfromentity(*entt);
}

void _ev_rigidbody_getcomponentfromentity_wrapper(
    RigidbodyComponent *out,
    ECSEntityID *entt)
{
  *out = _ev_rigidbody_getcomponentfromentity(*entt);
}

void
_ev_rigidbody_addforce_wrapper(
    RigidbodyHandle *handle,
    Vec3 *f)
{
  Vec3 force = Vec3new(f->x, f->y, f->z); // A new Vec3 is created to ensure alignment
  _ev_rigidbody_addforce(*handle, force);
}

void
_ev_rigidbody_setposition_wrapper(
    RigidbodyHandle *handle,
    Vec3 *f)
{
  Vec3 force = Vec3new(f->x, f->y, f->z); // A new Vec3 is created to ensure alignment
  _ev_rigidbody_setposition(*handle, force);
}

void 
init_scripting_api()
{
  evolmodule_t scripting_module = evol_loadmodule("script");
  if(!scripting_module) return;
  IMPORT_NAMESPACE(ScriptInterface, scripting_module);

  ScriptType voidSType = ScriptInterface->getType("void");
  ScriptType floatSType = ScriptInterface->getType("float");
  ScriptType ullSType = ScriptInterface->getType("unsigned long long");

  ScriptType rigidbodyHandleSType = ScriptInterface->addType("void*", sizeof(void*));
  ScriptType rigidbodyComponentSType = ScriptInterface->addStruct("RigidbodyComponent", sizeof(RigidbodyComponent), 1, (ScriptStructMember[]) {
      {"handle", rigidbodyHandleSType, offsetof(RigidbodyComponent, rbHandle)}
  });

  ScriptType vec3SType = ScriptInterface->addStruct("Vec3", sizeof(Vec3), 3, (ScriptStructMember[]) {
      {"x", floatSType, offsetof(Vec3, x)},
      {"y", floatSType, offsetof(Vec3, y)},
      {"z", floatSType, offsetof(Vec3, z)}
  });

  ScriptInterface->addFunction(_ev_rigidbody_addforce_wrapper, "ev_rigidbody_addforce", voidSType, 2, (ScriptType[]){rigidbodyHandleSType, vec3SType});
  ScriptInterface->addFunction(_ev_rigidbody_getfromentity_wrapper, "ev_rigidbody_getfromentity", rigidbodyHandleSType, 1, (ScriptType[]){ullSType});
  ScriptInterface->addFunction(_ev_rigidbody_getcomponentfromentity_wrapper, "ev_rigidbody_getcomponentfromentity", rigidbodyComponentSType, 1, (ScriptType[]){ullSType});
  ScriptInterface->addFunction(_ev_rigidbody_setposition_wrapper, "ev_rigidbody_setposition", voidSType, 2, (ScriptType[]){rigidbodyHandleSType, vec3SType});

  ScriptInterface->loadAPI("subprojects/evmod_physics/script_api.lua");

  evol_unloadmodule(scripting_module);
  // Invalidating namespace reference as the module is unloaded
  ScriptInterface = NULL;
}
