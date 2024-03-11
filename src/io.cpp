#include "../include/io.hpp"

namespace io {

[[maybe_unused]] std::size_t basic_parse(uint8_t const* istrm, std::string& s)
{
    auto start = istrm;
    std::size_t n;
    istrm += basic_parse(istrm, n);
    s.resize(n);
    for (auto& c : s) istrm += basic_parse(istrm, c);
    return istrm - start;
}

[[maybe_unused]] std::size_t basic_load(std::istream& istrm, std::string& s)
{
    std::size_t n;
    basic_load(istrm, n);
    s.resize(n);
    std::size_t bytes_read = sizeof(n);
    for (auto& c : s) bytes_read += basic_load(istrm, c);
    return bytes_read;
}

[[maybe_unused]] std::size_t basic_store(std::string const& s, std::ostream& ostrm)
{
    std::size_t n = s.size();
    std::size_t bytes_written = basic_store(n, ostrm);
    for (auto const& c : s) bytes_written += basic_store(c, ostrm);
    return bytes_written;
}

loader::loader(std::istream& strm) : istrm(strm) //, num_bytes_pods(0), num_bytes_vecs_of_pods(0)
{
    if (!istrm.good()) throw std::runtime_error("[Loader] Unreadable input stream");
}

void loader::visit(std::string& s)
{
    // auto nr = 
    basic_load(istrm, s);
    // num_bytes_vecs_of_pods += nr;
}

saver::saver(std::ostream& output_stream) : ostrm(output_stream) //, num_bytes_pods(0), num_bytes_vecs_of_pods(0)
{
    if (!ostrm.good()) throw std::runtime_error("[Saver] Unreadable input stream");
}

void saver::visit(std::string const& s) 
{
    // auto nr = 
    basic_store(s, ostrm);
    // num_bytes_vecs_of_pods += nr;
}

mut_saver::mut_saver(std::ostream& output_stream) : saver(output_stream)
{
    if (!ostrm.good()) throw std::runtime_error("[Saver] Unreadable input stream");
}

void mut_saver::visit(std::string& s) 
{
    // auto nr = 
    basic_store(s, ostrm);
    // num_bytes_vecs_of_pods += nr;
}

} // namespace io