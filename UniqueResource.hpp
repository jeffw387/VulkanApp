#pragma once

namespace resource
{
	template <typename TBehavior>
	class unique : TBehavior
	{
	public:
		using behavior_type = TBehavior;
		using handle_type = typename behavior_type::handle_type;

	private:
		handle_type _handle;

		auto& as_behavior() noexcept;
		const auto& as_behavior() const noexcept;

	public:
		unique() noexcept;
		~unique() noexcept;

		unique(const unique&) = delete;
		unique& operator=(const unique&) = delete;

		explicit unique(const handle_type& handle) noexcept;

		unique(unique&& rhs) noexcept;
		auto& operator=(unique&&) noexcept;

		auto release() noexcept;

		void reset() noexcept;
		void reset(const handle_type& handle) noexcept;

		void swap(unique& rhs) noexcept;

		auto get() const noexcept;

		explicit operator bool() const noexcept;

		friend bool operator==(const unique& lhs, const unique& rhs) noexcept;
		friend bool operator!=(const unique& lhs, const unique& rhs) noexcept;
		friend void swap(unique& lhs, unique& rhs) noexcept;
	};

	template <typename TBehavior>
	auto& unique<TBehavior>::as_behavior() noexcept
	{
		return static_cast<behavior_type&>(*this);
	}

	template <typename TBehavior>
	const auto& unique<TBehavior>::as_behavior() const noexcept
	{
		return static_cast<const behavior_type&>(*this);
	}

	template <typename TBehavior>
	unique<TBehavior>::unique() noexcept : _handle{as_behavior().null_handle()}
	{
	}

	template <typename TBehavior>
	unique<TBehavior>::~unique() noexcept
	{
		reset();
	}

	template <typename TBehavior>
	unique<TBehavior>::unique(const handle_type& handle) noexcept
		: _handle{handle}
	{
	}

	template <typename TBehavior>
	unique<TBehavior>::unique(unique&& rhs) noexcept : _handle{rhs.release()}
	{
	}

	template <typename TBehavior>
	auto& unique<TBehavior>::operator=(unique&& rhs) noexcept
	{
		reset(rhs.release());
		return *this;
	}

	template <typename TBehavior>
	auto unique<TBehavior>::release() noexcept
	{
		auto temp_handle(_handle);
		_handle = as_behavior().null_handle();
		return temp_handle;
	}

	template <typename TBehavior>
	void unique<TBehavior>::reset() noexcept
	{
		as_behavior().deinit(_handle);
		_handle = as_behavior().null_handle();
	}

	template <typename TBehavior>
	void unique<TBehavior>::reset(const handle_type& handle) noexcept
	{
		as_behavior().deinit(_handle);
		_handle = handle;
	}

	template <typename TBehavior>
	void unique<TBehavior>::swap(unique& rhs) noexcept
	{
		using std::swap;
		swap(_handle, rhs._handle);
	}

	template <typename TBehavior>
	auto unique<TBehavior>::get() const noexcept
	{
		return _handle;
	}

	template <typename TBehavior>
	unique<TBehavior>::operator bool() const noexcept
	{
		return _handle != as_behavior().null_handle();
	}

	template <typename TBehavior>
	bool operator==(
		const unique<TBehavior>& lhs, const unique<TBehavior>& rhs) noexcept
	{
		return lhs._handle == rhs._handle;
	}

	template <typename TBehavior>
	bool operator!=(
		const unique<TBehavior>& lhs, const unique<TBehavior>& rhs) noexcept
	{
		return !(lhs == rhs);
	}

	template <typename TBehavior>
	void swap(unique<TBehavior>& lhs, unique<TBehavior>& rhs) noexcept
	{
		lhs.swap(rhs);
	}
}