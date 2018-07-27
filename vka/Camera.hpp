#pragma once
#include <optional>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace vka
{
	class Camera2D
	{
	public:
		const glm::vec2& getPosition() { return m_Position; }
		void setPosition(glm::vec2&& position)
		{
			m_Position = position;
			m_View.reset();
		}

		const glm::vec2& getSize() { return m_Size; }
		void setSize(glm::vec2&& size)
		{
			m_Size = size;
			m_Projection.reset();
		}

		const float& getRotation() { return m_Rotation; }
		void setRotation(float&& rotation) 
		{ 
			m_Rotation = rotation;
			m_View.reset();
		}

		const glm::mat4& getViewMatrix()
		{
			if (!m_View.has_value())
			{
				m_View = glm::translate(glm::mat4(1.f), { m_Position.x, m_Position.y, 0.f });
			}
			return m_View.value();
		}

		const glm::mat4& getProjectionMatrix()
		{
			if (!m_Projection.has_value())
			{
				auto halfWidth = m_Size.x * 0.5f;
				auto halfHeight = m_Size.y * 0.5f;
				auto left = -halfWidth;
				auto right = halfWidth;
				auto bottom = halfHeight;
				auto top = -halfHeight;

				m_Projection = glm::ortho(left, right, top, bottom, 0.0f, 1.0f);
			}
			return m_Projection.value();
		}

	private:
		glm::vec2 m_Position = {0.f, 0.f};
		glm::vec2 m_Size = {0.f, 0.f};
		float m_Rotation = 0.f;
		std::optional<glm::mat4> m_View;
		std::optional<glm::mat4> m_Projection;
	};
}