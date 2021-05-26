#include <EvMotionState.h>
#include <evol/common/ev_log.h>

#define bt2evVec3(v) {{  v.x(), v.y(), v.z() }}
#define bt2evQuat(q) {{  q.x(), q.y(), q.z(), q.w() }}

U32 GameModuleRef::refcount = 0;
evolmodule_t GameModuleRef::game_module = nullptr;

EvMotionState::EvMotionState(
    btVector3* graphicsVec,
    const btTransform& startTransform,
    const btTransform& centerOfMassOffset)
{
  mod = GameModuleRef();
}


void EvMotionState::getWorldTransform(btTransform & centerOfMassWorldTrans) const
{
  const Matrix4x4 *objectTransform = Object->getWorldTransform(gameScene, gameObject);
  centerOfMassWorldTrans.setFromOpenGLMatrix(reinterpret_cast<const btScalar*>(*objectTransform));
}

void EvMotionState::setWorldTransform(const btTransform & centerOfMassWorldTrans)
{
  const btVector3& pos = centerOfMassWorldTrans.getOrigin();
  btQuaternion rot = centerOfMassWorldTrans.getRotation();

  Object->setPosition(gameScene, gameObject, bt2evVec3(pos));
  Object->setRotation(gameScene, gameObject, bt2evQuat(rot));
}
