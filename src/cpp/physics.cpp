#include <evol/common/ev_types.h>
#include <evol/common/ev_log.h>

#define TYPE_MODULE evmod_physics
#include <evol/meta/type_import.h>

#include <physics_api.h>

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#include "visual-dbg/BulletDbg.hpp"

#include <mutex>

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

} PhysicsData;

I32 
_ev_physics_init()
{
  PhysicsData.collisionConfiguration = new btDefaultCollisionConfiguration();
  PhysicsData.collisionDispatcher = new btCollisionDispatcher(PhysicsData.collisionConfiguration);
  PhysicsData.broadphase = new btDbvtBroadphase();
  PhysicsData.constraintSolver = new btSequentialImpulseConstraintSolver();
  PhysicsData.world = new btDiscreteDynamicsWorld(PhysicsData.collisionDispatcher, PhysicsData.broadphase, PhysicsData.constraintSolver, PhysicsData.collisionConfiguration);

  // If visualisation enabled
  PhysicsData.debugDrawer = new BulletDbg();
  PhysicsData.world->setDebugDrawer(PhysicsData.debugDrawer);
  return 0;
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

  delete PhysicsData.debugDrawer;

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

  // If visualization enabled
  // This should be a mix of a config var and
  // a compile time directive.
  if(!PhysicsData.debugDrawer->windowDestroyed) {
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
  RigidbodyInfo *rbInfo)
{
  bool isDynamic = rbInfo->type == EV_RIGIDBODY_DYNAMIC && rbInfo->mass > 0.;

  btCollisionShape *collisionShape = reinterpret_cast<btCollisionShape*>(rbInfo->collisionShape);

  btVector3 localInertia(0.0, 0.0, 0.0);
  if(isDynamic) {
    collisionShape->calculateLocalInertia(1.0, localInertia);
  }

  btRigidBody::btRigidBodyConstructionInfo btRbInfo(rbInfo->mass, new btDefaultMotionState(), collisionShape, localInertia);

  btRigidBody* body = new btRigidBody(btRbInfo);
  body->setRestitution(rbInfo->restitution);
  if(rbInfo->type == EV_RIGIDBODY_KINEMATIC) {
    UNIMPLEMENTED();
  }

  if(isDynamic) {
    body->setActivationState(DISABLE_DEACTIVATION);
  }

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
_ev_rigidbody_addforce(
    RigidbodyHandle rb,
    Vec3 f)
{
  btRigidBody* body = reinterpret_cast<btRigidBody *>(rb);
  body->applyCentralForce(ev2btVec3(f));
}
