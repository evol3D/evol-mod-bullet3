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

CollisionShape 
_ev_physics_createboxshape(
    F32 half_x, 
    F32 half_y, 
    F32 half_z);

CollisionShape 
_ev_physics_createsphereshape(
    F32 radius);

RigidBody 
_ev_physics_createrigidbody(
    RigidBodyInfo *rbInfo);

void
_ev_physics_setrigidbodyposition(
    RigidBody rb,
    Vec3 pos);

#if defined(__cplusplus)
}
#endif
