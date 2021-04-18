Rigidbody = 'RigidbodyComponent'

RigidbodyComponent = {}

function RigidbodyComponent:new(comp)
  new = comp or {}
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

ComponentGetters[Rigidbody] = function(entt)
  return RigidbodyComponent:new(C('ev_rigidbody_getcomponentfromentity', entt))
end
