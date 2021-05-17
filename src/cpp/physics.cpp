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

#define ev2btVec3(v) btVector3(v.x, v.y, v.z)
#define bt2evVec3(v) {{  v.x(), v.y(), v.z() }}

struct ev_PhysicsData {
  public:
  btCollisionConfiguration *collisionConfiguration;
  btCollisionDispatcher *collisionDispatcher;
  btBroadphaseInterface *broadphase;
  btSequentialImpulseConstraintSolver *constraintSolver;
  btDynamicsWorld *world;
  BulletDbg *debugDrawer;

  btAlignedObjectArray<btCollisionShape*> collisionShapes;

  std::mutex worldMtx;
  std::mutex shapeVecMtx;

  bool visualizationEnabled;
} PhysicsData;

void 
contactStartedCallback(
    btPersistentManifold* const& manifold);
void 
contactEndedCallback(
    btPersistentManifold* const& manifold);

I32 
_ev_physics_init()
{
  PhysicsData.collisionConfiguration = new btDefaultCollisionConfiguration();
  PhysicsData.collisionDispatcher = new btCollisionDispatcher(PhysicsData.collisionConfiguration);
  PhysicsData.broadphase = new btDbvtBroadphase();
  PhysicsData.constraintSolver = new btSequentialImpulseConstraintSolver();
  PhysicsData.world = new btDiscreteDynamicsWorld(PhysicsData.collisionDispatcher, PhysicsData.broadphase, PhysicsData.constraintSolver, PhysicsData.collisionConfiguration);

  // Collision Callbacks
  gContactStartedCallback = contactStartedCallback;
  gContactEndedCallback = contactEndedCallback;

  return 0;
}

I32
_ev_physics_enablevisualization(
    bool enable)
{
  if(enable) {
    PhysicsData.debugDrawer = new BulletDbg();
    PhysicsData.world->setDebugDrawer(PhysicsData.debugDrawer);
  } else {
    if(PhysicsData.debugDrawer) {
      delete PhysicsData.debugDrawer;
    }
    PhysicsData.world->setDebugDrawer(NULL);
  }

  PhysicsData.visualizationEnabled = enable;
}

void 
clearCollisionObjects()
{
  auto collisionObjects = PhysicsData.world->getCollisionObjectArray();
  for(int i = collisionObjects.size()-1; i >=0; --i) {
    auto object = collisionObjects[i];
    auto rb = btRigidBody::upcast(object);

    if(rb != nullptr) {
      delete rb->getMotionState();
    }

    PhysicsData.world->removeCollisionObject(object);
    delete object;
  }
}

void 
clearCollisionShapes()
{
  std::lock_guard<std::mutex> shapeGuard(PhysicsData.shapeVecMtx);

  for(int i = 0; i < PhysicsData.collisionShapes.size(); ++i) {
    delete PhysicsData.collisionShapes[i];
    PhysicsData.collisionShapes[i] = 0;
  }
}

I32 
_ev_physics_deinit()
{
  std::lock_guard<std::mutex> guard(PhysicsData.worldMtx);

  clearCollisionObjects();
  clearCollisionShapes();

  if(PhysicsData.visualizationEnabled) {
    delete PhysicsData.debugDrawer;
  }

  delete PhysicsData.world;
  delete PhysicsData.constraintSolver;
  delete PhysicsData.broadphase;
  delete PhysicsData.collisionDispatcher;
  delete PhysicsData.collisionConfiguration;

  return 0;
}

U32 
_ev_physics_update(
  F32 deltaTime)
{
  std::lock_guard<std::mutex> guard(PhysicsData.worldMtx);

  PhysicsData.world->stepSimulation(deltaTime, 10);

  if(PhysicsData.visualizationEnabled && PhysicsData.debugDrawer && !PhysicsData.debugDrawer->windowDestroyed) {
    PhysicsData.debugDrawer->startFrame();
    PhysicsData.world->debugDrawWorld();
    PhysicsData.debugDrawer->endFrame();
  }

  return 0;
}

#define STORE_COLLISION_SHAPE(x) do { \
    std::lock_guard<std::mutex> shapeGuard(PhysicsData.shapeVecMtx); \
    PhysicsData.collisionShapes.push_back(x); \
  } while (0)

CollisionShapeHandle
_ev_collisionshape_newbox(
  Vec3 half_extents)
{
  btCollisionShape* box = new btBoxShape(ev2btVec3(half_extents));

  STORE_COLLISION_SHAPE(box);

  return box;
}

CollisionShapeHandle
_ev_collisionshape_newsphere(
  F32 radius)
{
  btCollisionShape *sphere = new btSphereShape(radius);

  STORE_COLLISION_SHAPE(sphere);

  return sphere;
}

RigidbodyHandle
_ev_rigidbody_new(
  U64 entt,
  RigidbodyInfo *rbInfo)
{
  bool isDynamic = rbInfo->type == EV_RIGIDBODY_DYNAMIC && rbInfo->mass > 0.;

  btCollisionShape *collisionShape = reinterpret_cast<btCollisionShape*>(rbInfo->collisionShape);

  btVector3 localInertia;

  if(isDynamic) {
    collisionShape->calculateLocalInertia(rbInfo->mass, localInertia);
  }

  EvMotionState *motionState = new EvMotionState();
  motionState->setObjectID(entt);
  btRigidBody::btRigidBodyConstructionInfo btRbInfo(rbInfo->mass, motionState, collisionShape, localInertia);
  btRbInfo.m_restitution = rbInfo->restitution;

  btRigidBody* body = new btRigidBody(btRbInfo);
  body->setUserPointer(reinterpret_cast<void*>(entt));

  if(rbInfo->type == EV_RIGIDBODY_KINEMATIC) {
    body->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    body->setActivationState(DISABLE_DEACTIVATION);
  }

  if(isDynamic) {
    body->setActivationState(DISABLE_DEACTIVATION);
  }

  body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

  PhysicsData.worldMtx.lock();
  PhysicsData.world->addRigidBody(body);
  PhysicsData.worldMtx.unlock();

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
  _ev_physics_dispatch_collisionenter(
    reinterpret_cast<U64>(manifold->getBody0()->getUserPointer()),
    reinterpret_cast<U64>(manifold->getBody1()->getUserPointer()));
}

void 
contactEndedCallback(
    btPersistentManifold* const& manifold)
{
  _ev_physics_dispatch_collisionleave(
    reinterpret_cast<U64>(manifold->getBody0()->getUserPointer()),
    reinterpret_cast<U64>(manifold->getBody1()->getUserPointer()));
}

