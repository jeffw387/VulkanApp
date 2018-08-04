#pragma once
#include "BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.h"
#include "btBulletDynamicsCommon.h"

struct RigidBodyUnique {
  std::unique_ptr<btRigidBody> rigidBody;
  std::unique_ptr<btDefaultMotionState> motionState;
};

RigidBodyUnique CreateRigidBody(btCollisionShape *collisionShape, btScalar mass,
                                btTransform transform) {
  RigidBodyUnique result;

  auto localInertia = btVector3(0, 0, 0);
  collisionShape->calculateLocalInertia(mass, localInertia);

  auto startTransform = btTransform();
  startTransform.setIdentity();
  result.motionState = std::make_unique<btDefaultMotionState>(startTransform);

  auto rigidBodyInfo = btRigidBody::btRigidBodyConstructionInfo(
      mass, result.motionState.get(), collisionShape, localInertia);

  result.rigidBody = std::make_unique<btRigidBody>(rigidBodyInfo);

  return std::move(result);
}

struct CollisionData {
  void init() {
    auto collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    auto collisionDispatcher =
        std::make_unique<btCollisionDispatcher>(collisionConfig.get());
    auto collisionBroadPhase = std::make_unique<btDbvtBroadphase>();
    auto constraintSolver =
        std::make_unique<btSequentialImpulseConstraintSolver>();
    auto simplexSolver = std::make_unique<btVoronoiSimplexSolver>();
    auto penetrationDepthSolver =
        std::make_unique<btMinkowskiPenetrationDepthSolver>();
    auto dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        collisionDispatcher.get(), collisionBroadPhase.get(),
        constraintSolver.get(), collisionConfig.get());

    auto collisionShapes = std::vector<std::unique_ptr<btCollisionShape>>();
    auto rigidBodies = std::vector<btRigidBody>();

    dynamicsWorld->setGravity(btVector3(0, 0, 0));
  }
};