extern "C" {
#define TYPE_MODULE evmod_physics
#include <evol/meta/type_import.h>
#include <evol/common/ev_log.h>
}

#include <physics_api.h>

I32 _ev_physics_init()
{
  ev_log_trace("Physics Init");
  return 0;
}

I32 _ev_physics_deinit()
{
  ev_log_trace("Physics Deinit");
  return 0;
}

U32 _ev_physics_update(F32 deltaTime)
{
  ev_log_info("Physics Update");
  return 0;
}