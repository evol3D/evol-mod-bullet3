Rigidbody = 'RigidbodyComponent'

RigidbodyComponent = {}

RigidbodyInvalidHandle = C('ev_rigidbody_getinvalidhandle')

function RigidbodyComponent:new(comp)
  if comp.handle == RigidbodyInvalidHandle then return nil end
  new = comp
  setmetatable(new, self)
  self.__index = self

  return new
end

function RigidbodyComponent:getHandle()
  return self.handle
end

function RigidbodyComponent:setHandle(newHandle)
  self.handle = newHandle
end

function RigidbodyComponent:addForce(forceVec)
  C('ev_rigidbody_addforce', self.handle, forceVec)
end

function RigidbodyComponent:setPosition(new_pos)
  C('ev_rigidbody_setposition', self.handle, new_pos)
end

function RigidbodyComponent:setVelocity(new_vel)
  C('ev_rigidbody_setvelocity', self.handle, new_vel)
end

function RigidbodyComponent:getVelocity()
  local vel = C('ev_rigidbody_getvelocity', self.handle)
  return Vec3:new(vel.x, vel.y, vel.z)
end

function RigidbodyComponent:setRotation(new_rot)
  C('ev_rigidbody_setrotationeuler', self.handle, new_rot)
end

ComponentGetters[Rigidbody] = function(entt)
  return RigidbodyComponent:new(C('ev_rigidbody_getcomponentfromentity', entt))
end

function rayCast(orig, dir, len)
  res = C('ev_physics_raytest', orig, dir, len)

  hitPoint = Vec3:new(res.hitPoint.x, res.hitPoint.y, res.hitPoint.z)
  hitNormal = Vec3:new(res.hitNormal.x, res.hitNormal.y, res.hitNormal.z)
  object = Entities[res.object_id]

  res.hitPoint = hitPoint
  res.hitNormal = hitNormal
  res.object = object

  return res
end
