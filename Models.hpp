#pragma once

namespace Models
{
	const char* Path = "Content/Blender/";
	struct Cylinder 
	{
		static constexpr auto file = entt::HashedString("cylinder.gltf");

		constexpr operator const entt::HashedString::hash_type() const
		{
			return file;
		}
	};
	struct Cube 
	{
		static constexpr auto file = entt::HashedString("cube.gltf");

		constexpr operator const entt::HashedString::hash_type() const
		{
			return file;
		}
	};
	struct Triangle 
	{
		static constexpr auto file = entt::HashedString("triangle.gltf");

		constexpr operator const entt::HashedString::hash_type() const
		{
			return file;
		}
	};
	struct IcosphereSub2 
	{
		static constexpr auto file = entt::HashedString("icosphereSub2.gltf");

		constexpr operator const entt::HashedString::hash_type() const
		{
			return file;
		}
	};
	struct Pentagon 
	{
		static constexpr auto file = entt::HashedString("pentagon.gltf");

		constexpr operator const entt::HashedString::hash_type() const
		{
			return file;
		}
	};
}