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
    RigidbodyInfo *rbInfo);

void
_ev_rigidbody_setposition(
    RigidbodyHandle rb,
    Vec3 pos);

Vec3
_ev_rigidbody_getposition(
    RigidbodyHandle rb);

void
_ev_rigidbody_addforce(
    RigidbodyHandle rb,
    Vec3 f);

#if defined(__cplusplus)
}
#endif
