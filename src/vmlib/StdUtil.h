#ifndef GC_STDUTIL_H
#define GC_STDUTIL_H

namespace std
{
template<class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}//ns

#endif //GC_STDUTIL_H
