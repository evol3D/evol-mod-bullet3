#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
  Vec3 hitPoint;
  Vec3 hitNormal;
  U64 object_id;
  bool hasHit;
} RayHit;

I32 
_ev_physics_init();

I32 
_ev_physics_deinit();

U32 
_ev_physics_update(
    F32 deltaTime);

CollisionShapeHandle 
_ev_collisionshape_newcapsule(
    PhysicsWorldHandle world_handle,
    F32 radius,
    F32 height);

CollisionShapeHandle 
_ev_collisionshape_newbox(
    PhysicsWorldHandle world_handle,
    Vec3 half_extents);

CollisionShapeHandle 
_ev_collisionshape_newsphere(
    PhysicsWorldHandle world_handle,
    F32 radius);

RigidbodyHandle 
_ev_rigidbody_new(
    GenericHandle game_scene,
    U64 entt,
    RigidbodyInfo rbInfo);

void
_ev_rigidbody_setposition(
    RigidbodyHandle rb,
    Vec3 pos);

Vec3
_ev_rigidbody_getposition(
    RigidbodyHandle rb);

void
_ev_rigidbody_setrotationeuler(
    RigidbodyHandle rb,
    Vec3 rot);

void
_ev_rigidbody_addforce(
    RigidbodyHandle rb,
    Vec3 f);

void
_ev_physics_dispatch_collisionenter(
    U64 game_scene,
    U64 enttA,
    U64 enttB);

void
_ev_physics_dispatch_collisionleave(
    U64 game_scene,
    U64 enttA,
    U64 enttB);

void
_ev_physics_enablevisualization(
    bool enable);

PhysicsWorldHandle
ev_physicsworld_newworld();

void
ev_physicsworld_destroyworld(
    PhysicsWorldHandle world_handle);

U32
ev_physicsworld_progress(
    PhysicsWorldHandle world_handle,
    F32 deltaTime);

PhysicsWorldHandle
ev_physicsworld_invalidhandle();

RayHit
ev_physics_raytest(
    GameScene scene_handle,
    Vec3 orig,
    Vec3 dir,
    float len);

#if defined(__cplusplus)
}
#endif
