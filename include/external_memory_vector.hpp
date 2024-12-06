/**
 * Original author: Giulio Ermanno Pibiri (jermp)
 * 
 * Modified by: Yoshihiro Shibuya (yhhshb)
 */

#ifndef EXTERNAL_MEMORY_VECTOR
#define EXTERNAL_MEMORY_VECTOR

#include <algorithm>
#include <functional>
#include <vector>
#include <sstream>
#include "io.hpp"
#include "memory_mapped_file.hpp"

namespace emem {

template <typename T>
struct sorted_base {
    sorted_base(std::function<bool(T const&, T const&)> cmp) : m_sorter(cmp) {}
    std::function<bool(T const&, T const&)> m_sorter;
};

template <typename T>
struct unsorted_base {};

template <typename T, bool sorted = true>
class external_memory_vector : public std::conditional<sorted, sorted_base<T>, unsorted_base<T>>::type 
{
    public:
        using value_type = T;
        
        class const_iterator
        {
            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = T;
                using pointer           = value_type*;
                using reference         = value_type&;
                
                const_iterator(external_memory_vector<T, sorted> const* vec);
                const_iterator(external_memory_vector<T, sorted> const* vec, int);
                T const& operator*() const;
                const_iterator const& operator++();
                bool operator==(const_iterator const& other) const;
                bool operator!=(const_iterator const& other) const;

            private:
                class mm_parser
                {
                    public:
                        mm_parser(uint8_t const* begin, uint8_t const* end) 
                            : m_begin(begin)
                            , m_end(end)
                            , read(0)
                        {
                            read = io::basic_parse(m_begin, buffer);
                        }

                        void advance() 
                        { 
                            // m_begin += sizeof(T);
                            m_begin += read;
                            read = 0;
                            if (has_next()) read = io::basic_parse(m_begin, buffer);
                        }

                        T const& parse() const
                        { 
                            // return *reinterpret_cast<T const*>(m_begin);
                            return buffer;
                        }

                        bool has_next() const 
                        { 
                            return m_begin != m_end;
                        }

                    private:
                        uint8_t const* m_begin;
                        uint8_t const* m_end;
                        T buffer;
                        std::size_t read;
                };

                external_memory_vector<T, sorted> const* v;
                std::vector<mm_parser> m_parsers;
                std::vector<uint32_t> m_idx_heap;
                std::vector<memory::map::file_source<uint8_t>> m_mm_files;
                std::function<bool(uint32_t, uint32_t)> heap_idx_comparator;
                template <bool s = sorted>
                typename std::enable_if<s, void>::type advance_heap_head();
                template <bool s = sorted>
                typename std::enable_if<!s, void>::type advance_heap_head();
        };

        template <bool s = sorted>
        external_memory_vector(
            typename std::enable_if<s, uint64_t>::type available_space_bytes, 
            std::function<bool(T const&, T const&)> cmp, 
            std::string tmp_dir, 
            std::string name = "");
        
        template <bool s = sorted>
        external_memory_vector(
            typename std::enable_if<s, uint64_t>::type available_space_bytes, 
            std::string tmp_dir, 
            std::string name = "");
        
        template <bool s = sorted>
        external_memory_vector(
            typename std::enable_if<!s, uint64_t>::type available_space_bytes, 
            std::string tmp_dir, 
            std::string name = "");

        external_memory_vector(external_memory_vector&&) = default;
        void push_back(T const& elem);
        const_iterator cbegin() const;
        const_iterator cend() const;
        std::size_t size() const;
        void minimize();
        ~external_memory_vector();

    private:
        void init(uint64_t available_space_bytes);
        std::size_t m_buffer_size;
        std::size_t m_total_elems;
        std::string m_tmp_dirname;
        std::string m_prefix;
        std::vector<std::string> m_tmp_files;
        std::vector<T> m_buffer;
        void sort_and_flush();
        std::string get_tmp_output_filename(uint64_t id) const;
};

template <typename T, bool sorted>
template <bool s>
external_memory_vector<T, sorted>::external_memory_vector(
    typename std::enable_if<s, uint64_t>::type available_space_bytes,
    std::function<bool(T const&, T const&)> cmp, std::string tmp_dir, std::string name
) : sorted_base<T>(cmp), m_total_elems(0)
  , m_tmp_dirname(tmp_dir)
  , m_prefix(name) 
{
    init(available_space_bytes);
}

template <typename T, bool sorted>
template <bool s>
external_memory_vector<T, sorted>::external_memory_vector(
    typename std::enable_if<s, uint64_t>::type available_space_bytes, 
    std::string tmp_dir, 
    std::string name
) : sorted_base<T>([](T const& a, T const& b) { return a < b; })
  , m_total_elems(0)
  , m_tmp_dirname(tmp_dir)
  , m_prefix(name) 
{
    init(available_space_bytes);
}

template <typename T, bool sorted>
template <bool s>
external_memory_vector<T, sorted>::external_memory_vector(
    typename std::enable_if<!s, uint64_t>::type available_space_bytes, 
    std::string tmp_dir, 
    std::string name)
    : m_total_elems(0), m_tmp_dirname(tmp_dir), m_prefix(name) 
{
    init(available_space_bytes);
}

template <typename T, bool sorted>
void 
external_memory_vector<T, sorted>::init(uint64_t available_space_bytes) 
{
    if (available_space_bytes / sizeof(T) == 0) throw std::runtime_error("[EMV] Insufficient memory");
    m_buffer_size = available_space_bytes / sizeof(T) + 1;
    m_buffer.reserve(m_buffer_size);
}

template <typename T, bool sorted>
void 
external_memory_vector<T, sorted>::push_back(T const& elem) 
{
    // for optimal memory management in the general case one should try to reload the last tmp file if it isn't full
    m_buffer.reserve(m_buffer_size); // does nothing if enough space
    m_buffer.push_back(elem);
    ++m_total_elems;
    if (m_buffer.size() >= m_buffer_size) {
        sort_and_flush();
        m_buffer.clear();
    }
}

template <typename T, bool sorted>
typename external_memory_vector<T, sorted>::const_iterator
external_memory_vector<T, sorted>::cbegin() const
{
    external_memory_vector<T, sorted>& me = const_cast<external_memory_vector<T, sorted>&>(*this);
    me.minimize();
    // TODO: when creating an iterator do not call minimize(), just write the file instead.
    // That way we can keep adding elements to the same buffer and then update the last file
    return const_iterator(this);
}

template <typename T, bool sorted>
typename external_memory_vector<T, sorted>::const_iterator 
external_memory_vector<T, sorted>::cend() const 
{
    return const_iterator(this, 0);
}

template <typename T, bool sorted>
std::size_t 
external_memory_vector<T, sorted>::size() const 
{
    return m_total_elems;
}

/** 
 * Flushes and deallocate internal memory for minimal RAM footprint.
 */
template <typename T, bool sorted>
void 
external_memory_vector<T, sorted>::minimize()
{
    if (m_buffer.size() != 0) sort_and_flush();
    m_buffer.clear();
    m_buffer.shrink_to_fit();
    assert(m_buffer.capacity() == 0);
}

template <typename T, bool sorted>
external_memory_vector<T, sorted>::~external_memory_vector() 
{
    for (auto tmp : m_tmp_files) std::remove(tmp.c_str());
}

template <typename T, bool sorted>
void 
external_memory_vector<T, sorted>::sort_and_flush() 
{
    if constexpr (sorted) std::sort(m_buffer.begin(), m_buffer.end(), sorted_base<T>::m_sorter);
    m_tmp_files.push_back(get_tmp_output_filename(m_tmp_files.size()));
    std::ofstream out(m_tmp_files.back().c_str(), std::ofstream::binary);
    // out.write(reinterpret_cast<char const*>(m_buffer.data()), m_buffer.size() * sizeof(T)); // old C-like version unable to deal with VL structures
    for (auto& elem : m_buffer) io::basic_store(elem, out); // use custom overloads (see io.hpp for list of supported types)
}

template <typename T, bool sorted>
std::string 
external_memory_vector<T, sorted>::get_tmp_output_filename(uint64_t id) const 
{
    std::stringstream filename;
    filename << m_tmp_dirname << "/tmp.run";
    if (m_prefix != "") filename << "_" << m_prefix;
    filename << "_" << id << ".bin";
    return filename.str();
}

template <typename T, bool sorted>
external_memory_vector<T, sorted>::const_iterator::const_iterator(external_memory_vector<T, sorted> const* vec)
    : v(vec)
    , heap_idx_comparator([this](uint32_t i, uint32_t j) { return (m_parsers[i].parse() > m_parsers[j].parse()); }) 
{
    m_parsers.reserve(v->m_tmp_files.size());
    m_idx_heap.reserve(v->m_tmp_files.size());
    m_mm_files.reserve(v->m_tmp_files.size());
    /* create the input iterators and make the heap */
    for (uint64_t i = 0; i != v->m_tmp_files.size(); ++i) {
        m_mm_files.emplace_back(v->m_tmp_files.at(i), memory::map::advice::sequential);
        m_parsers.emplace_back(m_mm_files[i].data(), m_mm_files[i].data() + m_mm_files[i].size());
        m_idx_heap.push_back(i);
    }
    if constexpr (sorted) std::make_heap(m_idx_heap.begin(), m_idx_heap.end(), heap_idx_comparator);
    else std::reverse(m_idx_heap.begin(), m_idx_heap.end());
}

template <typename T, bool sorted>
external_memory_vector<T, sorted>::const_iterator::const_iterator(external_memory_vector<T, sorted> const* vec, [[maybe_unused]]int dummy)
    : v(vec),
    heap_idx_comparator([]([[maybe_unused]] uint32_t i, [[maybe_unused]] uint32_t j) {return false;})
{}

template <typename T, bool sorted>
T const& 
external_memory_vector<T, sorted>::const_iterator::operator*() const 
{
    if constexpr (sorted) return m_parsers[m_idx_heap.front()].parse();
    else return m_parsers[m_idx_heap.back()].parse();
}

template <typename T, bool sorted>
typename external_memory_vector<T, sorted>::const_iterator const& 
external_memory_vector<T, sorted>::const_iterator::operator++() 
{
    advance_heap_head();
    return *this;
}

template <typename T, bool sorted>
bool 
external_memory_vector<T, sorted>::const_iterator::operator==(const_iterator const& other) const 
{
    bool same_vector = v == other.v;
    bool same_idx = m_idx_heap.size() == other.m_idx_heap.size();
    return same_vector and same_idx;  // TODO make it a little bit stronger
}

template <typename T, bool sorted>
bool 
external_memory_vector<T, sorted>::const_iterator::operator!=(const_iterator const& other) const 
{
    return !(operator==(other));
}

template <typename T, bool sorted>
template <bool s>
typename std::enable_if<s, void>::type
external_memory_vector<T, sorted>::const_iterator::advance_heap_head() 
{
    uint32_t idx = m_idx_heap.front();
    m_parsers[idx].advance();
    if (m_parsers[idx].has_next()) {  // percolate down the head
        uint64_t pos = 0;
        uint64_t size = m_idx_heap.size();
        while (2 * pos + 1 < size) {
            uint64_t i = 2 * pos + 1;
            if (i + 1 < size and heap_idx_comparator(m_idx_heap[i], m_idx_heap[i + 1])) ++i;
            if (heap_idx_comparator(m_idx_heap[i], m_idx_heap[pos])) break;
            std::swap(m_idx_heap[pos], m_idx_heap[i]);
            pos = i;
        }
    } else {
        std::pop_heap(m_idx_heap.begin(), m_idx_heap.end(), heap_idx_comparator);
        m_idx_heap.pop_back();
    }
}

template <typename T, bool sorted>
template <bool s>
typename std::enable_if<!s, void>::type
external_memory_vector<T, sorted>::const_iterator::advance_heap_head() 
{
    uint32_t idx = m_idx_heap.back();  // we reversed the array when !sorted
    m_parsers[idx].advance();
    if (m_parsers[idx].has_next()) {}  // do nothing since next time the index will be still valid
    else m_idx_heap.pop_back();
}

}  // namespace emem

#endif // EXTERNAL_MEMORY_VECTOR
