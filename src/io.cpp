#include "../include/io.hpp"

namespace io {

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