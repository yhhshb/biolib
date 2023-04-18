#ifndef RANK_SELECT_HPP
#define RANK_SELECT_HPP

#include <vector>

namespace bit {

template <typename BitVector>
class rank_select
{
    public:
        rank_select(BitVector&& vector);
        std::size_t rank1() const;
        std::size_t rank0() const;
        std::size_t select1() const;
        std::size_t select0() const;

        template <class Visitor>
        void visit(Visitor& visitor);

    private:
        // index blocks
        const BitVector _data;
};

template <typename BitVector>
template <class Visitor>
void rank_select<BitVector>::visit(Visitor& visitor)
{
    visitor.apply(_data);
}

} // namespace bit

#endif // RANK_SELECT_HPP