#pragma once
namespace MPL
{
	template <std::size_t I>
	using Size = std::integral_constant<std::size_t, I>;

        // TypeList
    template <typename... Types>
    class TypeList
    {};

        // Front
    template <typename List>
    class Front;

    template <typename Head, typename... Tail>
    class Front<TypeList<Head, Tail...>>
    {
    public:
        using Type = Head;
    };

    template <typename List>
    using Front_t = typename Front<List>::Type;

        // Pop Front
    template <typename List>
    class PopFront;

    template <typename Head, typename... Tail>
    class PopFront<TypeList<Head, Tail...>>
    {
    public:
        using Type = TypeList<Tail...>;
    };

    template <typename List>
    using PopFront_t = typename PopFront<List>::Type;

        // Push Front
    template<typename List, typename NewElement>
    class PushFront;

    template <typename... Elements, typename NewElement>
    class PushFront<TypeList<Elements...>, NewElement>
    {
    public:
        using Type = TypeList<NewElement, Elements...>;
    };

    template <typename List, typename NewElement>
    using PushFront_t = typename PushFront<List, NewElement>::Type;

        // Nth Element
    template <typename List, unsigned N>
    class NthElement : public NthElement<PopFront_t<List>, N-1>
    {};

    template <typename List>
    class NthElement<List, 0> : public Front<List>
    {};

    template <typename List, unsigned N>
    using NthElement_t = typename NthElement<List, N>::Type;

        // Is Empty
    template <typename List>
    class IsEmpty
    {
    public:
        static constexpr bool value = false;
    };

    template <>
    class IsEmpty<TypeList<>>
    {
    public:
        static constexpr bool value = true;
    };

        // Push Back
    template <typename List, typename NewElement>
    class PushBack;

    template <typename... Elements, typename NewElement>
    class PushBack<TypeList<Elements...>, NewElement>
    {
    public:
        using Type = TypeList<Elements..., NewElement>;
    };

    template <typename List, typename NewElement>
    using PushBack_t = typename PushBack<List, NewElement>::Type;

        // Index Of
    template <typename...>
    struct Types
	{
		using Type = Types;
	};

	template <typename SearchElement, typename Types>
	struct IndexOf
	{
	};

	template <typename SearchElement, typename... Elements>
	struct IndexOf<SearchElement, Types<SearchElement, Elements...>> : Size<0>
	{
	};

	template <typename SearchElement, typename Element0, typename... RemainingElements>
	struct IndexOf<SearchElement, Types<Element0, RemainingElements...>> : Size<IndexOf<SearchElement, Types<RemainingElements...>>::value + 1>
	{
	};

	//template <typename SearchElement>
	//struct IndexOf<SearchElement>
	//{
	//	static_assert("Invalid call for IndexOf with no list to index into.");
	//	using value = Size<0>;
	//};

 //   template <typename SearchElement, typename... Elements>
	//struct IndexOf<SearchElement, SearchElement, Elements...> : Size<0>
 //   {};

 //   template <typename SearchElement, typename NotAMatch, typename... Elements>
	//struct IndexOf<SearchElement, NotAMatch, Elements...> :
	//	Size<1 + IndexOf<SearchElement, Elements...>::value> 
	//{};

	template <typename SearchElement, typename... Elements>
	using IndexOf_t = Size<IndexOf<SearchElement, Elements...>::value>;
}