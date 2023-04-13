/**
 * Original author: Giulio Ermanno Pibiri
 * at: https://github.com/jermp/mm_file/tree/41ca01bd11f2f84e485d823231d7740794bd0ef5
 * 
 * Modified by: Yoshihiro Shibuya
 */

#ifndef MEMORY_MAPPED_FILE_HPP
#define MEMORY_MAPPED_FILE_HPP

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>  // close(fd)

#include <cstdint>
#include <type_traits>
#include <string>

namespace mm {

namespace advice {
    static const int normal = POSIX_MADV_NORMAL;
    static const int random = POSIX_MADV_RANDOM;
    static const int sequential = POSIX_MADV_SEQUENTIAL;
}  // namespace advice

template <typename Pointer>
Pointer mmap(int fd, uint64_t size, int prot) 
{
    static const size_t offset = 0;
    Pointer p = static_cast<Pointer>(::mmap(NULL, size, prot, MAP_SHARED, fd, offset));
    if (p == MAP_FAILED) throw std::runtime_error("[mm::file] mmap failed");
    return p;
}

template <typename T>
class file
{
    public:
        class const_iterator 
        {
            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = T;
                using pointer           = value_type*;
                using reference         = value_type&;

                const_iterator(T* addr, size_t offset = 0) : m_ptr(addr + offset) {}
                T operator*() { return *m_ptr; }
                void operator++() { ++m_ptr; }
                bool operator==(const_iterator const& rhs) const { return m_ptr == rhs.m_ptr; }
                bool operator!=(const_iterator const& rhs) const { return !((*this) == rhs); }
            private:
                T* m_ptr;
        };

        file();
        file(file const&);             // non-copy-constructable
        file& operator=(file const&);  // non-copyable
        file(file&&) noexcept;
        bool is_open() const;
        const_iterator cbegin() const;
        const_iterator cend() const;
        uint64_t bytes() const;
        uint64_t size() const;
        T* data() const;
        void close();
        ~file();

    protected:
        int m_fd;
        size_t m_size;
        T* m_data;
        std::size_t* ref_count;
        void check_fd();

    private:
        void move(file&) noexcept;
};

template <typename T>
class file_source : public file<T const> 
{
    public:
        typedef file<T const> base;

        file_source() {}
        file_source(std::string const& path, int adv = advice::normal) { open(path, adv); }

    private:
        void open(std::string const& path, int adv = advice::normal);
};

template <typename T>
class file_sink : public file<T> {
    public:
        typedef file<T> base;

        file_sink() {}
        file_sink(std::string const& path) { open(path); }
        file_sink(std::string const& path, size_t n) { open(path, n); }

    private:
        void open(std::string const& path);
        void open(std::string const& path, size_t n);
};

template <typename T>
file<T>::file()
{
    m_fd = -1;
    m_size = 0;
    m_data = nullptr;
    ref_count = nullptr;
}

template <typename T>
file<T>::file(file const& other)
{
    m_fd = other.m_fd;
    m_size = other.m_size;
    m_data = other.m_data;
    ref_count = other.ref_count;
    if (other.is_open()) {
        if (ref_count == nullptr) std::runtime_error("[mm::file] file open with invalid ref_count");
        if (*ref_count == 0) std::runtime_error("[mm::file] file open with ref_count = 0");
        ++(*ref_count);
    } else {
        if (ref_count != nullptr) std::runtime_error("[mm::file] source file closed with valid reference count");
    }
}

template <typename T>
void file<T>::move(file& other) noexcept
{
    m_fd = other.m_fd;
    m_size = other.m_size;
    m_data = other.m_data;
    ref_count = other.ref_count;
    other.m_fd = -1;
    other.m_size = 0;
    other.m_data = nullptr;
    other.ref_count = nullptr;
}

template <typename T>
file<T>::file(file&& other) noexcept
{
    move(other);
}

template <typename T>
file<T>& file<T>::operator=(file const& source)
{
    file copy = file(source);
    move(copy);
    return *this;
}

template <typename T>
bool file<T>::is_open() const { return m_fd != -1; }

template <typename T>
typename file<T>::const_iterator file<T>::cbegin() const { return const_iterator(m_data); }

template <typename T>
typename file<T>::const_iterator file<T>::cend() const { return const_iterator(m_data, size()); }

template <typename T>
uint64_t file<T>::bytes() const { return m_size; }

template <typename T>
uint64_t file<T>::size() const { return m_size / sizeof(T); }

template <typename T>
T* file<T>::data() const { return m_data; }

template <typename T>
void file<T>::close()
{
    if (is_open()) {
        if(ref_count == nullptr or *ref_count == 0) throw std::runtime_error("[mm::file] reference count is already 0 before decrement");
        --(*ref_count);
        if (*ref_count == 0) {
            if (munmap((char*)m_data, m_size) == -1) throw std::runtime_error("[mm::file] munmap failed when closing file");
            ::close(m_fd);
            delete ref_count;
        }
        m_fd = -1;
        m_size = 0;
        m_data = nullptr;
        ref_count = nullptr;
    }
}

template <typename T>
file<T>::~file()
{
    close();
}

template <typename T>
void file<T>::check_fd() 
{
    if (m_fd == -1) throw std::runtime_error("[mm::file] cannot open file");
}

template <typename T>
void file_source<T>::open(std::string const& path, int adv) 
{
    if (base::is_open() or base::ref_count != nullptr) throw std::runtime_error("[mm::file_sink] file already open");
    base::m_fd = ::open(path.c_str(), O_RDONLY);
    base::check_fd();
    struct stat fs;
    if (fstat(base::m_fd, &fs) == -1) throw std::runtime_error("[mm::file] cannot stat file");
    base::m_size = fs.st_size;
    base::m_data = mmap<T const*>(base::m_fd, base::m_size, PROT_READ);
    if (posix_madvise((void*)base::m_data, base::m_size, adv)) throw std::runtime_error("[mm::file] madvise failed");
    base::ref_count = new std::size_t;
    *base::ref_count = 1;
}

template <typename T>
void file_sink<T>::open(std::string const& path) 
{
    if (base::is_open() or base::ref_count != nullptr) throw std::runtime_error("[mm::file_sink (simple mode)] file already open");
    static const mode_t mode = 0600;  // read/write
    base::m_fd = ::open(path.c_str(), O_RDWR, mode);
    base::check_fd();
    struct stat fs;
    if (fstat(base::m_fd, &fs) == -1) throw std::runtime_error("[mm::file] cannot stat file");
    base::m_size = fs.st_size;
    base::m_data = mmap<T*>(base::m_fd, base::m_size, PROT_READ | PROT_WRITE);
    base::ref_count = new std::size_t;
    *base::ref_count = 1;
}

template <typename T>
void file_sink<T>::open(std::string const& path, size_t n) 
{
    if (base::is_open() or base::ref_count != nullptr) throw std::runtime_error("[mm::file_sink (create mode)] file already open");
    static const mode_t mode = 0600;  // read/write
    base::m_fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, mode);
    base::check_fd();
    base::m_size = n * sizeof(T);
    ftruncate(base::m_fd, base::m_size);  // truncate the file at the new size
    base::m_data = mmap<T*>(base::m_fd, base::m_size, PROT_READ | PROT_WRITE);
    base::ref_count = new std::size_t;
    *base::ref_count = 1;
}

} // namespace mm

#endif // MEMORY_MAPPED_FILE_HPP