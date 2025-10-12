#ifndef _SERIALIZATION_HPP_
#define _SERIALIZATION_HPP_

#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include <set>
#include <unordered_set>
#include <string>
#include <tuple>
#include <type_traits>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>

// =====================
// Compatibility helpers
// =====================

// void_t (C++14 compatible)
#if __cplusplus >= 201703L
using std::void_t;
#else
template <typename... Ts>
using void_t = void;
#endif

// index_sequence (C++11+ compatible)
#if __cplusplus >= 201402L
using std::index_sequence;
using std::index_sequence_for;
using std::make_index_sequence;
#else
template <std::size_t... Is>
struct index_sequence{};

template <std::size_t N, std::size_t... Is>
struct make_index_sequence_impl : make_index_sequence_impl<N - 1, N - 1, Is...> {};

template <std::size_t... Is>
struct make_index_sequence_impl<0, Is...>{ using type = index_sequence<Is...>; };

template <std::size_t N>
using make_index_sequence = typename make_index_sequence_impl<N>::type;

template <typename... Ts>
using index_sequence_for = make_index_sequence<sizeof...(Ts)>;
#endif

namespace Serialization
{

  // =====================
  // Helper traits
  // =====================

  template <typename T>
  struct always_false : std::false_type {};

  // map-like detection
  template <typename T, typename = void>
  struct is_map_like : std::false_type {};

  template <typename T>
  struct is_map_like<T, void_t<typename T::mapped_type>> : std::true_type {};

  // string detection
  template <typename T>
  struct is_std_string : std::false_type {};

  template <>
  struct is_std_string<std::string> : std::true_type {};

  // push_back detection
  template <typename T, typename = void>
  struct has_push_back : std::false_type {};

  template <typename T>
  struct has_push_back<T, void_t<
                            decltype(std::declval<T &>().push_back(std::declval<typename T::value_type>()))>> : std::true_type {};

  // tuple-like detection
  template <typename T, typename = void>
  struct is_tuple_like : std::false_type {};

  template <typename T>
  struct is_tuple_like<T, void_t<decltype(std::tuple_size<T>::value)>> : std::true_type {};

  // sequence-like detection (has begin() and end())
  template <typename T, typename = void>
  struct has_begin_end : std::false_type {};

  template <typename T>
  struct has_begin_end<T, void_t<
                              decltype(std::declval<T &>().begin()),
                              decltype(std::declval<T &>().end())>> : std::true_type {};

  // =====================
  // Serializer
  // =====================
  class Serializer
  {
  private:
    std::vector<uint8_t> buffer;

    // POD
    template <typename T>
    void writePod(const T &value)
    {
      static_assert(std::is_trivially_copyable<T>::value, "writePod: T must be trivially copyable");
      const uint8_t *p = reinterpret_cast<const uint8_t *>(&value);
      buffer.insert(buffer.end(), p, p + sizeof(T));
    }

    // String
    void writeString(const std::string &s)
    {
      uint64_t len = s.size();
      writePod(len);
      buffer.insert(buffer.end(),
                    reinterpret_cast<const uint8_t *>(s.data()),
                    reinterpret_cast<const uint8_t *>(s.data()) + len);
    }

    // Sequence-like
    template <typename Seq>
    void writeSequenceLike(const Seq &seq)
    {
      uint64_t len = seq.size();
      writePod(len);
      for (const auto &v : seq)
        write(v);
    }

    // Tuple-like
    template <typename T, size_t... I>
    void writeTupleElements(const T &tup, index_sequence<I...>)
    {
      using expander = int[];
      (void)expander{0, (write(std::get<I>(tup)), void(), 0)...};
    }

    template <typename T>
    void writeTupleLike(const T &tup)
    {
      constexpr size_t N = std::tuple_size<T>::value;
      writeTupleElements(tup, make_index_sequence<N>{});
    }

    // Map-like
    template <typename Map>
    void writeMapLike(const Map &map)
    {
      uint64_t len = map.size();
      writePod(len);
      for (typename Map::const_iterator it = map.begin(); it != map.end(); ++it)
      {
        write(it->first);
        write(it->second);
      }
    }

    // =====================
    // Write dispatcher (SFINAE)
    // =====================
    template <typename T, typename Enable = void>
    struct write_helper
    {
      static void apply(Serializer &, const T &)
      {
        static_assert(always_false<T>::value, "Unsupported type for Serializer::write()");
      }
    };

    template <typename T>
    struct write_helper<T, typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    {
      static void apply(Serializer &s, const T &v) { s.writePod(v); }
    };

    template <typename T>
    struct write_helper<T, typename std::enable_if<is_std_string<T>::value>::type>
    {
      static void apply(Serializer &s, const T &v) { s.writeString(v); }
    };

    template <typename T>
    struct write_helper<T, typename std::enable_if<is_map_like<T>::value>::type>
    {
      static void apply(Serializer &s, const T &v) { s.writeMapLike(v); }
    };

    template <typename T>
    struct write_helper<T, typename std::enable_if<is_tuple_like<T>::value>::type>
    {
      static void apply(Serializer &s, const T &v) { s.writeTupleLike(v); }
    };

    template <typename T>
    struct write_helper<T, typename std::enable_if<!is_map_like<T>::value &&
                                                   !is_std_string<T>::value &&
                                                   has_begin_end<T>::value>::type>
    {
      static void apply(Serializer &s, const T &v) { s.writeSequenceLike(v); }
    };

  public:
    template <typename T>
    void write(const T &value)
    {
      write_helper<T>::apply(*this, value);
    }

    const uint8_t* data() const
    {
      return buffer.data();
    }

    size_t dataLength() const
    {
      return buffer.size();
    }
  };

  // =====================
  // Deserializer
  // =====================
  class Deserializer
  {
  public:
    explicit Deserializer(const std::vector<uint8_t> &buf)
        : data(buf.data()), size(buf.size()) {}

    explicit Deserializer(const uint8_t* &buf, size_t length)
        : data(buf), size(length) {}

    // POD
    template <typename T>
    void readPod(T &value)
    {
      static_assert(std::is_trivially_copyable<T>::value, "readPod: T must be trivially copyable");
      if (pos + sizeof(T) > size)
        throw std::runtime_error("Buffer underflow");
      std::memcpy(&value, data + pos, sizeof(T));
      pos += sizeof(T);
    }

    // String
    void readString(std::string &s)
    {
      uint64_t len;
      readPod(len);
      if (pos + len > size)
        throw std::runtime_error("Buffer underflow");
      s.assign(reinterpret_cast<const char *>(data + pos), len);
      pos += len;
    }

    // =====================
    // readSequenceLike overloads
    // =====================
    // Push-backable sequences: vector, list, deque
    template <typename Seq>
    typename std::enable_if<has_push_back<Seq>::value>::type
    readSequenceLike(Seq &seq)
    {
      uint64_t len;
      readPod(len);
      seq.clear();
      for (uint64_t i = 0; i < len; ++i)
      {
        typename Seq::value_type v;
        read(v);
        seq.push_back(std::move(v));
      }
    }

    // Insert-only sequences: set, unordered_set
    template <typename Seq>
    typename std::enable_if<!has_push_back<Seq>::value && has_begin_end<Seq>::value>::type
    readSequenceLike(Seq &seq)
    {
      uint64_t len;
      readPod(len);
      seq.clear();
      for (uint64_t i = 0; i < len; ++i)
      {
        typename Seq::value_type v;
        read(v);
        seq.insert(std::move(v));
      }
    }

    // Generic tuple-like reader
    template <typename T, std::size_t... I>
    void readTupleElements(T &tup, index_sequence<I...>)
    {
      using expander = int[];
      (void)expander{0, (read(std::get<I>(tup)), 0)...};
    }

    template <typename T>
    void readTupleLike(T &tup)
    {
      const std::size_t N = std::tuple_size<T>::value;
      readTupleElements(tup, make_index_sequence<N>{});
    }

    // Map-like
    template <typename Map>
    void readMapLike(Map &map)
    {
      uint64_t len;
      readPod(len);
      map.clear();
      for (uint64_t i = 0; i < len; ++i)
      {
        typename Map::key_type k;
        typename Map::mapped_type v;
        read(k);
        read(v);
        map.emplace(std::move(k), std::move(v));
      }
    }

  private:
    const uint8_t *data;
    size_t pos = 0;
    size_t size = 0;

    // =====================
    // Read dispatcher (SFINAE)
    // =====================
    template <typename T, typename Enable = void>
    struct read_helper
    {
      static void apply(Deserializer &, T &)
      {
        static_assert(always_false<T>::value, "Unsupported type for Deserializer::read()");
      }
    };

    template <typename T>
    struct read_helper<T, typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    {
      static void apply(Deserializer &d, T &v) { d.readPod(v); }
    };

    template <typename T>
    struct read_helper<T, typename std::enable_if<is_std_string<T>::value>::type>
    {
      static void apply(Deserializer &d, T &v) { d.readString(v); }
    };

    template <typename T>
    struct read_helper<T, typename std::enable_if<is_map_like<T>::value>::type>
    {
      static void apply(Deserializer &d, T &v) { d.readMapLike(v); }
    };

    template <typename T>
    struct read_helper<T, typename std::enable_if<is_tuple_like<T>::value>::type>
    {
      static void apply(Deserializer &d, T &v) { d.readTupleLike(v); }
    };

    template <typename T>
    struct read_helper<T, typename std::enable_if<!is_map_like<T>::value &&
                                                  !is_std_string<T>::value &&
                                                  has_begin_end<T>::value>::type>
    {
      static void apply(Deserializer &d, T &v) { d.readSequenceLike(v); }
    };

  public:
    template <typename T>
    void read(T &value)
    {
      read_helper<T>::apply(*this, value);
    }
  };

} // namespace Serialization

#endif // _SERIALIZATION_HPP_

#include "Serialization.tpp"
