#define EV_MODULE_DEFINE
#include <evol/evolmod.h>

#include <physics_api.h>

#define IMPORT_MODULE evmod_ecs
#include <evol/meta/module_import.h>
#define IMPORT_MODULE evmod_script
#include <evol/meta/module_import.h>
#define IMPORT_MODULE evmod_game
#include <evol/meta/module_import.h>

struct {
  evolmodule_t ecs_mod;
  evolmodule_t script_mod;
  evolmodule_t game_mod;
  GameComponentID rigidbodyComponentID;
} Data;

void 
init_scripting_api();

// ECS stuff
typedef struct {
    RigidbodyHandle rbHandle;
} RigidbodyComponent;

EV_CONSTRUCTOR 
{ 
  Data.ecs_mod = evol_loadmodule("ecs");
  if(Data.ecs_mod) {
    imports(Data.ecs_mod, (GameECS));
    if(GameECS) {
      Data.rigidbodyComponentID = GameECS->registerComponent("RigidbodyComponent", sizeof(RigidbodyComponent), EV_ALIGNOF(RigidbodyComponent));
    }
  }

  Data.script_mod = evol_loadmodule("script");
  if(Data.script_mod) {
    imports(Data.script_mod, (Script, ScriptInterface));
    init_scripting_api();
  }

  Data.game_mod = evol_loadmodule("game");
  if(Data.game_mod) {
    imports(Data.game_mod, (Object));
  }

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
  if(Data.game_mod != NULL) {
    evol_unloadmodule(Data.game_mod);
  }

  return _ev_physics_deinit(); 
} 

void
_ev_physics_dispatch_collisionenter(
    ECSGameWorldHandle world_handle,
    U64 enttA,
    U64 enttB)
{
  vec(U64) *enttAList = Script->getCollisionEnterList(world_handle, enttA);
  vec(U64) *enttBList = Script->getCollisionEnterList(world_handle, enttB);

  if(enttAList != NULL) {
    vec_push(enttAList, &enttB);
  }
  if(enttBList != NULL) {
    vec_push(enttBList, &enttA);
  }
}

void
_ev_physics_dispatch_collisionleave(
    ECSGameWorldHandle world_handle,
    U64 enttA,
    U64 enttB)
{
  vec(U64) *enttAList = Script->getCollisionLeaveList(world_handle, enttA);
  vec(U64) *enttBList = Script->getCollisionLeaveList(world_handle, enttB);

  if(enttAList != NULL) {
    vec_push(enttAList, &enttB);
  }
  if(enttBList != NULL) {
    vec_push(enttBList, &enttA);
  }
}

RigidbodyHandle
_ev_rigidbody_addtoentity(
    PhysicsWorldHandle world,
    GameEntityID entt,
    RigidbodyInfo *rbInfo)
{
    RigidbodyComponent comp = {
        .rbHandle = _ev_rigidbody_new(world, entt, rbInfo)
    };
    GameECS->setComponent(rbInfo->ecs_world, entt, Data.rigidbodyComponentID, &comp);

    return comp.rbHandle;
}

RigidbodyHandle
_ev_rigidbody_getfromentity(
    GameScene scene,
    GameEntityID entt)
{
  RigidbodyComponent *rbComp = Object->getComponent(scene, entt, Data.rigidbodyComponentID);
  return rbComp->rbHandle;
}

RigidbodyComponent
_ev_rigidbody_getcomponentfromentity(
    GameScene scene,
    ECSEntityID entt)
{
  if(Object->hasComponent(scene, entt, Data.rigidbodyComponentID)) {
    return *(RigidbodyComponent*)Object->getComponent(scene, entt, Data.rigidbodyComponentID);
  }
  return (RigidbodyComponent) {
    .rbHandle = NULL
  };
}

EV_BINDINGS
{
    EV_NS_BIND_FN(PhysicsWorld, newWorld    , ev_physicsworld_newworld);
    EV_NS_BIND_FN(PhysicsWorld, destroyWorld, ev_physicsworld_destroyworld);
    EV_NS_BIND_FN(PhysicsWorld, progress    , ev_physicsworld_progress);

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
    EV_UNALIGNED RigidbodyHandle *out, 
    EV_UNALIGNED ECSEntityID *entt)
{
  *out = _ev_rigidbody_getfromentity(NULL, *entt);
}

void
_ev_rigidbody_getinvalidhandle_wrapper(
    EV_UNALIGNED RigidbodyHandle *out)
{
  *out = NULL;
}

void _ev_rigidbody_getcomponentfromentity_wrapper(
    EV_UNALIGNED RigidbodyComponent *out,
    EV_UNALIGNED ECSEntityID *entt)
{
  *out = _ev_rigidbody_getcomponentfromentity(NULL, *entt);
}

void
_ev_rigidbody_addforce_wrapper(
    EV_UNALIGNED RigidbodyHandle *handle,
    EV_UNALIGNED Vec3 *force)
{
  _ev_rigidbody_addforce(*handle, Vec3new(
        force->x,
        force->y,
        force->z));
}

void
_ev_rigidbody_setposition_wrapper(
    EV_UNALIGNED RigidbodyHandle *handle,
    EV_UNALIGNED Vec3 *pos)
{
  _ev_rigidbody_setposition(*handle, Vec3new(
        pos->x,
        pos->y,
        pos->z));
}

void
_ev_rigidbody_setrotationeuler_wrapper(
    EV_UNALIGNED RigidbodyHandle *handle,
    EV_UNALIGNED Vec3 *rot)
{
  _ev_rigidbody_setrotationeuler(*handle, Vec3new(
        rot->x,
        rot->y,
        rot->z));
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
  ScriptInterface->addFunction(_ev_rigidbody_getinvalidhandle_wrapper, "ev_rigidbody_getinvalidhandle", rigidbodyHandleSType, 0, NULL);
  ScriptInterface->addFunction(_ev_rigidbody_getcomponentfromentity_wrapper, "ev_rigidbody_getcomponentfromentity", rigidbodyComponentSType, 1, (ScriptType[]){ullSType});
  ScriptInterface->addFunction(_ev_rigidbody_setposition_wrapper, "ev_rigidbody_setposition", voidSType, 2, (ScriptType[]){rigidbodyHandleSType, vec3SType});

  ScriptInterface->addFunction(_ev_rigidbody_setrotationeuler_wrapper, "ev_rigidbody_setrotationeuler", voidSType, 2, (ScriptType[]){rigidbodyHandleSType, vec3SType});

  ScriptInterface->loadAPI("subprojects/evmod_physics/script_api.lua");
}
