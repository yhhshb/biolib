#include "../include/io.hpp"

namespace io {

loader::loader(std::istream& strm) : istrm(strm), num_bytes_pods(0), num_bytes_vecs_of_pods(0)
{
    if (!istrm.good()) throw std::runtime_error("[Loader] Unreadable input stream");
}

saver::saver(std::ostream& output_stream) : ostrm(output_stream)
{
    if (!ostrm.good()) throw std::runtime_error("[Saver] Unreadable input stream");
}

} // namespace io