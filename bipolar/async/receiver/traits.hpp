//! Async traits for `Receiver`
//!
//! # Brief
//!

#ifndef BIPOLAR_ASYNC_RECEIVER_TRAITS_HPP_
#define BIPOLAR_ASYNC_RECEIVER_TRAITS_HPP_

#include "bipolar/async/properties.hpp"

namespace bipolar {
/// @{
/// is_receiver
BIPOLAR_ASYNC_CATEGORY_PROPERTY_DEFINE(Receiver)

template <typename PropertySet>
struct is_receiver
    : property_query<PropertySet, BIPOLAR_ASYNC_PROPERTY(Receiver)> {};

template <typename PropertySet>
inline constexpr bool is_receiver_v = is_receiver<PropertySet>::value;
/// @}

/// @{
/// is_flow
///
/// flow affects both sender and receiver
BIPOLAR_ASYNC_CATEGORY_PROPERTY_DEFINE(Flow)

template <typename PropertySet>
struct is_flow : property_query<PropertySet, BIPOLAR_ASYNC_PROPERTY(Flow)> {};

template <typename PropertySet>
inline constexpr bool is_flow_v = is_flow<PropertySet>::value;
/// @}

} // namespace bipolar

#endif
