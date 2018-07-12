
#define STB_IMAGE_IMPLEMENTATION
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#include <variant>
#include <memory>

#include "vka/VulkanApp.hpp"
#include "TimeHelper.hpp"
#include "ECSComponents.hpp"
#include "Models.hpp"
#include "entt/entt.hpp"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.h"
#include "vka/GLTF.hpp"

#undef min
#undef max

#ifndef CONTENTROOT
#define CONTENTROOT
#endif

//#include "SpriteData.hpp"
const char *ConfigPath = CONTENTROOT "config/VulkanInitInfo.json";

namespace Fonts
{
	constexpr auto AeroviasBrasil = entt::HashedString(CONTENTROOT "Content/Fonts/AeroviasBrasilNF.ttf");
}

class ClientApp : public vka::VulkanApp
{
public:
	entt::DefaultRegistry enttRegistry;
	vka::Model cubeModel;
	vka::Model cylinderModel;
	vka::Model icosphereSub2Model;
	vka::Model pentagonModel;
	vka::Model triangleModel;

	void LoadModels()
	{
		LoadModelFromFile(Models::Path, Models::Cube::file);
		LoadModelFromFile(Models::Path, Models::Cylinder::file);
		LoadModelFromFile(Models::Path, Models::IcosphereSub2::file);
		LoadModelFromFile(Models::Path, Models::Pentagon::file);
		LoadModelFromFile(Models::Path, Models::Triangle::file);
	}

	void LoadImages()
	{
		//auto sheet1 = vka::loadImageFromFile(std::string(Sprites::SpriteSheet1::ImagePath));
		//CreateImage2D(Sprites::SpriteSheet1::ImagePath, sheet1);
		//CreateSprite(Sprites::SpriteSheet1::ImagePath,
		//             Sprites::SpriteSheet1::starpng::Name,
		//             Sprites::SpriteSheet1::starpng::SpriteQuad);
	}

	void Update(TimePoint_ms updateTime)
	{
		bool inputToProcess = true;
		while (inputToProcess)
		{
			auto messageOptional = inputBuffer.popFirstIf(
				[updateTime](vka::InputMessage message) { return message.time < updateTime; });

			if (messageOptional.has_value())
			{
				auto &message = *messageOptional;
				auto bindHash = inputBindMap[message.signature];
				auto stateVariant = inputStateMap[bindHash];
				vka::StateVisitor visitor;
				visitor.signature = message.signature;
				std::visit(visitor, stateVariant);
			}
			else
			{
				inputToProcess = false;
			}
		}

		//auto physicsView = enttRegistry.persistent<cmp::Engine, cmp::Velocity, cmp::Position, cmp::Transform>();
	}

	void Draw()
	{
		// 2D rendering
		//auto view = enttRegistry.view<cmp::Sprite, cmp::Transform, cmp::Color>(entt::persistent_t{});
  //      for (auto entity : view)
  //      {
  //          auto &sprite = view.get<cmp::Sprite>(entity);
  //          auto &transform = view.get<cmp::Transform>(entity);
  //          auto &color = view.get<cmp::Color>(entity);
  //          if (transform.matrix.has_value())
  //          {
  //              RenderSpriteInstance(sprite.index, transform.matrix.value(), color.rgba);
  //          }
  //      }

		// 3D rendering
		auto cubeView = enttRegistry.view<
			cmp::Transform,
			cmp::Color,
			Models::Cube>(entt::persistent_t{});
		BeginRender(5);
		RenderModelInstances<cmp::Transform, cmp::Color, Models::Cube>(cubeView);
	}
};

struct RigidBodyUnique
{
	std::unique_ptr<btRigidBody> rigidBody;
	std::unique_ptr<btDefaultMotionState> motionState;
};

RigidBodyUnique CreateRigidBody(
	btCollisionShape *collisionShape,
	btScalar mass,
	btTransform transform)
{
	RigidBodyUnique result;

	auto localInertia = btVector3(0, 0, 0);
	collisionShape->calculateLocalInertia(mass, localInertia);

	auto startTransform = btTransform();
	startTransform.setIdentity();
	result.motionState = std::make_unique<btDefaultMotionState>(startTransform);

	auto rigidBodyInfo = btRigidBody::btRigidBodyConstructionInfo(mass, result.motionState.get(), collisionShape, localInertia);

	result.rigidBody = std::make_unique<btRigidBody>(rigidBodyInfo);

	return std::move(result);
}

int main()
{
	ClientApp app;
	auto collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
	auto collisionDispatcher = std::make_unique<btCollisionDispatcher>(collisionConfig.get());
	auto collisionBroadPhase = std::make_unique<btDbvtBroadphase>();
	auto constraintSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
	auto simplexSolver = std::make_unique<btVoronoiSimplexSolver>();
	auto penetrationDepthSolver = std::make_unique<btMinkowskiPenetrationDepthSolver>();
	auto dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
		collisionDispatcher.get(),
		collisionBroadPhase.get(),
		constraintSolver.get(),
		collisionConfig.get());

	auto collisionShapes = std::vector<std::unique_ptr<btCollisionShape>>();
	auto rigidBodies = std::vector<btRigidBody>();

	dynamicsWorld->setGravity(btVector3(0, 0, 0));

	app.inputBindMap[vka::MakeSignature(GLFW_KEY_LEFT, GLFW_PRESS)] = Bindings::Left;
	app.inputBindMap[vka::MakeSignature(GLFW_KEY_RIGHT, GLFW_PRESS)] = Bindings::Right;
	app.inputBindMap[vka::MakeSignature(GLFW_KEY_UP, GLFW_PRESS)] = Bindings::Up;
	app.inputBindMap[vka::MakeSignature(GLFW_KEY_DOWN, GLFW_PRESS)] = Bindings::Down;

	app.inputStateMap[Bindings::Left] = vka::MakeAction([]() { std::cout << "Left Pressed.\n"; });
	app.inputStateMap[Bindings::Right] = vka::MakeAction([]() { std::cout << "Right Pressed.\n"; });
	app.inputStateMap[Bindings::Up] = vka::MakeAction([]() { std::cout << "Up Pressed.\n"; });
	app.inputStateMap[Bindings::Down] = vka::MakeAction([]() { std::cout << "Down Pressed.\n"; });

	// render view
	app.enttRegistry.prepare<cmp::Sprite, cmp::Transform, cmp::Color>();
	// physics view
	//app.enttRegistry.prepare<cmp::Engine, cmp::Velocity, cmp::Position, cmp::Transform>();

	// 3D render views
	app.enttRegistry.prepare<cmp::Transform, cmp::Color, Models::Cube>();
	app.enttRegistry.prepare<cmp::Transform, cmp::Color, Models::Cylinder>();
	app.enttRegistry.prepare<cmp::Transform, cmp::Color, Models::IcosphereSub2>();
	app.enttRegistry.prepare<cmp::Transform, cmp::Color, Models::Pentagon>();
	app.enttRegistry.prepare<cmp::Transform, cmp::Color, Models::Triangle>();

	auto starPrototype = entt::DefaultPrototype(app.enttRegistry);
	starPrototype.set<cmp::Sprite>();
	starPrototype.set<cmp::Transform>(glm::mat4(1.f));
	starPrototype.set<cmp::Color>(glm::vec4(1.f));
	// 2D entities
	for (auto i = 0.f; i < 3.f; ++i)
	{
		auto entity = starPrototype();
	}

	// 3D entities
	auto lightPrototype = entt::DefaultPrototype(app.enttRegistry);
	lightPrototype.set<cmp::Light>();
	lightPrototype.set<cmp::Color>(glm::vec4(1.f));
	lightPrototype.set<cmp::Transform>(glm::mat4(1.f));

	auto light0 = lightPrototype();
	app.enttRegistry.get<cmp::Transform>(light0) = glm::translate(glm::mat4(1.f), glm::vec3(50.f, 50.f, -10.f));

	auto trianglePrototype = entt::DefaultPrototype(app.enttRegistry);
	trianglePrototype.set<cmp::Transform>();
	trianglePrototype.set<Models::Triangle>();
	trianglePrototype.set<cmp::Color>(glm::vec4(1.f));

	for (auto i = 0.f; i < 3.f; ++i)
	{
		auto entity = trianglePrototype();
		app.enttRegistry.get<cmp::Transform>(entity) = glm::translate(glm::mat4(1.f), glm::vec3(i * 100.f));
	}

	try
	{
		app.Run(std::string(ConfigPath));
	}
	catch (const std::exception& e)
	{
		std::cout << e.what();
	}
}