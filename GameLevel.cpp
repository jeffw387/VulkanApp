#include <vector>

using Health = float;
using Mass = float;
using EntityID = uint64_t;

struct Vec2
{
	float x;
	float y;

	// named rhs operators
		// vec2 rhs
	Vec2 operator +(const Vec2& rhs)
	{
		Vec2 result;
		result.x = this->x + rhs.x;
		result.y = this->y + rhs.y;
		return result;
	}

	Vec2& operator +=(const Vec2& rhs)
	{
		this->x += rhs.x;
		this->y += rhs.y;
		return *this;
	}

	Vec2 operator -(const Vec2& rhs)
	{
		Vec2 result;
		result.x = this->x - rhs.x;
		result.y = this->y - rhs.y;
		return result;
	}

	Vec2& operator -=(const Vec2& rhs)
	{
		this->x -= rhs.x;
		this->y -= rhs.y;
		return *this;
	}

		// float rhs
	Vec2 operator *(const float& rhs)
	{
		Vec2 result;
		result.x = this->x * rhs;
		result.y = this->y * rhs;
		return result;
	}

	Vec2& operator /(const float& rhs)
	{
		return *this * (1.0f / rhs);
	}

	// unnamed rhs operators
};

using Position = Vec2;
using Velocity = Vec2;

struct PhysicsProperties
{
	Position position;
	Velocity velocity;
	Mass mass;
};

class GameLevel
{
public:
	void Init()
	{
		m_MaxEntities = 1000;
		m_CurrentEntityCount = 0;
		m_PhysicsProperties.resize(m_MaxEntities);
		m_Health.resize(m_MaxEntities);
	}

	void Update(float timeDelta)
	{
		for (EntityID i = 0; i < m_CurrentEntityCount; i++)
		{
			m_PhysicsProperties[i].position += (m_PhysicsProperties[i].velocity * timeDelta);
		}
	}

	void AccelerateEntity(EntityID id, Velocity accelerationVector, float timeDelta)
	{
		if (id < m_CurrentEntityCount)
		{
			m_PhysicsProperties[id].velocity += (accelerationVector / m_PhysicsProperties[id].mass * timeDelta);
		}
	}

	void AddEntity(Position position, Mass mass, Velocity velocity, Health health)
	{
		if (m_CurrentEntityCount < m_MaxEntities)
		{
			m_PhysicsProperties[m_CurrentEntityCount].position = position;
			m_PhysicsProperties[m_CurrentEntityCount].mass = mass;
			m_PhysicsProperties[m_CurrentEntityCount].velocity = velocity;
			m_Health[m_CurrentEntityCount] = health;
			m_CurrentEntityCount++;
		}
	}

private:
	std::vector<PhysicsProperties> m_PhysicsProperties;
	std::vector<Health> m_Health;
	uint64_t m_MaxEntities;
	uint64_t m_CurrentEntityCount;
};