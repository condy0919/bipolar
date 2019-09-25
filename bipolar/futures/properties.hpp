/// \file properties.hpp
/// \brief Traits about properties
///
/// There are three concepts:
///
/// - \c Property
/// - \c PropertyCategory
/// - \c PropertySet
///
/// A \c PropertySet consists of \c Property s of unique \c PropertyCategory
///

#ifndef BIPOLAR_FUTURES_PROPERTIES_HPP_
#define BIPOLAR_FUTURES_PROPERTIES_HPP_

#include <type_traits>

#include <boost/mp11.hpp>

namespace bipolar {
////////////////////////////////////////////////////////////////////////////////
// property
////////////////////////////////////////////////////////////////////////////////

namespace detail {
/// \internal
/// \brief Gets \c property_category type from \c T
///
/// \endinternal
template <typename T>
using property_category_t = typename T::property_category;
} // namespace detail

/// \struct property
/// \brief The property interface
/// \note All properties should be derived from \c property
///
/// \tparam Category the \c Category type
///
/// ```
/// struct FooCategory {};
/// struct FooProperty : property<FooCategory> {};
///
/// assert(is_property_v<FooProperty>);
/// ```
template <typename Category>
struct property {
    using property_category = Category;
};

/// @{
/// \struct property_traits
/// \brief Extracts the \c PropertyCategory type from \c Property
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
/// \struct is_property
/// \brief Checks whether T is property type
/// \note The check relaxes the property restriction. If \c T looks like a
/// \c property (having property_category type alias inside), it's a \c property
///
/// ```
/// struct FooCategory {};
/// struct FooProperty : property<FooCategory> {};
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
/// \internal
/// \brief Gets \c properties type from \c T
///
/// \endinternal
template <typename T>
using properties_t = typename T::properties;
} // namespace detail

/// \struct property_set
/// \brief a \c Property set consists of properties
/// \note the Properties cannot have the same category
template <typename... Properties>
struct property_set {
    static_assert(std::conjunction_v<is_property<Properties>...>,
                  "property_set only supports Property types");
    static_assert(
        std::is_same_v<
            boost::mp11::mp_unique<
                boost::mp11::mp_list<property_category_t<Properties>...>>,
            boost::mp11::mp_list<property_category_t<Properties>...>>,
        "property_set has multiple properties from the same category");

    using properties = boost::mp11::mp_list<Properties...>;
};

/// @{
/// \struct property_set_traits
/// \brief Extracts the properties type from T
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
/// \struct is_property_set
/// \brief Checks where T is a property_set type
/// \note The check relaxes the property_set restriction. If \c T looks like a
/// \c property_set (having properties type alias inside), it's a property_set
///
/// ```
/// struct FooCategory {};
/// struct FooProperty : property<FooCategory> {};
///
/// using PS = property_set<FooProperty>;
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

namespace detail {
/// \internal
/// \brief The fallback version of contains_derived_property
/// \endinternal
template <typename PropertySet, typename Property, typename = void>
struct contains_derived_property : std::false_type {};

/// \internal
/// \struct contains_derived_property
/// \brief Searches for the \c Property in \c PropertySet
/// With the following conditions satisfied:
/// - Only one \c Property of queried \e PropertySet has the same \c Category
/// with the expected \e Property's
/// - The expected \e Property is derived from the corresponding \c Property
/// which meets the above condition
///
/// \tparam PropertySet the queried \e PropertySet
/// \tparam Property the expected \e Property
///
/// \endinternal
template <typename PropertySet, typename Property>
struct contains_derived_property<
    PropertySet, Property,
    std::void_t<std::is_base_of<
        Property, boost::mp11::mp_at<
                      properties_t<PropertySet>,
                      boost::mp11::mp_find<
                          boost::mp11::mp_transform<property_category_t,
                                                    properties_t<PropertySet>>,
                          property_category_t<Property>>>>>> : std::true_type {
};
} // namespace detail

/// @{
/// \struct property_query
/// \brief Checks whether all properties have the same category with the
/// corresponding one in property_set
///
/// \see contains_derived_property
///
/// ```
/// struct FooCategory {};
/// struct BarCategory {};
///
/// struct FooProperty : property<FooCategory> {};
/// struct BarProperty : property<BarCategory> {};
///
/// // regular cases
/// using PS0 = property_set<FooProperty, BarProperty>;
/// assert((property_query_v<PS0, FooProperty>));
/// assert((property_query_v<PS0, BarProperty>));
///
/// // the expected \e FooProperty is the base case of \e BazProperty
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
/// \internal
/// \brief The fallback version of contains_category
///
/// \endinternal
template <typename PropertySet, typename Category, typename = void>
struct contains_category : std::false_type {};

/// \struct contains_category
/// \brief Checks whether the expected \e Category is in \e PropertySet
///
/// \tparam PropertySet the queried \e PropertySet
/// \tparam Category the expected \e Category
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
/// \brief Checks whether all expected \e Category s existed in \e PropertySet
///
/// \see contains_category
///
/// ```
/// struct FooCategory {};
///
/// struct FooProperty {};
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
