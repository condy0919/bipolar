//! Properties related traits
//!
//! There are three concepts:
//! - Property
//! - PropertyCategory
//! - PropertySet
//!
//! A `PropertySet` consists of `Property` s of unique `PropertyCategory`

#ifndef BIPOLAR_ASYNC_PROPERTIES_HPP_
#define BIPOLAR_ASYNC_PROPERTIES_HPP_

#include <type_traits>

#include <boost/mp11.hpp>

#define BIPOLAR_ASYNC_CATEGORY(name) name##Category
#define BIPOLAR_ASYNC_CATEGORY_DEFINE(name)                                    \
    struct BIPOLAR_ASYNC_CATEGORY(name) {};

#define BIPOLAR_ASYNC_PROPERTY(name) name##Property
#define BIPOLAR_ASYNC_PROPERTY_DEFINE(name, cat)                               \
    struct BIPOLAR_ASYNC_PROPERTY(name)                                        \
        : Property<BIPOLAR_ASYNC_CATEGORY(cat)> {};

#define BIPOLAR_ASYNC_CATEGORY_PROPERTY_DEFINE(name)                           \
    BIPOLAR_ASYNC_CATEGORY_DEFINE(name)                                        \
    BIPOLAR_ASYNC_PROPERTY_DEFINE(name, name)

namespace bipolar {
////////////////////////////////////////////////////////////////////////////////
// property
////////////////////////////////////////////////////////////////////////////////

namespace detail {
// Gets `property_category` type from `T`
template <typename T>
using property_category_t = typename T::property_category;
} // namespace detail

/// Property
///
/// # Brief
///
/// The property interface
///
/// All properties should be derived from \c property
/// 
/// # Examples
///
/// ```
/// struct FooCategory {};
/// struct FooProperty : Property<FooCategory> {};
///
/// assert(is_property_v<FooProperty>);
/// ```
template <typename Category>
struct Property {
    using property_category = Category;
};

/// @{
/// property_traits
///
/// Extracts the `PropertyCategory` type from a `Property`
template <typename, typename = void>
struct property_traits;

template <typename T>
struct property_traits<
    T, std::void_t<detail::property_category_t<std::decay_t<T>>>> {
    using type = detail::property_category_t<std::decay_t<T>>;
};

template <typename T>
using property_category_t = typename property_traits<T>::type;
/// @}

/// @{
/// is_property
///
/// # Brief
///
/// Checks whether `T` is property type.
///
/// The check relaxes the property restriction. If `T` looks like a
/// `Property` (having `property_category` type alias inside), it's a `Property`
///
/// # Examples
///
/// ```
/// struct FooCategory {};
/// struct FooProperty : Property<FooCategory> {};
/// struct DuckProperty {
///     using property_category = FooCategory;
/// };
///
/// assert(is_property_v<FooProperty>);
/// assert(is_property_v<DuckProperty>);
/// ```
template <typename T, typename = void>
struct is_property : std::false_type {};

template <typename T>
struct is_property<T, std::void_t<property_category_t<T>>> : std::true_type {};

template <typename T>
inline constexpr bool is_property_v = is_property<T>::value;
/// @}

////////////////////////////////////////////////////////////////////////////////
// property_set
////////////////////////////////////////////////////////////////////////////////

namespace detail {
// Gets `properties` type from `T`
template <typename T>
using properties_t = typename T::properties;
} // namespace detail

/// PropertySet
///
/// A `PropertySet` consists of properties which cannot have the same category
template <typename... Properties>
struct PropertySet {
    static_assert(std::conjunction_v<is_property<Properties>...>,
                  "PropertySet only supports Property types");
    static_assert(
        std::is_same_v<
            boost::mp11::mp_unique<
                boost::mp11::mp_list<property_category_t<Properties>...>>,
            boost::mp11::mp_list<property_category_t<Properties>...>>,
        "PropertySet has multiple properties from the same category");

    using properties = boost::mp11::mp_list<Properties...>;
};

/// @{
/// property_set_traits
///
/// Extracts the `properties` type from T
template <typename T, typename = void>
struct property_set_traits;

template <typename T>
struct property_set_traits<T,
                           std::void_t<detail::properties_t<std::decay_t<T>>>> {
    using type = detail::properties_t<std::decay_t<T>>;
};

template <typename T>
using properties_t = typename property_set_traits<T>::type;
/// @}

/// @{
/// is_property_set
///
/// # Brief
///
/// Checks where `T` is a `PropertySet` type.
///
/// The check relaxes the property_set restriction. If `T` looks like a
/// `PropertySet` (having properties type alias inside), it's a `PropertySet`
///
/// # Examples
///
/// ```
/// struct FooCategory {};
/// struct FooProperty : Property<FooCategory> {};
///
/// using PS = PropertySet<FooProperty>;
/// assert(is_property_set_v<PS>);
/// assert(!is_property_set_v<FooProperty>);
/// ```
template <typename, typename = void>
struct is_property_set : std::false_type {};

template <typename T>
struct is_property_set<T, std::void_t<properties_t<T>>> : std::true_type {};

template <typename T>
inline constexpr bool is_property_set_v = is_property_set<T>::value;
/// @}

/// @{
/// property_set_from
///
/// # Brief
///
/// Re-construct `PropertySet` type from a derived type of it
///
/// # Examples
///
/// ```
/// BIPOLAR_ASYNC_CATEGORY_PROPERTY_DEFINE(Foo)
///
/// struct foo : PropertySet<BIPOLAR_ASYNC_PROPERTY(Foo)> {};
///
/// static_assert(std::is_same_v<PropertySet<BIPOLAR_ASYNC_PROPERTY(Foo)>,
///                              property_set_from_t<foo>>);
/// ```
template <typename T>
struct property_set_from {
private:
    static_assert(is_property_set_v<T>, "T must be a PropertySet type");

    template <typename>
    struct impl;

    template <typename... Properties>
    struct impl<boost::mp11::mp_list<Properties...>> {
        using type = PropertySet<Properties...>;
    };

public:
    using type = typename impl<properties_t<T>>::type;
};

template <typename T>
using property_set_from_t = typename property_set_from<T>::type;
/// @}

namespace detail {
template <typename PropertySet, typename Property, typename = void>
struct contains_derived_property : std::false_type {};

// contains_derived_property
//
// Searches for the `Property` in `PropertySet`
// with the following conditions satisfied:
// - Only one `Property` of queried `PropertySet` has the same `Category`
// with the expected `Property`s
// - The expected `Property` is derived from the corresponding `Property`
// which meets the above condition
template <typename PropertySet, typename Property>
struct contains_derived_property<
    PropertySet, Property,
    std::enable_if_t<std::is_base_of_v<
        Property, boost::mp11::mp_at<
                      properties_t<PropertySet>,
                      boost::mp11::mp_find<
                          boost::mp11::mp_transform<property_category_t,
                                                    properties_t<PropertySet>>,
                          property_category_t<Property>>>>>> : std::true_type {
};
} // namespace detail

/// @{
/// property_query
///
/// # Brief
///
/// Checks whether all properties exist in `PropertySet`.
///
/// see `contains_derived_property` for details
///
/// # Examples
///
/// ```
/// struct FooCategory {};
/// struct BarCategory {};
///
/// struct FooProperty : Property<FooCategory> {};
/// struct BarProperty : Property<BarCategory> {};
///
/// // regular cases
/// using PS0 = property_set<FooProperty, BarProperty>;
/// assert((property_query_v<PS0, FooProperty>));
/// assert((property_query_v<PS0, BarProperty>));
///
/// // the expected FooProperty is the base case of BazProperty
/// struct BazProperty : FooProperty {};
/// using PS1 = property_set<BazProperty, BarProperty>;
/// assert((property_query_v<PS1, FooProperty>));
/// ```
template <typename PropertySet, typename... Properties>
struct property_query
    : std::bool_constant<is_property_set_v<PropertySet> &&
                         std::conjunction_v<is_property<Properties>...> &&
                         std::conjunction_v<detail::contains_derived_property<
                             PropertySet, Properties>...>> {};

template <typename PropertySet, typename... Properties>
inline constexpr bool property_query_v =
    property_query<PropertySet, Properties...>::value;
/// @}

namespace detail {
template <typename PropertySet, typename Category, typename = void>
struct contains_category : std::false_type {};

/// contains_category
///
/// Checks whether the expected `Category` is in `PropertySet`
template <typename PropertySet, typename Category>
struct contains_category<
    PropertySet, Category,
    std::void_t<boost::mp11::mp_at<
        properties_t<PropertySet>,
        boost::mp11::mp_find<
            boost::mp11::mp_transform<property_category_t,
                                      properties_t<PropertySet>>,
            Category>>>> : std::true_type {};
} // namespace detail

/// @{
/// category_query
///
/// # Brief
///
/// Checks whether all expected `Category` s existed in `PropertySet`
///
/// see `contains_category` for details
///
/// # Examples
///
/// ```
/// struct FooCategory {};
///
/// struct FooProperty : Property<FooCategory> {};
///
/// using PS = property_set<FooProperty>;
/// assert(!(category_query_v<PS, int>));
/// assert(!(category_query_v<PS, FooProperty>));
/// assert((category_query_v<PS, FooCategory>));
/// ```
template <typename PropertySet, typename... Categories>
struct category_query
    : std::bool_constant<
          is_property_set_v<PropertySet> &&
          std::conjunction_v<std::negation<is_property<Categories>>...> &&
          std::conjunction_v<
              detail::contains_category<PropertySet, Categories>...>> {};

template <typename PropertySet, typename... Categories>
inline constexpr bool category_query_v =
    category_query<PropertySet, Categories...>::value;
/// @}

} // namespace bipolar

#endif
