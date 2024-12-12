#ifndef IO_HPP
#define IO_HPP

#include <cstdint>
#include <fstream>
#include <cstring> // memcpy
#include <string>
#include <vector>
#include <utility> // std::pair
#include <tuple> // TODO with variadic templates

namespace io {

/*
 * General-purpose load functions
 * Supported types:
 * - all simple C types (NO ARRAYS)
 * - std::pair
 * - std::vector
 * - std::string
 * 
 * Add specializations for other types
 */

[[maybe_unused]] std::size_t basic_parse(uint8_t const* istrm, std::string& s);

template <typename T>
[[maybe_unused]] static std::size_t basic_parse(uint8_t const* istrm, T& val)
{
    static_assert(std::is_fundamental<T>::value);
    if constexpr (std::is_array<T>::value) throw std::domain_error("[Function load] C arrays are not supported");
    std::memcpy(reinterpret_cast<char*>(&val), istrm, sizeof(T));
    return sizeof(T);
}

template <typename T, typename Allocator>
[[maybe_unused]] static std::size_t basic_parse(uint8_t const* istrm, std::vector<T, Allocator>& vec)
{
    // static_assert(std::is_fundamental<T>::value);
    auto start = istrm;
    std::size_t n;
    istrm += basic_parse(istrm, n);
    vec.resize(n);
    for (auto& v : vec) istrm += basic_parse(istrm, v);
    return istrm - start;
}

template <typename T1, typename T2>
[[maybe_unused]] static std::size_t basic_parse(uint8_t const* istrm, std::pair<T1, T2>& p)
{
    // static_assert(std::is_fundamental<T1>::value and std::is_fundamental<T2>::value);
    auto start = istrm;
    istrm += basic_parse(istrm, p.first);
    istrm += basic_parse(istrm, p.second);
    return istrm - start;
}

//-----------------------------------------------------------------------------------------------------------------------

[[maybe_unused]] std::size_t basic_load(std::istream& istrm, std::string& s);

template <typename T>
[[maybe_unused]] static std::size_t basic_load(std::istream& istrm, T& val)
{
    static_assert(std::is_fundamental<T>::value);
    if constexpr (std::is_array<T>::value) throw std::domain_error("[Function load] C arrays are not supported");
    istrm.read(reinterpret_cast<char*>(&val), sizeof(T));
    return sizeof(T);
}

template <typename T, typename Allocator>
[[maybe_unused]] static std::size_t basic_load(std::istream& istrm, std::vector<T, Allocator>& vec)
{
    // static_assert(std::is_fundamental<T>::value);
    std::size_t n;
    basic_load(istrm, n);
    vec.resize(n);
    std::size_t bytes_read = sizeof(n);
    for (auto& v : vec) bytes_read += basic_load(istrm, v);
    return bytes_read;
}

template <typename T1, typename T2>
[[maybe_unused]] static std::size_t basic_load(std::istream& istrm, std::pair<T1, T2>& p)
{
    // static_assert(std::is_fundamental<T1>::value and std::is_fundamental<T2>::value);
    std::size_t bytes_read = basic_load(istrm, p.first);
    bytes_read += basic_load(istrm, p.second);
    return bytes_read;
}

//-----------------------------------------------------------------------------------------------------------------------

[[maybe_unused]] std::size_t basic_store(std::string const& s, std::ostream& ostrm);

template <typename T>
[[maybe_unused]] static std::size_t basic_store(T const& val, std::ostream& ostrm)
{
    static_assert(std::is_fundamental<T>::value);
    if constexpr (std::is_array<T>::value) throw std::domain_error("[Function store] C arrays are not supported");
    ostrm.write(reinterpret_cast<char const*>(&val), sizeof(T));
    return sizeof(T);
}

template <typename T, typename Allocator>
[[maybe_unused]] static std::size_t basic_store(std::vector<T, Allocator> const& vec, std::ostream& ostrm)
{
    // static_assert(std::is_fundamental<T>::value);
    std::size_t n = vec.size();
    std::size_t bytes_written = basic_store(n, ostrm);
    for (auto const& v : vec) bytes_written += basic_store(v, ostrm);
    return bytes_written;
}

template <typename T1, typename T2>
[[maybe_unused]] static std::size_t basic_store(std::pair<T1, T2> const& p, std::ostream& ostrm)
{
    // static_assert(std::is_fundamental<T1>::value and std::is_fundamental<T2>::value);
    std::size_t bytes_written = basic_store(p.first, ostrm);
    bytes_written += basic_store(p.second, ostrm);
    return bytes_written;
}

//-------------------------------------------------------------------------------------------------------------------------

class loader 
{
    public:
        loader(std::istream& input_stream);

        template <class T>
        void visit(T& var);

        template <class T, class Allocator>
        void visit(std::vector<T, Allocator>& vec);

        template <class T1, class T2>
        void visit(std::pair<T1, T2>& p);

        void visit(std::string& s);

        std::size_t get_byte_size() const {return istrm.tellg();}
        // std::size_t get_byte_size_of_simple_types() const {return num_bytes_pods;}
        // std::size_t get_byte_size_of_vectors() const {return num_bytes_vecs_of_pods;}

    private:
        std::istream& istrm;
        // std::size_t num_bytes_pods;
        // std::size_t num_bytes_vecs_of_pods;
};

/*
 * The method is intended to be used by classes implementing a visit method.
 * The amount of bytes read is also saved, extending the functionality of the load() functions. 
 */
template <class T>
void loader::visit(T& var)
{
    if constexpr (std::is_fundamental<T>::value) {
        basic_load(istrm, var);
    } else {
        var.visit(*this); // supposing the to-be saved object has a visit() method
    }
}

/**
 * This specialization for std::vector restricts the more general load() to vectors of basic C types.
 * This is to store their size separate from other types.
 */
template <class T, typename Allocator>
void loader::visit(std::vector<T, Allocator>& vec)
{
    // DO NOT replace with basic_load
    std::size_t n;
    basic_load(istrm, n);
    vec.resize(n);
    for (auto& v : vec) visit(v); // because of this
}

template <class T1, class T2>
void loader::visit(std::pair<T1, T2>& p)
{
    visit(p.first);
    visit(p.second);
}

//-------------------------------------------------------------------------------------------------------------------------------

class saver 
{
    public:
        saver(std::ostream& output_stream);
        
        template <typename T>
        void visit(T const& var);

        template <typename T, class Allocator>
        void visit(std::vector<T, Allocator> const& vec);

        template <class T1, class T2>
        void visit(std::pair<T1, T2> const& p);

        void visit(std::string const& s);

        std::size_t get_byte_size() const {return ostrm.tellp();}
        // std::size_t get_byte_size_of_simple_types() const {return num_bytes_pods;}
        // std::size_t get_byte_size_of_vectors() const {return num_bytes_vecs_of_pods;}

    protected:
        std::ostream& ostrm;
        // std::size_t num_bytes_pods;
        // std::size_t num_bytes_vecs_of_pods;
};

/** 
 * Extension of saver visitor with non-const methods.
 * Primarily used when target classes load and store by using the very same 
 * visitor method (which will be non-const since loaders are non-const).
 */
class mut_saver : public saver
{
    public:
        mut_saver(std::ostream& output_stream);

        using saver::visit; // import template methods of base class

        
        template <typename T>
        void visit(T& var);

        template <typename T, class Allocator>
        void visit(std::vector<T, Allocator>& vec);

        template <class T1, class T2>
        void visit(std::pair<T1, T2>& p);

        void visit(std::string& s);
};

template <typename T>
void saver::visit(T const& var) 
{
    if constexpr (std::is_fundamental<T>::value) {
        basic_store(var, ostrm);
    } else {
        var.visit(*this);
    }
}

template <typename T, typename Allocator>
void saver::visit(std::vector<T, Allocator> const& vec) 
{
    // DO NOT replace with basic_store
    std::size_t n = vec.size();
    basic_store(n, ostrm);
    for (auto& v : vec) visit(v); // because of this
}

template <class T1, class T2>
void saver::visit(std::pair<T1, T2> const& p)
{
    visit(p.first);
    visit(p.second);
}

template <typename T>
void mut_saver::visit(T& var) 
{
    if constexpr (std::is_fundamental<T>::value) {
        basic_store(var, ostrm);
    } else {
        var.visit(*this);
    }
}

template <typename T, typename Allocator>
void mut_saver::visit(std::vector<T, Allocator>& vec) 
{
    // DO NOT replace with basic_store
    std::size_t n = vec.size();
    basic_store(n, ostrm);
    for (auto& v : vec) visit(v); // because of this
}

template <class T1, class T2>
void mut_saver::visit(std::pair<T1, T2>& p)
{
    visit(p.first);
    visit(p.second);
}

//----------------------------------------------------------------------------------------------------------------------------------

template <typename Visitor, typename T, class StreamType>
static std::size_t visit(T& data_structure, StreamType& strm)
{
    Visitor visitor(strm);
    visitor.visit(data_structure);
    return visitor.get_byte_size();
}

template <typename T>
static std::size_t load(T& data_structure, std::string filename)
{
    std::ifstream strm(filename, std::ios::binary);
    return visit<loader, T>(data_structure, strm);
}

template <typename T>
static T load(std::string filename)
{
    std::ifstream istrm(filename, std::ios::binary);
    loader ldr(istrm);
    return T::load(ldr);
}

template <typename T>
static std::size_t store(T& data_structure, std::string filename)
{
    std::ofstream strm(filename, std::ios::binary);
    return visit<saver, T>(data_structure, strm);
}

} // namespace io

#endif // IO_HPP