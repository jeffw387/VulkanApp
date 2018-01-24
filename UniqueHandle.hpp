#pragma once
#include <type_traits>
#include <iostream>

template <typename HandleBehavior>
class UniqueHandle : HandleBehavior
{
public:
	using HandleType = HandleBehavior::template HandleType;

	// Default Constructor
	UniqueHandle() noexcept :
		m_Handle(HandleBehavior::NullHandleValue())
	{
	}

	// Primary constructor
	UniqueHandle(HandleType handle) noexcept :
		m_Handle(handle)
	{
		std::cout << "UniqueHandle constructed.\n";
	}

	// Move capabilities
	UniqueHandle(UniqueHandle&& handle) noexcept :
		m_Handle(std::move(handle))
	{
		std::cout << "UniqueHandle move constructed.\n";
	}
	UniqueHandle(HandleType&& handle) noexcept :
		m_Handle(std::move(handle))
	{
		std::cout << "UniqueHandle move constructed from HandleType&&.\n";
	}
	UniqueHandle& operator=(UniqueHandle&&) noexcept = default;
	UniqueHandle& operator=(HandleType&& handle) noexcept
	{
		std::cout << "UniqueHandle move assigned from HandleType&&.\n";
		this->m_Handle = std::move(handle);
		return *this;
	}

	// Disable copying
	UniqueHandle(const UniqueHandle&) = delete;
	UniqueHandle& operator=(const UniqueHandle&) = delete;

	// Handle access
	HandleType get() noexcept
	{
		return m_Handle;
	}

	HandleType operator->() noexcept
	{
		return m_Handle;
	}

	HandleType operator*() noexcept
	{
		return m_Handle;
	}

	const HandleType* operator&() noexcept
	{
		return &m_Handle;
	}

	HandleType&& release() noexcept
	{
		std::cout << "UniqueHandle is released.\n";
		auto result = HandleType();
		std::swap(m_Handle, result);
		return std::move(result);
	}

	void reset(HandleType handle = HandleType()) noexcept
	{
		std::cout << "UniqueHandle is reset with value " << handle << ".\n";
		auto oldHandle = m_Handle;
		m_Handle = handle;
		destroy(oldHandle);
	}

	~UniqueHandle() noexcept
	{
		std::cout << "UniqueHandle destructed.\n";
		destroy(m_Handle);
	}

private:
	HandleType m_Handle = HandleBehavior::NullHandleValue();

	void destroy(HandleType& handle) noexcept
	{
		if (handle != HandleBehavior::NullHandleValue())
		{
			HandleBehavior::destruct(handle);
			handle = HandleBehavior::NullHandleValue();
		}
	}
};