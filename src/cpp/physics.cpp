#include <evol/common/ev_types.h>
#include <evol/common/ev_log.h>

#define INVALID_WORLD_HANDLE (~0ull)

#define TYPE_MODULE evmod_physics
#include <evol/meta/type_import.h>

#define IMPORT_MODULE evmod_assets
#include <evol/meta/module_import.h>

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#include "visual-dbg/BulletDbg.hpp"


#include <mutex>

#include <EvMotionState.h>

#include <physics_api.h>

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
  evolmodule_t asset_mod;

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

PhysicsWorldHandle
ev_physicsworld_invalidhandle()
{
  return INVALID_WORLD_HANDLE;
}

void
ev_physicsworld_destroyworld(
    PhysicsWorldHandle world_handle)
{
  if(world_handle == INVALID_WORLD_HANDLE) {
    return;
  }
  PhysicsWorld &physWorld = PhysicsData.worlds[world_handle];
  if(physWorld.world == nullptr) {
    return;
  }

  std::lock_guard<std::mutex> guard(physWorld.worldMtx);

  // Clear collision objects
  auto collisionObjects = physWorld.world->getCollisionObjectArray();
  for(int i = collisionObjects.size()-1; i >=0; --i) {
    auto object = collisionObjects[i];
    auto rb = btRigidBody::upcast(object);

    if(rb != nullptr) {
      delete rb->getMotionState();
      delete reinterpret_cast<RigidbodyData*>(rb->getUserPointer());
      rb->setUserPointer(nullptr);
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

  PhysicsData.game_mod = evol_loadmodule_weak("game");
  if(PhysicsData.game_mod) {
    imports(PhysicsData.game_mod, (Scene));
  }

  PhysicsData.asset_mod = evol_loadmodule_weak("assetmanager");
  if(PhysicsData.asset_mod) {
    imports(PhysicsData.asset_mod, (Asset, MeshLoader));
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


  return 0;
}

#define STORE_COLLISION_SHAPE(pw, x) do { \
    PhysicsWorld &physWorld = PhysicsData.worlds[pw]; \
    std::lock_guard<std::mutex> shapeGuard(physWorld.shapeVecMtx); \
    physWorld.collisionShapes.push_back(x); \
  } while (0)

CollisionShapeHandle
_ev_collisionshape_newcapsule(
  PhysicsWorldHandle world_handle,
  F32 radius,
  F32 height)
{
  btCollisionShape* capsule = new btCapsuleShape(radius, height);

  STORE_COLLISION_SHAPE(world_handle, capsule);

  return capsule;
}

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
_ev_collisionshape_newmesh(
    PhysicsWorldHandle world_handle,
    CONST_STR mesh_path)
{
  AssetHandle mesh_handle = Asset->load(mesh_path);
  MeshAsset meshAsset = MeshLoader->loadAsset(mesh_handle);

  btStridingMeshInterface* buffer_interface = new btTriangleIndexVertexArray(
      meshAsset.indexCount / 3,
      reinterpret_cast<int32_t*>(meshAsset.indexData),
      meshAsset.indexBuferSize / meshAsset.indexCount * 3,
      meshAsset.vertexCount,
      meshAsset.vertexData,
      meshAsset.vertexBuferSize / meshAsset.vertexCount);

  btCollisionShape* mesh = new btBvhTriangleMeshShape(buffer_interface, true);
  STORE_COLLISION_SHAPE(world_handle, mesh);

  Asset->free(mesh_handle);

  return mesh;
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
  RigidbodyInfo rbInfo)
{
  PhysicsWorldHandle world_handle = Scene->getPhysicsWorld(game_scene);
  PhysicsWorld &physWorld = PhysicsData.worlds[world_handle];
  bool isDynamic = rbInfo.type == EV_RIGIDBODY_DYNAMIC && rbInfo.mass > 0.;
  bool isGhost = rbInfo.type == EV_RIGIDBODY_GHOST;

  btCollisionShape *collisionShape = reinterpret_cast<btCollisionShape*>(rbInfo.collisionShape);

  btVector3 localInertia(0.0, 0.0, 0.0);

  if(isDynamic) {
    collisionShape->calculateLocalInertia(rbInfo.mass, localInertia);
  }

  EvMotionState *motionState = new EvMotionState();
  motionState->setGameObject(entt);
  motionState->setGameScene(game_scene);
  btRigidBody::btRigidBodyConstructionInfo btRbInfo(rbInfo.mass, motionState, collisionShape, localInertia);
  btRbInfo.m_restitution = rbInfo.restitution;

  btRigidBody* body = new btRigidBody(btRbInfo);
  RigidbodyData *rbData = new RigidbodyData;
  rbData->entt_id = entt;
  rbData->game_scene = game_scene;
  body->setUserPointer(rbData);

  if(rbInfo.type == EV_RIGIDBODY_KINEMATIC) {
    body->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    body->setActivationState(DISABLE_DEACTIVATION);
  }

  if(isGhost) {
    body->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
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

RayHit
ev_physics_raytest(
    GameScene scene_handle,
    Vec3 orig,
    Vec3 dir,
    float len)
{
  PhysicsWorldHandle world_handle = Scene->getPhysicsWorld(scene_handle);
  PhysicsWorld &physWorld = PhysicsData.worlds[world_handle];

  btVector3 from = ev2btVec3(orig);
  btVector3 to = ev2btVec3(dir) * len;
  btCollisionWorld::ClosestRayResultCallback rayResult(from, to);
  physWorld.world->rayTest(from, to, rayResult);

  RayHit hit;
  hit.hasHit = rayResult.hasHit();
  hit.hitPoint = bt2evVec3(rayResult.m_hitPointWorld);
  hit.hitNormal = bt2evVec3(rayResult.m_hitNormalWorld);
  if(hit.hasHit) {
    hit.object_id = reinterpret_cast<RigidbodyData*>(rayResult.m_collisionObject->getUserPointer())->entt_id;
  } else {
    hit.object_id = 0;
  }

  return hit;
}

void
_ev_rigidbody_setposition(
    RigidbodyHandle rb,
    Vec3 pos)
{
  btRigidBody* body = reinterpret_cast<btRigidBody *>(rb);
  body->getWorldTransform().setOrigin(ev2btVec3(pos));
}

void
_ev_rigidbody_setvelocity(
    RigidbodyHandle rb,
    Vec3 vel)
{
  btRigidBody* body = reinterpret_cast<btRigidBody *>(rb);
  body->setLinearVelocity(ev2btVec3(vel));
}


Vec3
_ev_rigidbody_getposition(
    RigidbodyHandle rb)
{
  btRigidBody* body = reinterpret_cast<btRigidBody *>(rb);
  btVector3 &position = body->getWorldTransform().getOrigin();
  return bt2evVec3(position);
}

Vec3
_ev_rigidbody_getvelocity(
    RigidbodyHandle rb)
{
  btRigidBody* body = reinterpret_cast<btRigidBody *>(rb);
  const btVector3 &velocity = body->getLinearVelocity();
  return bt2evVec3(velocity);
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

void
_ev_rigidbody_destroy(
  GameScene game_scene,
  RigidbodyHandle rb)
{
  PhysicsWorldHandle world_handle = Scene->getPhysicsWorld(game_scene);
  PhysicsWorld &physWorld = PhysicsData.worlds[world_handle];
  btRigidBody* body = reinterpret_cast<btRigidBody *>(rb);
  if(body != nullptr) {
    delete body->getMotionState();
    delete reinterpret_cast<RigidbodyData*>(body->getUserPointer());
    body->setUserPointer(nullptr);
  }

  physWorld.world->removeRigidBody(body);
  delete body;
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
  if(rbData0 == nullptr || rbData1 == nullptr) {
    return;
  }
  _ev_physics_dispatch_collisionleave(
    rbData0->game_scene,
    static_cast<U64>(rbData0->entt_id),
    static_cast<U64>(rbData1->entt_id));
}

