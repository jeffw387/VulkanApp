#pragma once

using Entity = size_t;

namespace ECS
{
    enum ECSBool
    {
        ECSFalse = static_cast<size_t>(0U),
        ECSTrue = static_cast<size_t>(1U)
    };

    class Manager
    {
    public:
        std::vector<Entity> entities;
        std::vector<size_t> isActive;
        std::vector<Entity> inactiveEntities;

        Entity CreateEntity()
        {
            Entity newEntity;
            if (inactiveEntities.size() > 0)
            {
                newEntity = inactiveEntities.back();
                inactiveEntities.pop_back();
                isActive[newEntity] = ECSTrue;
            }
            else
            {
                newEntity = entities.size();
                entities.push_back(newEntity);
                isActive.push_back(1);
            }
            return newEntity;
        }

        void DestroyEntity(Entity entity)
        {
            if (entity < entities.size())
            {
                entities.at(entity) = ECSFalse;
                inactiveEntities.push_back(entity);
            }
        }
    };
}