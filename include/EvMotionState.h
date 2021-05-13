#pragma once

#include <btBulletDynamicsCommon.h>
#include <evol/common/ev_types.h>

#define TYPE_MODULE evmod_game
#define NAMESPACE_MODULE evmod_game
#include <evol/meta/type_import.h>
#include <evol/meta/namespace_import.h>

class GameModuleRef
{
  private:
    static evolmodule_t game_module;
    static U32 refcount;

  public:
    GameModuleRef() {
      if(refcount++ == 0) {
        game_module = evol_loadmodule("game");
        IMPORT_NAMESPACE(Object, game_module);
      }
    }

    ~GameModuleRef() {
      if(--refcount == 0) {
        evol_unloadmodule(game_module);
      }
    }
};

ATTRIBUTE_ALIGNED16(struct)
EvMotionState : public btMotionState
{
private:
  ObjectID objectID;
  GameModuleRef mod;

public:
  EvMotionState(
      btVector3* graphicsVec = nullptr,
      const btTransform& startTransform = btTransform::getIdentity(),
      const btTransform& centerOfMassOffset = btTransform::getIdentity());

  void getWorldTransform(btTransform & centerOfMassWorldTrans) const override;
  void setWorldTransform(const btTransform & centerOfMassWorldTrans) override;

  inline void setObjectID(U64 id) {
    objectID = static_cast<ObjectID>(id);
  };
};
