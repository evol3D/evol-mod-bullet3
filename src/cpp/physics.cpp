#include <evol/common/ev_types.h>
#include <evol/common/ev_log.h>

#define TYPE_MODULE evmod_physics
#include <evol/meta/type_import.h>

#include <physics_api.h>

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#include "visual-dbg/BulletDbg.hpp"

#include <mutex>

#include <EvMotionState.h>

#include <vector>

#define ev2btVec3(v) btVector3(v.x, v.y, v.z)
#define bt2evVec3(v) {{  v.x(), v.y(), v.z() }}

struct RigidbodyData {
  GenericHandle entt_id;
  GenericHandle game_scene;
};

struct PhysicsWorld {
  btCollisionConfiguration *collisionConfiguration;
  btCollisionDispatcher *collisionDispatcher;
  btBroadphaseInterface *broadphase;
  btSequentialImpulseConstraintSolver *constraintSolver;
  btDynamicsWorld *world;

  btAlignedObjectArray<btCollisionShape*> collisionShapes;

  std::mutex worldMtx;
  std::mutex shapeVecMtx;

  PhysicsWorld(const PhysicsWorld& old) 
  {
    collisionConfiguration = old.collisionConfiguration;
    collisionDispatcher = old.collisionDispatcher;
    broadphase = old.broadphase;
    constraintSolver = old.constraintSolver;
    world = old.world;

    collisionShapes = old.collisionShapes;
  }

  PhysicsWorld() = default;
};


struct ev_PhysicsData {
  std::vector<PhysicsWorld> worlds;

  BulletDbg *debugDrawer;

  evolmodule_t game_mod;

  bool visualizationEnabled;
} PhysicsData;

void 
contactStartedCallback(
    btPersistentManifold* const& manifold);
void 
contactEndedCallback(
    btPersistentManifold* const& manifold);

PhysicsWorldHandle
ev_physicsworld_newworld()
{
  PhysicsWorld newWorld;
  newWorld.collisionConfiguration = new btDefaultCollisionConfiguration();
  newWorld.collisionDispatcher = new btCollisionDispatcher(newWorld.collisionConfiguration);
  newWorld.broadphase = new btDbvtBroadphase();
  newWorld.constraintSolver = new btSequentialImpulseConstraintSolver();
  newWorld.world = new btDiscreteDynamicsWorld(newWorld.collisionDispatcher, newWorld.broadphase, newWorld.constraintSolver, newWorld.collisionConfiguration);

  if(PhysicsData.visualizationEnabled) {
    newWorld.world->setDebugDrawer(PhysicsData.debugDrawer);
  }

  PhysicsData.worlds.push_back(newWorld);

  return (PhysicsWorldHandle) (PhysicsData.worlds.size() - 1);
}

void
ev_physicsworld_destroyworld(
    PhysicsWorldHandle world_handle)
{
  PhysicsWorld &physWorld = PhysicsData.worlds[world_handle];
  std::lock_guard<std::mutex> guard(physWorld.worldMtx);

  // Clear collision objects
  auto collisionObjects = physWorld.world->getCollisionObjectArray();
  for(int i = collisionObjects.size()-1; i >=0; --i) {
    auto object = collisionObjects[i];
    auto rb = btRigidBody::upcast(object);

    if(rb != nullptr) {
      delete rb->getMotionState();
      delete reinterpret_cast<RigidbodyData*>(rb->getUserPointer());
    }


    physWorld.world->removeCollisionObject(object);
    delete object;
  }

  // Clear collision shapes  
  std::lock_guard<std::mutex> shapeGuard(physWorld.shapeVecMtx);

  for(int i = 0; i < physWorld.collisionShapes.size(); ++i) {
    delete physWorld.collisionShapes[i];
    physWorld.collisionShapes[i] = nullptr;
  }

  delete physWorld.world;
  delete physWorld.constraintSolver;
  delete physWorld.broadphase;
  delete physWorld.collisionDispatcher;
  delete physWorld.collisionConfiguration;
  physWorld.world = nullptr;
  physWorld.constraintSolver = nullptr;
  physWorld.broadphase = nullptr;
  physWorld.collisionDispatcher = nullptr;
  physWorld.collisionConfiguration = nullptr;
}

U32
ev_physicsworld_progress(
    PhysicsWorldHandle world_handle,
    F32 deltaTime)
{
  PhysicsWorld &physWorld = PhysicsData.worlds[world_handle];
  /* ev_log_trace("Progressing PhysicsWorld { %llu } with delta time { %f }", world_handle, deltaTime); */
  std::lock_guard<std::mutex> guard(physWorld.worldMtx);

  physWorld.world->stepSimulation(deltaTime, 10);

  if(PhysicsData.visualizationEnabled && PhysicsData.debugDrawer && !PhysicsData.debugDrawer->windowDestroyed) {
    /* ev_log_trace("Visualization enabled. Drawing frame from PhysicsWorld { %llu }", world_handle); */
    PhysicsData.debugDrawer->startFrame();
    physWorld.world->debugDrawWorld();
    PhysicsData.debugDrawer->endFrame();
  }

  return 0;
}

I32
_ev_physics_init()
{
  // Collision Callbacks
  gContactStartedCallback = contactStartedCallback;
  gContactEndedCallback = contactEndedCallback;

  PhysicsData.game_mod = evol_loadmodule("game");
  if(PhysicsData.game_mod) {
    imports(PhysicsData.game_mod, (Scene));
  }

  return 0;
}

void
_ev_physics_enablevisualization(
    bool enable)
{
  if(enable) {
    PhysicsData.debugDrawer = new BulletDbg();
    for(auto &physWorld : PhysicsData.worlds) {
      physWorld.world->setDebugDrawer(PhysicsData.debugDrawer);
    }
  } else {
    if(PhysicsData.debugDrawer) {
      delete PhysicsData.debugDrawer;
    }
    for(auto &physWorld : PhysicsData.worlds) {
      physWorld.world->setDebugDrawer(nullptr);
    }
  }

  PhysicsData.visualizationEnabled = enable;
}

I32 
_ev_physics_deinit()
{
  if(PhysicsData.visualizationEnabled) {
    delete PhysicsData.debugDrawer;
  }

  if(PhysicsData.game_mod) {
    evol_unloadmodule(PhysicsData.game_mod);
  }

  return 0;
}

#define STORE_COLLISION_SHAPE(pw, x) do { \
    PhysicsWorld &physWorld = PhysicsData.worlds[pw]; \
    std::lock_guard<std::mutex> shapeGuard(physWorld.shapeVecMtx); \
    physWorld.collisionShapes.push_back(x); \
  } while (0)

CollisionShapeHandle
_ev_collisionshape_newbox(
  PhysicsWorldHandle world_handle,
  Vec3 half_extents)
{
  btCollisionShape* box = new btBoxShape(ev2btVec3(half_extents));

  STORE_COLLISION_SHAPE(world_handle, box);

  return box;
}

CollisionShapeHandle
_ev_collisionshape_newsphere(
  PhysicsWorldHandle world_handle,
  F32 radius)
{
  btCollisionShape *sphere = new btSphereShape(radius);

  STORE_COLLISION_SHAPE(world_handle, sphere);

  return sphere;
}

RigidbodyHandle
_ev_rigidbody_new(
  GameScene game_scene,
  U64 entt,
  RigidbodyInfo *rbInfo)
{
  PhysicsWorldHandle world_handle = Scene->getPhysicsWorld(game_scene);
  PhysicsWorld &physWorld = PhysicsData.worlds[world_handle];
  bool isDynamic = rbInfo->type == EV_RIGIDBODY_DYNAMIC && rbInfo->mass > 0.;

  btCollisionShape *collisionShape = reinterpret_cast<btCollisionShape*>(rbInfo->collisionShape);

  btVector3 localInertia;

  if(isDynamic) {
    collisionShape->calculateLocalInertia(rbInfo->mass, localInertia);
  }

  EvMotionState *motionState = new EvMotionState();
  motionState->setGameObject(entt);
  motionState->setGameScene(game_scene);
  btRigidBody::btRigidBodyConstructionInfo btRbInfo(rbInfo->mass, motionState, collisionShape, localInertia);
  btRbInfo.m_restitution = rbInfo->restitution;

  btRigidBody* body = new btRigidBody(btRbInfo);
  RigidbodyData *rbData = new RigidbodyData;
  rbData->entt_id = entt;
  rbData->game_scene = game_scene;
  body->setUserPointer(rbData);

  if(rbInfo->type == EV_RIGIDBODY_KINEMATIC) {
    body->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    body->setActivationState(DISABLE_DEACTIVATION);
  }

  if(isDynamic) {
    body->setActivationState(DISABLE_DEACTIVATION);
  }

  body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

  physWorld.worldMtx.lock();
  physWorld.world->addRigidBody(body);
  physWorld.worldMtx.unlock();

  ev_log_trace("New rigidbody added to PhysicsWorld { %llu }. Current rigidbody count in that world = %llu", world_handle, physWorld.world->getNumCollisionObjects());

  return body;
}

void
_ev_rigidbody_setposition(
    RigidbodyHandle rb,
    Vec3 pos)
{
  btRigidBody* body = reinterpret_cast<btRigidBody *>(rb);
  body->getWorldTransform().setOrigin(ev2btVec3(pos));
}

Vec3
_ev_rigidbody_getposition(
    RigidbodyHandle rb)
{
  btRigidBody* body = reinterpret_cast<btRigidBody *>(rb);
  btVector3 &position = body->getWorldTransform().getOrigin();
  return bt2evVec3(position);
}

void
_ev_rigidbody_setrotationeuler(
    RigidbodyHandle rb,
    Vec3 rot)
{
  btRigidBody* body = reinterpret_cast<btRigidBody *>(rb);
  btQuaternion rot_quat;
  rot_quat.setEuler(rot.y, rot.x, rot.z);
  body->getWorldTransform().setRotation(rot_quat);
}

void
_ev_rigidbody_addforce(
    RigidbodyHandle rb,
    Vec3 f)
{
  btRigidBody* body = reinterpret_cast<btRigidBody *>(rb);
  body->applyCentralForce(ev2btVec3(f));
}

// ==========================
// Custom Collision Callbacks
// ==========================

void 
contactStartedCallback(
    btPersistentManifold* const& manifold)
{
  RigidbodyData *rbData0 = reinterpret_cast<RigidbodyData*>(manifold->getBody0()->getUserPointer());
  RigidbodyData *rbData1 = reinterpret_cast<RigidbodyData*>(manifold->getBody1()->getUserPointer());
  _ev_physics_dispatch_collisionenter(
    rbData0->game_scene,
    static_cast<U64>(rbData0->entt_id),
    static_cast<U64>(rbData1->entt_id));
}

void 
contactEndedCallback(
    btPersistentManifold* const& manifold)
{
  RigidbodyData *rbData0 = reinterpret_cast<RigidbodyData*>(manifold->getBody0()->getUserPointer());
  RigidbodyData *rbData1 = reinterpret_cast<RigidbodyData*>(manifold->getBody1()->getUserPointer());
  _ev_physics_dispatch_collisionleave(
    rbData0->game_scene,
    static_cast<U64>(rbData0->entt_id),
    static_cast<U64>(rbData1->entt_id));
}

