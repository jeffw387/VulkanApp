
namespace MPL
{
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
    class PushFront<<TypeList<Elements...>, NewElement>
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

    
}