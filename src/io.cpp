#include "../include/io.hpp"

namespace io {

loader::loader(std::istream& strm) : istrm(strm), num_bytes_pods(0), num_bytes_vecs_of_pods(0)
{
    if (!istrm.good()) throw std::runtime_error("[Loader] Unreadable input stream");
}

void loader::apply(std::string& s)
{
    auto nr = basic_load(istrm, s);
    num_bytes_vecs_of_pods += nr;
}

saver::saver(std::ostream& output_stream) : ostrm(output_stream)
{
    if (!ostrm.good()) throw std::runtime_error("[Saver] Unreadable input stream");
}

void saver::apply(std::string const& s) 
{
    basic_store(s, ostrm);
}

void mut_saver::apply(std::string& s) 
{
    basic_store(s, ostrm);
}

} // namespace io