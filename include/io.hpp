#ifndef IO_HPP
#define IO_HPP

#include <string>
#include <vector>
#include <string>
#include <fstream>

namespace io {

/*
 * General-purpose load functions
 * Supported types:
 * - all simple C types (NO ARRAYS)
 * - std::vector
 * 
 * Add specializations for other types
 */

template <typename T>
static std::size_t basic_load(std::istream& istrm, T& val)
{
    static_assert(std::is_fundamental<T>::value);
    if constexpr (std::is_array<T>::value) throw std::domain_error("[Function load] C arrays are not supported");
    istrm.read(reinterpret_cast<char*>(&val), sizeof(T));
    return sizeof(T);
}

template <typename T, typename Allocator>
static std::size_t basic_load(std::istream& istrm, std::vector<T, Allocator>& vec)
{
    std::size_t n;
    basic_load(istrm, n);
    vec.resize(n);
    std::size_t bytes_read = sizeof(n);
    for (auto& v : vec) bytes_read += basic_load(istrm, v);
    return bytes_read;
}

[[maybe_unused]] static std::size_t basic_load(std::istream& istrm, std::string& s)
{
    std::size_t n;
    basic_load(istrm, n);
    s.resize(n);
    std::size_t bytes_read = sizeof(n);
    for (auto& c : s) bytes_read += basic_load(istrm, c);
    return bytes_read;
}

//-----------------------------------------------------------------------------------------------------------------------

template <typename T>
static std::size_t basic_store(T const& val, std::ostream& ostrm)
{
    static_assert(std::is_fundamental<T>::value);
    if constexpr (std::is_array<T>::value) throw std::domain_error("[Function store] C arrays are not supported");
    ostrm.write(reinterpret_cast<char const*>(&val), sizeof(T));
    return sizeof(T);
}

template <typename T, typename Allocator>
static std::size_t basic_store(std::vector<T, Allocator> const& vec, std::ostream& ostrm)
{
    std::size_t n = vec.size();
    std::size_t bytes_written = basic_store(n, ostrm);
    for (auto const& v : vec) bytes_written += basic_store(v, ostrm);
    return bytes_written;
}

[[maybe_unused]] static std::size_t basic_store(std::string const& s, std::ostream& ostrm)
{
    std::size_t n = s.size();
    std::size_t bytes_written = basic_store(n, ostrm);
    for (auto const& c : s) bytes_written += basic_store(c, ostrm);
    return bytes_written;
}

//-------------------------------------------------------------------------------------------------------------------------

class loader 
{
    public:
        loader(std::istream& input_stream);

        template <typename T>
        void visit(T& var);

        template <typename T, class Allocator>
        void visit(std::vector<T, Allocator>& vec);

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
template <typename T>
void loader::visit(T& var)
{
    if constexpr (std::is_fundamental<T>::value) {
        // auto nr = 
        basic_load(istrm, var);
        // num_bytes_pods += nr;
    } else {
        var.visit(*this); // supposing the to-be saved object has a visit() method
    }
}

/**
 * This specialization for std::vector restricts the more general load() to vectors of basic C types.
 * This is to store their size separate from other types.
 */
template <typename T, typename Allocator>
void loader::visit(std::vector<T, Allocator>& vec)
{
    // if constexpr (std::is_fundamental<T>::value) {
    //     // auto nr = 
    //     basic_load(istrm, vec);
    //     // num_bytes_vecs_of_pods += nr;
    // } else {
        std::size_t n;
        visit(n);
        vec.resize(n);
        for (auto& v : vec) visit(v); // Call visit(), not load() since we want to recursively count the number of bytes
    // }
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

        void visit(std::string const& s);

        std::size_t get_byte_size() const {return ostrm.tellp();}
        // std::size_t get_byte_size_of_simple_types() const {return num_bytes_pods;}
        // std::size_t get_byte_size_of_vectors() const {return num_bytes_vecs_of_pods;}

    protected:
        std::ostream& ostrm;
        // std::size_t num_bytes_pods;
        // std::size_t num_bytes_vecs_of_pods;
};

class mut_saver : public saver
{
    public:
        mut_saver(std::ostream& output_stream);

        // non-const versions
        template <typename T>
        void visit(T& var);

        template <typename T, class Allocator>
        void visit(std::vector<T, Allocator>& vec);

        void visit(std::string& s);
};

template <typename T>
void saver::visit(T const& var) 
{
    if constexpr (std::is_fundamental<T>::value) {
        // auto nr = 
        basic_store(var, ostrm);
        // num_bytes_pods += nr;
    } else {
        var.visit(*this);
    }
}

template <typename T, typename Allocator>
void saver::visit(std::vector<T, Allocator> const& vec) 
{
    // if constexpr (std::is_fundamental<T>::value) {
    //     // auto nr = 
    //     basic_store(vec, ostrm);
    //     // num_bytes_vecs_of_pods += nr;
    // } else {
        size_t n = vec.size();
        visit(n);
        for (auto& v : vec) visit(v);
    // }
}

template <typename T>
void mut_saver::visit(T& var) 
{
    if constexpr (std::is_fundamental<T>::value) {
        // auto nr = 
        basic_store(var, ostrm);
        // num_bytes_vecs_of_pods += nr;
    } else {
        var.visit(*this);
    }
}

template <typename T, typename Allocator>
void mut_saver::visit(std::vector<T, Allocator>& vec) 
{
    // if constexpr (std::is_fundamental<T>::value) {
    //     // auto nr = 
    //     basic_store(vec, ostrm);
    //     // num_bytes_vecs_of_pods += nr;
    // } else {
        size_t n = vec.size();
        visit(n);
        for (auto& v : vec) visit(v);
    // }
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