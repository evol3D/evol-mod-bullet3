#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

I32 _ev_physics_init();
I32 _ev_physics_deinit();
U32 _ev_physics_update(F32 deltaTime);

#if defined(__cplusplus)
}
#endif
