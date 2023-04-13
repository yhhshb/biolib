#ifndef JACCARD_HPP
#define JACCARD_HPP

#include <tuple>

namespace algorithm {

template <typename Iterator1, typename Iterator2>
std::tuple<std::size_t, std::size_t, std::size_t, std::size_t> jaccard(Iterator1 start1, Iterator1 stop1, Iterator2 start2, Iterator2 stop2)
{
    std::size_t unione, intersection, size1, size2;
    unione = intersection = size1 = size2 = 0;
    while(start1 != stop1 or start2 != stop2)
	{
		if(start1 != stop1 and start2 != stop2 and *start1 == *start2)
		{
			unione += 1;
			intersection += 1;
            ++start1;
            ++start2;
            ++size1;
            ++size2;
		}
		else if (start1 != stop1 and (start2 == stop2 or *start1 < *start2))
		{
			unione += 1;
			++start1;
            ++size1;
		}
		else if (start2 != stop2 and (start1 == stop1 or *start2 < *start1))
		{
			unione += 1;
            ++start2;
            ++size2;
		}
	}
    return std::make_tuple(intersection, unione, size1, size2);
}

} // algorithm

#endif // JACCARD_HPP