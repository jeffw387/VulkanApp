#pragma once
#include "entt/entt.hpp"
#include "ECSComponents.hpp"
#include "Models.hpp"

struct {
	using proto = entt::DefaultPrototype;
	proto player;
	proto triangle;
	proto cube;
	proto cylinder;
	proto icosphereSub2;
	proto pentagon;
	proto light;

	void init(entt::DefaultRegistry& registry)
	{
		player = proto(registry);
		triangle = proto(registry);
		cube = proto(registry);
		cylinder = proto(registry);
		icosphereSub2 = proto(registry);
		pentagon = proto(registry);
		light = proto(registry);

		triangle.set<Models::Triangle>();
		triangle.set<cmp::Transform>(glm::mat4(1.f));
		triangle.set<cmp::Color>(glm::vec4(1.f));

		cube.set<Models::Cube>();
		cube.set<cmp::Transform>(glm::mat4(1.f));
		cube.set<cmp::Color>(glm::vec4(1.f));

		cylinder.set<Models::Cylinder>();
		cylinder.set<cmp::Transform>(glm::mat4(1.f));
		cylinder.set<cmp::Color>(glm::vec4(1.f));

		icosphereSub2.set<Models::IcosphereSub2>();
		icosphereSub2.set<cmp::Transform>(glm::mat4(1.f));
		icosphereSub2.set<cmp::Color>(glm::vec4(1.f));

		pentagon.set<Models::Pentagon>();
		pentagon.set<cmp::Transform>(glm::mat4(1.f));
		pentagon.set<cmp::Color>(glm::vec4(1.f));

		light.set<cmp::Light>();
		light.set<cmp::Color>(glm::vec4(1.f));
		light.set<cmp::Transform>(glm::mat4(1.f));
	}
} prototypes;