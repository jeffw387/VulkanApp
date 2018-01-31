#pragma once
#include <optional>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

class Camera2D
{
public:
	const glm::vec2& getPosition() { return m_Position; }
	void setPosition(glm::vec2&& position)
	{
		m_Position = position;
		m_ViewProjectionMat.reset();
	}

	const glm::vec2& getSize() { return m_Size; }
	void setSize(glm::vec2&& size)
	{
		m_Size = size;
		m_ViewProjectionMat.reset();
	}

	const float& getRotation() { return m_Rotation; }
	void setRotation(float&& rotation) 
	{ 
		m_Rotation = rotation;
		m_ViewProjectionMat.reset();
	}

	const glm::mat4& getMatrix()
	{
		if (m_ViewProjectionMat.has_value() == false)
		{
			auto halfWidth = m_Size.x * 0.5f;
			auto halfHeight = m_Size.y * 0.5f;
			auto left = -halfWidth + m_Position.x;
			auto right = halfWidth + m_Position.x;
			auto bottom = halfHeight + m_Position.y;
			auto top = -halfHeight + m_Position.y;

			m_ViewProjectionMat = glm::ortho(left, right, top, bottom, 0.0f, 1.0f);
		}
		return m_ViewProjectionMat.value();
	}

private:
	glm::vec2 m_Position = {0.f, 0.f};
	glm::vec2 m_Size = {0.f, 0.f};
	float m_Rotation = 0.f;
	std::optional<glm::mat4> m_ViewProjectionMat;
};