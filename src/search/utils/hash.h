#ifndef UTILS_HASH_H
#define UTILS_HASH_H

#include <cassert>
#include <functional>
#include <utility>
#include <vector>

namespace utils {
/*
  Ideas and code for our hash functions have been taken and adapted from
  lookup3.c, by Bob Jenkins, May 2006, Public Domain.
  (http://www.burtleburtle.net/bob/c/lookup3.c)
*/

static_assert(sizeof(unsigned int) == 4, "unsigned int has unexpected size");

/*
  Circular rotation (http://stackoverflow.com/a/31488147/224132).
*/
static inline unsigned int rotate(unsigned int value, unsigned int offset) {
    return (value << offset) | (value >> (32 - offset));
}

/*
  Mix 3 32-bit values bijectively.

  Any information in (a, b, c) before mix() is still in (a, b, c) after
  mix().
*/
static inline void mix(unsigned int &a, unsigned int &b, unsigned int &c) {
    a -= c;
    a ^= rotate(c, 4);
    c += b;
    b -= a;
    b ^= rotate(a, 6);
    a += c;
    c -= b;
    c ^= rotate(b, 8);
    b += a;
    a -= c;
    a ^= rotate(c, 16);
    c += b;
    b -= a;
    b ^= rotate(a, 19);
    a += c;
    c -= b;
    c ^= rotate(b, 4);
    b += a;
}

/*
  Final mixing of 3 32-bit values (a, b, c) into c.

  Triples of (a, b, c) differing in only a few bits will usually produce values
  of c that look totally different.
*/
static inline void final_mix(unsigned int &a, unsigned int &b, unsigned int &c) {
    c ^= b;
    c -= rotate(b, 14);
    a ^= c;
    a -= rotate(c, 11);
    b ^= a;
    b -= rotate(a, 25);
    c ^= b;
    c -= rotate(b, 16);
    a ^= c;
    a -= rotate(c, 4);
    b ^= a;
    b -= rotate(a, 14);
    c ^= b;
    c -= rotate(b, 24);
}

/*
  Internal class storing the state of the hashing process.
*/
class HashState {
    unsigned int a, b, c;
    int pending_values;

public:
    HashState()
        : a(0xdeadbeef),
          b(a),
          c(a),
          pending_values(0) {
    }

    void feed(unsigned int value) {
        assert(pending_values != -1);
        if (pending_values == 3) {
            mix(a, b, c);
            pending_values = 0;
        }
        if (pending_values == 0) {
            a += value;
            ++pending_values;
        } else if (pending_values == 1) {
            b += value;
            ++pending_values;
        } else if (pending_values == 2) {
            c += value;
            ++pending_values;
        }
    }

    unsigned int get_hash_value32() {
        assert(pending_values != -1);
        if (pending_values) {
            /*
               pending_values == 0 can only hold if we never called
               feed(), i.e., if we are hashing an empty sequence.
            */
            final_mix(a, b, c);
        }
        pending_values = -1;
        return c;
    }

    uint64_t get_hash_value64() {
        assert(pending_values != -1);
        if (pending_values) {
            /*
               pending_values == 0 can only hold if we never called
               feed(), i.e., if we are hashing an empty sequence.
            */
            final_mix(a, b, c);
        }
        pending_values = -1;
        return (static_cast<uint64_t>(b) << 32) | c;
    }
};


/*
  These functions add a new object to an existing HashState object.

  You can add hashing support for your own type by providing such a function
  for your type in the "util" namespace.
*/
inline void feed(HashState &hash_state, int value) {
    hash_state.feed(static_cast<unsigned int>(value));
}

inline void feed(HashState &hash_state, unsigned int value) {
    hash_state.feed(value);
}

inline void feed(HashState &hash_state, uint64_t value) {
    hash_state.feed(static_cast<uint32_t>(value));
    value >>= 32;
    hash_state.feed(static_cast<uint32_t>(value));
}

template<typename T1, typename T2>
void feed(HashState &hash_state, const std::pair<T1, T2> &p) {
    feed(hash_state, p.first);
    feed(hash_state, p.second);
}


/*
  Public hash functions. By providing a suitable feed() function in the "util"
  namespace, you can add support for custom types.
*/
template<typename T>
uint32_t get_hash32(const T &value) {
    HashState hash_state;
    feed(hash_state, value);
    return hash_state.get_hash_value32();
}

template<typename T>
uint64_t get_hash64(const T &value) {
    HashState hash_state;
    feed(hash_state, value);
    return hash_state.get_hash_value64();
}

template<typename T>
size_t get_hash(const T &value) {
    return static_cast<size_t>(get_hash64(value));
}

/*
  Hash a new value and combine it with an existing hash.

  This function should only be called from within this module.
*/
template<typename T>
inline void hash_combine(size_t &hash, const T &value) {
    std::hash<T> hasher;
    /*
      The combination of hash values is based on issue 6.18 in
      http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2005/n1756.pdf.
      Boost combines hash values in the same way.
    */
    hash ^= hasher(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
}

template<typename Sequence>
size_t hash_sequence(const Sequence &data, size_t length) {
    size_t hash = 0;
    for (size_t i = 0; i < length; ++i) {
        hash_combine(hash, data[i]);
    }
    return hash;
}

/*
  Hash an array of unsigned ints starting at "key" with length
  "length".

  This is the hashword() function from
  http://www.burtleburtle.net/bob/c/lookup3.c (public domain).
*/
extern unsigned int hash_unsigned_int_sequence(
    const unsigned int *key,
    unsigned int length);
}

namespace std {
template<typename T>
struct hash<std::vector<T>> {
    size_t operator()(const std::vector<T> &vec) const {
        return utils::hash_sequence(vec, vec.size());
    }
};

template<typename TA, typename TB>
struct hash<std::pair<TA, TB>> {
    size_t operator()(const std::pair<TA, TB> &pair) const {
        size_t hash = 0;
        utils::hash_combine(hash, pair.first);
        utils::hash_combine(hash, pair.second);
        return hash;
    }
};
}

#endif
