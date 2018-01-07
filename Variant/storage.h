#ifndef STORAGE_H
#define STORAGE_H
#include <type_traits>
#include <iostream>
#include <utility>
#include <typeinfo>

inline constexpr std::size_t variant_npos = -1;


template <size_t I>
using build_const = std::integral_constant<size_t, I>;

template <typename T0, typename... Ts>
constexpr bool all_copy_constructible = std::is_copy_constructible_v<T0> && std::conjunction_v<std::is_copy_constructible<Ts>...>;

template <typename T0, typename... Ts>
constexpr bool all_move_constructible = std::is_move_constructible_v<T0> && std::conjunction_v<std::is_move_constructible<Ts>...>;

template<bool is_trivially_destructible, typename ... Ts>
struct storage;

template<typename ... Ts>
using storage_t = storage<std::conjunction_v<std::is_trivially_destructible<Ts>...>, Ts...>;

template<bool is_trivially_destructible, typename ... Ts>
struct storage {

    template<typename ...Args>
    void copy_constructor(size_t index, Args&&... args);

    template<typename S>
    void move_constructor(size_t index, S&& other)
    {}

    void reset(size_t index)
    {}
};

template<typename T0, typename ... Ts>
struct storage<1, T0, Ts...>
{
    union
    {
        T0 head;
        storage_t<Ts...> tail;
    };

    constexpr storage() noexcept(std::is_nothrow_constructible_v<T0>)
        : head()
    {}

    template<typename ... Args>
    constexpr storage(build_const<0>, Args&& ... args) noexcept(std::is_nothrow_constructible_v<T0, Args...>)
        : head(std::forward<Args>(args)...)
    {}

    template<size_t I, typename ... Args>
    constexpr storage(build_const<I>, Args&& ... args)
    noexcept(std::is_nothrow_constructible_v<storage_t<Ts...>, build_const<I - 1>, Args...>)
        : tail(build_const<I - 1>{}, std::forward<Args>(args)...)
    {}

    template<typename S>
    void move_constructor(size_t index, S&& other){
        if(index == 0){
            new(&head) T0(std::forward<S>(other).head);
        }
        else {
            tail.move_constructor(index - 1, std::forward<S>(other).tail);
        }
    }

    void reset(size_t index) noexcept
    {
        if (index == 0){
            head.~T0();
        }
        else{
            tail.reset(index - 1);
        }
    }

    template<typename... Args>
    void copy_constructor(size_t index, Args&& ...args){
        if(index == 0){
            new(&head) T0(std::forward<Args>(args)...);
        }
        else {
            tail.copy_constructor(index - 1, std::forward<Args>(args)...);
        }
    }
};

template<typename S>
constexpr decltype(auto) get_storage_data(build_const<0>, S&& s){
    return (std::forward<S>(s).head);
}

template<typename S, size_t I>
constexpr decltype(auto) get_storage_data(build_const<I>, S&& s){
    return get_storage_data(build_const<I - 1>{}, std::forward<S>(s).tail);
}

template<typename ...Ts>
void swap_data(size_t index, storage_t<Ts...>& st1, storage_t<Ts...>& st2){
    if(index == 0){
        std::swap(st1.head, st2.head);
    }
    else {
        swap_data(index - 1, st1.tail, st2.tail);
    }
}

template<>
void swap_data(size_t index, storage_t<>& st1, storage_t<>& st2){
}

template<typename T0, typename ... Ts>
struct storage<0, T0, Ts...>
{
    union
    {
        T0 head;
        storage_t<Ts...> tail;
    };

    constexpr storage() noexcept(std::is_nothrow_constructible_v<T0>)
        : head()
    {}

    template<typename ... Args>
    constexpr storage(build_const<0>, Args&& ... args) noexcept(std::is_nothrow_constructible_v<T0, Args...>)
        : head(std::forward<Args>(args)...)
    {}

    template<size_t I, typename ... Args>
    constexpr storage(build_const<I>, Args&& ... args)
    noexcept(std::is_nothrow_constructible_v<storage_t<Ts...>, build_const<I - 1>, Args...>)
        : tail(build_const<I - 1>{}, std::forward<Args>(args)...)
    {}

    template<typename S>
    void move_constructor(size_t index, S&& other){
        if(index == 0){
            new(&head) T0(std::forward<S>(other).head);
        }
        else {
            tail.move_constructor(index - 1, std::forward<S>(other).tail);
        }
    }

    void reset(size_t index) noexcept
    {
        if (index == 0){
            head.~T0();
        }
        else{
            tail.reset(index - 1);
        }
    }

    template<typename... Args>
    void copy_constructor(size_t index, Args&& ...args){
        if(index == 0){
            new(&head) T0(std::forward<Args>(args)...);
        }
        else {
            tail.copy_constructor(index - 1, std::forward<Args>(args)...);
        }
    }
    ~storage() noexcept {}
};

template<bool is_trivially_destructible, typename... Ts>
struct indexed_destroyable_storage: storage_t<Ts...>
{
    size_t type_id;
    using base = storage_t<Ts...>;
    indexed_destroyable_storage() = default;
//    indexed_destroyable_storage(indexed_destroyable_storage const&) = default;
    template<size_t I, typename ... Args>
    constexpr indexed_destroyable_storage(build_const<I>, Args&& ... args)
    noexcept(std::is_nothrow_constructible_v<base, build_const<I>, Args...>)
        : base(build_const<I>{}, std::forward<Args>(args)...),
          type_id(I)
    {}

    constexpr void set_index(size_t index){
        type_id = index;
    }

    constexpr size_t ind() const noexcept
    {
        return type_id;
    }

    constexpr bool valueless_by_exception_impl() const noexcept
    {
        return ind() == variant_npos;
    }

    constexpr base& get_storage() & noexcept
    {
        return *this;
    }

    constexpr const base& get_storage() const & noexcept
    {
        return *this;
    }

    constexpr base&& get_storage() && noexcept
    {
        return std::move(*this);
    }

    constexpr const base&& get_storage() const && noexcept
    {
        return std::move(*this);
    }
};

template<typename ... Ts>
struct indexed_destroyable_storage<false, Ts...>: indexed_destroyable_storage<true, Ts...>
{
    using indexed_destroyable_storage<true, Ts...>::indexed_destroyable_storage;
    using indexed_destroyable_storage<true, Ts...>::get_storage;
    using indexed_destroyable_storage<true, Ts...>::ind;
    using indexed_destroyable_storage<true, Ts...>::valueless_by_exception_impl;
    using indexed_destroyable_storage<true, Ts...>::set_index;
    using indexed_destroyable_storage<true, Ts...>::base;
    using indexed_destroyable_storage<true, Ts...>::type_id;
    using indexed_destroyable_storage<true, Ts...>::reset;

    ~indexed_destroyable_storage() noexcept{
        if(!valueless_by_exception_impl()){
            reset(type_id);
        }
    }
};


template<typename... Ts>
using indexed_destroyable_storage_t = indexed_destroyable_storage<std::conjunction_v<std::is_trivially_destructible<Ts>...>, Ts...>;

template<bool is_move_constructible, typename...Ts>
struct moveable_storage: indexed_destroyable_storage_t<Ts...>
{
    using moveable_base = indexed_destroyable_storage_t<Ts...>;
    using moveable_base::moveable_base;

    moveable_storage(moveable_storage &&other) {
        moveable_base::type_id = other.type_id;
        if(!moveable_base::valueless_by_exception_impl()){
            moveable_base::move_constructor(moveable_base::type_id, std::move(other));
        }
    }
};

template<typename...Ts>
struct moveable_storage<false, Ts...>: moveable_storage<true, Ts...>
{
    using moveable_base = moveable_storage<true, Ts...>;
    using moveable_base::moveable_base;
    moveable_storage(moveable_storage &&other) = delete;
};

template<typename... Ts>
using moveable_storage_t = moveable_storage<std::conjunction_v<std::is_move_constructible<Ts>...>, Ts...>;

template<bool is_copy_constructible, typename ...Ts>
struct copyable_storage: moveable_storage_t<Ts...>
{
    using copyable_base = moveable_storage_t<Ts...>;
    using copyable_base::copyable_base;

    copyable_storage(const copyable_storage& other) {
        copyable_base::type_id = other.type_id;
        if(!copyable_base::valueless_by_exception_impl()){
            copyable_base::move_constructor(copyable_base::type_id, other);
        }
    }
};

template<typename...Ts>
struct copyable_storage<false, Ts...>: copyable_storage<true, Ts...>
{
    using copyable_base = copyable_storage<true, Ts...>;
    using copyable_base::copyable_base;
    copyable_storage(const copyable_storage &other) = delete;
};

template<typename... Ts>
using copyable_storage_t = copyable_storage<std::conjunction_v<std::is_copy_constructible<Ts>...>, Ts...>;

#endif // STORAGE_H
