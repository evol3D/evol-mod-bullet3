#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

I32 
_ev_physics_init();

I32 
_ev_physics_deinit();

U32 
_ev_physics_update(
    F32 deltaTime);

CollisionShapeHandle 
_ev_collisionshape_newbox(
    Vec3 half_extents);

CollisionShapeHandle 
_ev_collisionshape_newsphere(
    F32 radius);

RigidbodyHandle 
_ev_rigidbody_new(
    U64 entt,
    RigidbodyInfo *rbInfo);

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
    U64 enttA,
    U64 enttB);

void
_ev_physics_dispatch_collisionleave(
    U64 enttA,
    U64 enttB);

void
_ev_physics_enablevisualization(
    bool enable);

#if defined(__cplusplus)
}
#endif
