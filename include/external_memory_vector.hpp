/**
 * Original author: Giulio Ermanno Pibiri (jermp)
 * 
 * Modified by: Yoshihiro Shibuya (yhhshb)
 */

#ifndef EXTERNAL_MEMORY_VECTOR
#define EXTERNAL_MEMORY_VECTOR

#include <functional>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
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
                
                const_iterator();
                const_iterator(external_memory_vector<T, sorted> const* vec);
                T const& operator*() const;
                const_iterator const& operator++();
                bool operator==(const_iterator const& other) const;
                bool operator!=(const_iterator const& other) const;

            private:
                class mm_loader_iterator 
                {
                    public:
                        mm_loader_iterator(uint8_t const* begin, uint8_t const* end) : m_begin(begin), m_end(end) {}
                        void operator++() { m_begin += sizeof(T); }
                        T const& operator*() const { return *reinterpret_cast<T const*>(m_begin); }
                        bool has_next() const { return m_begin != m_end; }

                    private:
                        uint8_t const* m_begin;
                        uint8_t const* m_end;
                };

                external_memory_vector<T, sorted> const* v;
                std::vector<mm_loader_iterator> m_iterators;
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
    if (m_buffer.size() >= m_buffer_size) sort_and_flush();
}

template <typename T, bool sorted>
typename external_memory_vector<T, sorted>::const_iterator
external_memory_vector<T, sorted>::cbegin() const
{
    external_memory_vector<T, sorted>& me = const_cast<external_memory_vector<T, sorted>&>(*this);
    if (m_buffer.size() != 0) me.sort_and_flush();
    me.m_buffer.clear();
    me.m_buffer.shrink_to_fit();
    return const_iterator(this);
}

template <typename T, bool sorted>
typename external_memory_vector<T, sorted>::const_iterator 
external_memory_vector<T, sorted>::cend() const 
{
    return const_iterator();
}

template <typename T, bool sorted>
std::size_t 
external_memory_vector<T, sorted>::size() const 
{
    return m_total_elems;
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
    out.write(reinterpret_cast<char const*>(m_buffer.data()), m_buffer.size() * sizeof(T));
    m_buffer.clear();
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
    // , m_mm_files(v->m_tmp_files.size())
    , heap_idx_comparator([this](uint32_t i, uint32_t j) { return (*m_iterators[i] > *m_iterators[j]); }) 
{
    m_iterators.reserve(v->m_tmp_files.size());
    m_idx_heap.reserve(v->m_tmp_files.size());
    m_mm_files.reserve(v->m_tmp_files.size());
    /* create the input iterators and make the heap */
    for (uint64_t i = 0; i != v->m_tmp_files.size(); ++i) {
        m_mm_files.emplace_back(v->m_tmp_files.at(i), memory::map::advice::sequential);
        m_iterators.emplace_back(m_mm_files[i].data(), m_mm_files[i].data() + m_mm_files[i].size());
        m_idx_heap.push_back(i);
    }
    if constexpr (sorted) std::make_heap(m_idx_heap.begin(), m_idx_heap.end(), heap_idx_comparator);
    else std::reverse(m_idx_heap.begin(), m_idx_heap.end());
}

template <typename T, bool sorted>
external_memory_vector<T, sorted>::const_iterator::const_iterator()
    : v(nullptr),
    heap_idx_comparator([]([[maybe_unused]] uint32_t i, [[maybe_unused]] uint32_t j) {return false;})
{}

template <typename T, bool sorted>
T const& 
external_memory_vector<T, sorted>::const_iterator::operator*() const 
{
    if constexpr (sorted) return *m_iterators[m_idx_heap.front()];
    else return *m_iterators[m_idx_heap.back()];
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
    return m_idx_heap.size() == other.m_idx_heap.size();  // TODO make it a little bit stronger
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
    ++m_iterators[idx];
    if (m_iterators[idx].has_next()) {  // percolate down the head
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
    ++m_iterators[idx];
    if (m_iterators[idx].has_next()) {}  // do nothing since next time the index will be still valid
    else m_idx_heap.pop_back();
}

}  // namespace emem

#endif // EXTERNAL_MEMORY_VECTOR
