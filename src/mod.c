#define EV_MODULE_DEFINE
#include <evol/evolmod.h>

#include <physics_api.h>

#define IMPORT_MODULE evmod_ecs
#include <evol/meta/module_import.h>
#define IMPORT_MODULE evmod_script
#include <evol/meta/module_import.h>

struct {
  evolmodule_t ecs_mod;
  evolmodule_t script_mod;
  ECSEntityID rigidbodyComponentID;
} Data;

void 
init_scripting_api();

EV_CONSTRUCTOR 
{ 
  Data.ecs_mod = NULL;
  Data.script_mod = evol_loadmodule("script");

  IMPORT_NAMESPACE(Script, Data.script_mod);
  IMPORT_NAMESPACE(ScriptInterface, Data.script_mod);

  init_scripting_api();
  _ev_physics_init();

  _ev_physics_enablevisualization(visualize_physics);

  return 0;
}

EV_DESTRUCTOR 
{ 
  if(Data.ecs_mod != NULL) {
    evol_unloadmodule(Data.ecs_mod);
  }
  if(Data.script_mod != NULL) {
    evol_unloadmodule(Data.script_mod);
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

RigidbodyHandle
_ev_rigidbody_addtoentity(
        ECSEntityID entt,
        RigidbodyInfo *rbInfo)
{
    RigidbodyComponent comp = {
        .rbHandle = _ev_rigidbody_new(entt, rbInfo)
    };
    ECS->addComponent(entt, Data.rigidbodyComponentID, sizeof(RigidbodyComponent), &comp);

    return comp.rbHandle;
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
  if(ECS->hasComponent(entt, Data.rigidbodyComponentID)) {
    return *(RigidbodyComponent*)ECS->getComponent(entt, Data.rigidbodyComponentID);
  }
  return (RigidbodyComponent) {
    .rbHandle = NULL
  };
}

EV_BINDINGS
{
    EV_NS_BIND_FN(Physics, update, _ev_physics_update);
    EV_NS_BIND_FN(Physics, initECS, _ev_physics_initecs);

    EV_NS_BIND_FN(CollisionShape, newBox, _ev_collisionshape_newbox);
    EV_NS_BIND_FN(CollisionShape, newSphere, _ev_collisionshape_newsphere);

    EV_NS_BIND_FN(Rigidbody, setPosition, _ev_rigidbody_setposition);
    EV_NS_BIND_FN(Rigidbody, getPosition, _ev_rigidbody_getposition);
    EV_NS_BIND_FN(Rigidbody, addToEntity, _ev_rigidbody_addtoentity);
    EV_NS_BIND_FN(Rigidbody, getFromEntity, _ev_rigidbody_getfromentity);
    EV_NS_BIND_FN(Rigidbody, addForce, _ev_rigidbody_addforce);
}

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
    Vec3 *p)
{
  Vec3 pos = Vec3new(p->x, p->y, p->z); // A new Vec3 is created to ensure alignment
  _ev_rigidbody_setposition(*handle, pos);
}

void
_ev_rigidbody_setrotationeuler_wrapper(
    RigidbodyHandle *handle,
    Vec3 *r)
{
  Vec3 rot = Vec3new(r->x, r->y, r->z);
  _ev_rigidbody_setrotationeuler(*handle, rot);
}

void 
init_scripting_api()
{
  if(!ScriptInterface) return;

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

  ScriptInterface->addFunction(_ev_rigidbody_setrotationeuler_wrapper, "ev_rigidbody_setrotationeuler", voidSType, 2, (ScriptType[]){rigidbodyHandleSType, vec3SType});

  ScriptInterface->loadAPI("subprojects/evmod_physics/script_api.lua");
}
