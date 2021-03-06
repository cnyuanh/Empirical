/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2016-2017
 *
 *  @file  map_utils.h
 *  @brief A set of simple functions to manipulate maps.
 *  @note Status: BETA
 */

#ifndef EMP_MAP_UTILS_H
#define EMP_MAP_UTILS_H

#include <map>
#include <unordered_map>

namespace emp {

  /// Take any map type, and run find to determine if a key is present.
  template <class MAP_T, class KEY_T>
  inline bool Has( const MAP_T & in_map, const KEY_T & key ) {
    return in_map.find(key) != in_map.end();
  }


  /// Take any map, run find() member function, and return the result found
  /// (or default value if no results found).
  template <class MAP_T, class KEY_T>
  inline auto Find( const MAP_T & in_map, const KEY_T & key, const typename MAP_T::mapped_type & dval) {
    auto val_it = in_map.find(key);
    if (val_it == in_map.end()) return dval;
    return val_it->second;
  }


  /// Take any map and element, run find() member function, and return a reference to
  /// the result found (or default value if no results found).
  template <class MAP_T, class KEY_T>
  inline const auto & FindRef( const MAP_T & in_map, const KEY_T & key, const typename MAP_T::mapped_type & dval) {
    auto val_it = in_map.find(key);
    if (val_it == in_map.end()) return dval;
    return val_it->second;
  }


  // The following two functions are from:
  // http://stackoverflow.com/questions/5056645/sorting-stdmap-using-value

  /// Take an std::pair<A,B> and return the flipped pair std::pair<B,A>
  template<typename A, typename B> constexpr std::pair<B,A> flip_pair(const std::pair<A,B> &p)
  {
    return std::pair<B,A>(p.second, p.first);
  }

  /// Take an std::map<A,B> and return the flipped map (now multimap to be safe): std::multimap<B,A>
  template<typename A, typename B> std::multimap<B,A> flip_map(const std::map<A,B> &src)
  {
    std::multimap<B,A> dst;
    for (const auto & x : src) dst.insert( flip_pair(x) );
    return dst;
  }
}

#endif
