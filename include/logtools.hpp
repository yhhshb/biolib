#ifndef LOGTOOLS_HPP
#define LOGTOOLS_HPP

#include <cstddef>
#include <vector>
#include <string>
#include <chrono>

// #include <iostream>

namespace logging_tools {

template <typename T>
static std::size_t basic_size_measure([[maybe_unused]] T const& val)
{
    static_assert(std::is_fundamental<T>::value);
    if constexpr (std::is_array<T>::value) throw std::domain_error("[Function load] C arrays are not supported");
    return sizeof(T);
}

template <typename T, typename Allocator>
static std::size_t basic_size_measure(std::vector<T, Allocator> const& vec)
{
    auto tmp = vec.size();
    std::size_t bytes_read = sizeof(decltype(tmp));
    for (auto& v : vec) bytes_read += basic_size_measure(v);
    return bytes_read;
}

class libra
{
    public:
        libra();

        template <typename T>
        void apply(T const& var) noexcept;

        template <typename T, class Allocator>
        void apply(std::vector<T, Allocator> const& vec) noexcept;

        std::size_t get_byte_size() const noexcept;

    private:
        std::size_t acc;
};

template <typename T>
void libra::apply(T const& var) noexcept
{
    if constexpr (std::is_fundamental<T>::value) {
        auto nr = basic_size_measure(var);
        acc += nr;
    } else {
        var.visit(*this); // supposing the to-be measured object has a visit() method
    }
}

template <typename T, typename Allocator>
void libra::apply(std::vector<T, Allocator> const& vec) noexcept
{
    if constexpr (std::is_fundamental<T>::value) {
        auto nr = basic_size_measure(vec);
        acc += nr;
    } else {
        auto n = vec.size();
        apply(n);
        for (auto const& v : vec) apply(v); // Call apply(), not load() since we want to recursively count the number of bytes
    }
}

template <class ClockType, typename MeasurementType>
class timer
{
    public:
        timer();
        timer(std::size_t queue_size); // queue_size = 0 means unbounded
        void start() noexcept;
        std::size_t stop(bool keep = true);
        std::size_t get_last_measurement() const;
        std::size_t get_cumulative_measurement() const noexcept;
        std::size_t min() const noexcept;
        std::size_t max() const noexcept;
        std::size_t size() const noexcept;
        void resize(std::size_t queue_size);
        void clear() noexcept;
        std::vector<std::size_t> const& data() const noexcept;
    
    private:
        std::vector<std::size_t> measurements;
        std::size_t head, tail; // circular buffer index
        std::size_t cumulative_sum;
        typename ClockType::time_point pstart;
        bool started;

        void insert(MeasurementType elapsed) noexcept;
        std::size_t back_n_idx(std::size_t n) const noexcept;
        // std::size_t back_idx() const noexcept;
        #ifndef NDEBUG
        std::size_t stupid_sum() const noexcept; // for debug purposes
        #endif
};

typedef timer<std::chrono::high_resolution_clock, std::chrono::microseconds> micro_timer;

template <class ClockType, typename MeasurementType>
timer<ClockType, MeasurementType>::timer()
    : head(1), tail(0), cumulative_sum(0), started(false)
{
    measurements.resize(1);
    measurements.shrink_to_fit();
}

template <class ClockType, typename MeasurementType>
timer<ClockType, MeasurementType>::timer(std::size_t queue_size)
    : head(queue_size), tail(0), cumulative_sum(0), started(false)
{
    measurements.resize(queue_size);
    measurements.shrink_to_fit();
}

template <class ClockType, typename MeasurementType>
void
timer<ClockType, MeasurementType>::start() noexcept
{
    started = true;
    pstart = ClockType::now();
}

template <class ClockType, typename MeasurementType>
std::size_t
timer<ClockType, MeasurementType>::stop(bool keep)
{
    if (not started) throw std::runtime_error("[Timer] timer was not started");
    auto pstop = ClockType::now();
    auto elapsed = std::chrono::duration_cast<MeasurementType>(pstop - pstart);
    started = false;
    if (keep) insert(elapsed);
    return elapsed.count();
}

template <class ClockType, typename MeasurementType>
std::size_t 
timer<ClockType, MeasurementType>::get_last_measurement() const
{
    if (head == measurements.size()) std::runtime_error("[Timer] empty timer");
    return measurements.at(back_n_idx(1));
}

template <class ClockType, typename MeasurementType>
std::size_t 
timer<ClockType, MeasurementType>::get_cumulative_measurement() const noexcept
{
#ifndef NDEBUG
    auto sum = stupid_sum();
    // std::cerr << "[[" << cumulative_sum << "]] | [[" << sum << "]]\n";
    assert(sum == cumulative_sum);
#endif
    return cumulative_sum;
}

template <class ClockType, typename MeasurementType>
std::size_t 
timer<ClockType, MeasurementType>::min() const noexcept
{
    return *std::max_element(measurements.cbegin(), measurements.cend());
}

template <class ClockType, typename MeasurementType>
std::size_t 
timer<ClockType, MeasurementType>::max() const noexcept
{
    return *std::max_element(measurements.cbegin(), measurements.cend());
}

template <class ClockType, typename MeasurementType>
std::size_t 
timer<ClockType, MeasurementType>::size() const noexcept
{
    if (head < measurements.size()) {
        if (head < tail) return tail - head;
        return measurements.size() - (head - tail);
    }
    return 0;
}

template <class ClockType, typename MeasurementType>
void 
timer<ClockType, MeasurementType>::resize(std::size_t queue_size)
{
    if (head < measurements.size()) {
        std::vector<std::size_t> buffer;
        buffer.resize(queue_size);
        std::size_t idx = back_n_idx(queue_size);
        if (queue_size >= size()) idx = head;
        else idx = back_n_idx(queue_size);
        cumulative_sum = 0;
        for(std::size_t buffer_idx = 0; buffer_idx < queue_size; ++buffer_idx) {
            idx %= measurements.size();
            buffer[buffer_idx] = measurements.at(idx);
            cumulative_sum += measurements.at(idx);
            ++idx;
        }
        measurements.swap(buffer);
    } else {
        measurements.resize(queue_size);
    }
    measurements.shrink_to_fit();
}

template <class ClockType, typename MeasurementType>
void 
timer<ClockType, MeasurementType>::clear() noexcept
{
    head = measurements.size();
    tail = 0;
    cumulative_sum = 0;
    started = false;
}

template <class ClockType, typename MeasurementType>
std::vector<std::size_t> const& 
timer<ClockType, MeasurementType>::data() const noexcept
{
    return measurements;
}

template <class ClockType, typename MeasurementType>
void 
timer<ClockType, MeasurementType>::insert(MeasurementType elapsed) noexcept
{
    if (head == tail) {
        // std::cerr << "removing " << measurements.at(head) << " from cs = " << cumulative_sum << "\n";
        cumulative_sum -= measurements.at(head);
        ++head;
    }
    if (head >= measurements.size()) head = 0;
    // std::cerr << "adding " << elapsed.count() << " to cs = " << cumulative_sum << "\n";
    cumulative_sum += elapsed.count();
    measurements[tail++] = elapsed.count();
    tail %= measurements.size();
}

template <class ClockType, typename MeasurementType>
std::size_t
timer<ClockType, MeasurementType>::back_n_idx(std::size_t n) const noexcept
{
    assert(head < measurements.size());
    auto aldy_head = (tail >= n) ? tail - n : measurements.size() - (n - tail);
    return (n > size()) ? head : aldy_head;
}

#ifndef NDEBUG
template <class ClockType, typename MeasurementType>
std::size_t 
timer<ClockType, MeasurementType>::stupid_sum() const noexcept
{
    std::size_t s = 0;
    if (head < measurements.size()) {
        std::size_t i = head;
        // std::cerr << "(" << head << ", " << tail << ")\n";
        do {
            // i %= measurements.size();
            s += measurements.at(i);
        } while ((i = (i + 1) % measurements.size()) != tail);
    }
    return s;
}
#endif

// template <class ClockType, typename MeasurementType>
// std::size_t
// timer<ClockType, MeasurementType>::back_idx() const noexcept
// {
//     // return (tail == 0) ? measurements.size() - 1 : tail - 1;
//     return back_n_idx(1);
// }

} // namespace logging_tools

#endif