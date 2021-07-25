#define EV_MODULE_DEFINE
#include <evol/evolmod.h>


#define IMPORT_MODULE evmod_ecs
#include <evol/meta/module_import.h>
#define IMPORT_MODULE evmod_script
#include <evol/meta/module_import.h>
#define IMPORT_MODULE evmod_game
#include <evol/meta/module_import.h>

#include <physics_api.h>

struct {
  GameComponentID rigidbodyComponentID;
} Data;

void 
ev_physicsmod_scriptapi_loader(
    EVNS_ScriptInterface *ScriptInterface,
    ScriptContextHandle ctx_h);

// ECS stuff
typedef struct {
    RigidbodyHandle rbHandle;
} RigidbodyComponent;

void
RigidbodyComponentOnRemoveTrigger(
    ECSQuery query)
{
  RigidbodyComponent *rbComp = ECS->getQueryColumn(query, sizeof(RigidbodyComponent), 1);
  U32 count = ECS->getQueryMatchCount(query);

  for(U32 i = 0; i < count; i++) {
    RigidbodyComponent rb = rbComp[i];
    _ev_rigidbody_destroy(0, rb.rbHandle);
  }
}

EV_CONSTRUCTOR 
{ 
  evolmodule_t ecs_mod = evol_loadmodule_weak("ecs");
  if(ecs_mod) {
    imports(ecs_mod, (ECS, GameECS));
    if(GameECS) {
      Data.rigidbodyComponentID = GameECS->registerComponent("RigidbodyComponent", sizeof(RigidbodyComponent), EV_ALIGNOF(RigidbodyComponent));
      GameECS->setOnRemoveTrigger("RigidbodyComponentOnRemoveTrigger", "RigidbodyComponent", RigidbodyComponentOnRemoveTrigger);
    }
  }

  evolmodule_t script_mod = evol_loadmodule_weak("script");
  if(script_mod) {
    imports(script_mod, (Script, ScriptInterface));
    ScriptInterface->registerAPILoadFn(ev_physicsmod_scriptapi_loader);
  }

  evolmodule_t game_mod = evol_loadmodule_weak("game");
  if(game_mod) {
    imports(game_mod, (Scene, Object));
  }

  _ev_physics_init();
  _ev_physics_enablevisualization(visualize_physics);

  return 0;
}

EV_DESTRUCTOR 
{ 
  return _ev_physics_deinit(); 
} 

void
_ev_physics_dispatch_collisionenter(
    GameScene scene,
    U64 enttA,
    U64 enttB)
{
  vec(U64) *enttAList = Script->getCollisionEnterList(scene, enttA);
  vec(U64) *enttBList = Script->getCollisionEnterList(scene, enttB);

  if(enttAList != NULL) {
    vec_push(enttAList, &enttB);
  }
  if(enttBList != NULL) {
    vec_push(enttBList, &enttA);
  }
}

void
_ev_physics_dispatch_collisionleave(
    GameScene scene,
    U64 enttA,
    U64 enttB)
{
  vec(U64) *enttAList = Script->getCollisionLeaveList(scene, enttA);
  vec(U64) *enttBList = Script->getCollisionLeaveList(scene, enttB);

  if(enttAList != NULL) {
    vec_push(enttAList, &enttB);
  }
  if(enttBList != NULL) {
    vec_push(enttBList, &enttA);
  }
}

RigidbodyHandle
_ev_rigidbody_addtoentity(
    GameScene game_scene,
    GameEntityID entt,
    RigidbodyInfo rbInfo)
{
    RigidbodyComponent comp = {
        .rbHandle = _ev_rigidbody_new(game_scene, entt, rbInfo)
    };
    ECSGameWorldHandle ecs_world = Scene->getECSWorld(game_scene);
    GameECS->setComponent(ecs_world, entt, Data.rigidbodyComponentID, &comp);
    /* ev_log_trace("Set RigidbodyComponent for entity %llu", entt); */

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
  ECSGameWorldHandle ecs_world = Scene->getECSWorld(scene);
  const RigidbodyComponent *component = GameECS->getComponent(ecs_world, entt, Data.rigidbodyComponentID);
  if(component) {
    return *component;
  } else {
    /* ev_log_trace("Didn't find RigidbodyComponent for entity %llu", entt); */
    return (RigidbodyComponent) { NULL };
  }
}

EV_BINDINGS
{
    EV_NS_BIND_FN(PhysicsWorld, newWorld    , ev_physicsworld_newworld);
    EV_NS_BIND_FN(PhysicsWorld, invalidHandle    , ev_physicsworld_invalidhandle);
    EV_NS_BIND_FN(PhysicsWorld, destroyWorld, ev_physicsworld_destroyworld);
    EV_NS_BIND_FN(PhysicsWorld, progress    , ev_physicsworld_progress);

    EV_NS_BIND_FN(CollisionShape, newBox, _ev_collisionshape_newbox);
    EV_NS_BIND_FN(CollisionShape, newSphere, _ev_collisionshape_newsphere);
    EV_NS_BIND_FN(CollisionShape, newCapsule, _ev_collisionshape_newcapsule);
    EV_NS_BIND_FN(CollisionShape, newMesh, _ev_collisionshape_newmesh);

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
_ev_rigidbody_setvelocity_wrapper(
    EV_UNALIGNED RigidbodyHandle *handle,
    EV_UNALIGNED Vec3 *vel)
{
  _ev_rigidbody_setvelocity(*handle, Vec3new(
        vel->x,
        vel->y,
        vel->z));
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
ev_physics_raytest_wrapper(
    EV_UNALIGNED RayHit *out,
    EV_UNALIGNED Vec3 *orig,
    EV_UNALIGNED Vec3 *dir,
    EV_UNALIGNED float *len)
{
  RayHit res = ev_physics_raytest(NULL, 
      Vec3new(orig->x, orig->y, orig->z),
      Vec3new(dir->x, dir->y, dir->z),
      *len);

  *out = (RayHit) {
    .hasHit = res.hasHit,
    .hitPoint = Vec3new(res.hitPoint.x, res.hitPoint.y, res.hitPoint.z),
    .hitNormal = Vec3new(res.hitNormal.x, res.hitNormal.y, res.hitNormal.z),
    .object_id = res.object_id
  };
}

void 
ev_physicsmod_scriptapi_loader(
    EVNS_ScriptInterface *ScriptInterface,
    ScriptContextHandle ctx_h)
{
  ScriptType boolSType = ScriptInterface->getType(ctx_h, "bool");
  ScriptType voidSType = ScriptInterface->getType(ctx_h, "void");
  ScriptType floatSType = ScriptInterface->getType(ctx_h, "float");
  ScriptType ullSType = ScriptInterface->getType(ctx_h, "unsigned long long");

  ScriptType rigidbodyHandleSType = ScriptInterface->addType(ctx_h, "void*", sizeof(void*));
  ScriptType rigidbodyComponentSType = ScriptInterface->addStruct(ctx_h, "RigidbodyComponent", sizeof(RigidbodyComponent), 1, (ScriptStructMember[]) {
      {"handle", rigidbodyHandleSType, offsetof(RigidbodyComponent, rbHandle)}
  });

  ScriptType vec3SType = ScriptInterface->addStruct(ctx_h, "Vec3", sizeof(Vec3), 3, (ScriptStructMember[]) {
      {"x", floatSType, offsetof(Vec3, x)},
      {"y", floatSType, offsetof(Vec3, y)},
      {"z", floatSType, offsetof(Vec3, z)}
  });

  ScriptType rayHitSType = ScriptInterface->addStruct(ctx_h, "RayHit", sizeof(RayHit), 4, (ScriptStructMember[]) {
      {"hitPoint", vec3SType, offsetof(RayHit, hitPoint)},
      {"hitNormal", vec3SType, offsetof(RayHit, hitNormal)},
      {"object_id", ullSType, offsetof(RayHit, object_id)},
      {"hasHit", boolSType, offsetof(RayHit, hasHit)}
  });

  ScriptInterface->addFunction(ctx_h, _ev_rigidbody_addforce_wrapper, "ev_rigidbody_addforce", voidSType, 2, (ScriptType[]){rigidbodyHandleSType, vec3SType});
  ScriptInterface->addFunction(ctx_h, _ev_rigidbody_getfromentity_wrapper, "ev_rigidbody_getfromentity", rigidbodyHandleSType, 1, (ScriptType[]){ullSType});
  ScriptInterface->addFunction(ctx_h, _ev_rigidbody_getinvalidhandle_wrapper, "ev_rigidbody_getinvalidhandle", rigidbodyHandleSType, 0, NULL);
  ScriptInterface->addFunction(ctx_h, _ev_rigidbody_getcomponentfromentity_wrapper, "ev_rigidbody_getcomponentfromentity", rigidbodyComponentSType, 1, (ScriptType[]){ullSType});
  ScriptInterface->addFunction(ctx_h, _ev_rigidbody_setposition_wrapper, "ev_rigidbody_setposition", voidSType, 2, (ScriptType[]){rigidbodyHandleSType, vec3SType});
  ScriptInterface->addFunction(ctx_h, _ev_rigidbody_setvelocity_wrapper, "ev_rigidbody_setvelocity", voidSType, 2, (ScriptType[]){rigidbodyHandleSType, vec3SType});

  ScriptInterface->addFunction(ctx_h, _ev_rigidbody_setrotationeuler_wrapper, "ev_rigidbody_setrotationeuler", voidSType, 2, (ScriptType[]){rigidbodyHandleSType, vec3SType});

  ScriptInterface->addFunction(ctx_h, ev_physics_raytest_wrapper, "ev_physics_raytest", rayHitSType, 3, (ScriptType[]){vec3SType, vec3SType, floatSType});


  ScriptInterface->loadAPI(ctx_h, "subprojects/evmod_physics/script_api.lua");
  ev_log_trace("Successfully loaded physics API");
}
