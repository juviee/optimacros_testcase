// header for test case
// how2thread
#ifndef HOW_2_THREAD
#define HOW_2_THREAD

#include <atomic>
#include <string>
#include <mutex>
#include <queue>
#include <iterator>
namespace how2thread
{
struct Config_consts
{
    static constexpr size_t slicer_no = 2;
    // configure that corresponding your CPU
    // (do not reserve all threads)
};

struct Request
{
    std::string data;
    size_t line_no; // line no from local beginning
    size_t slicer_no; // used for calculating global line no
    bool fine;
};

struct Finding
{
    std::string data;
    size_t line_no; // line no from global beginning
    size_t slicer_no; // used for calculating global line no
    size_t line_pos; // pos in line
    bool fine;
};

class Actor_processor;
class Actor_slicer;
class Scheduler
{
// class for schedule slicers, processors and collecting data
public:
    Scheduler(const std::vector<char>& data,
              const std::string& mask); // construct and parse data
    Request get_request(); // used to get another batch
    void write_request(Request&&);

    void write_finding(Finding&& find);

    using Find_it = decltype(std::declval<std::vector<Finding>&>().cbegin());
    Find_it cbegin();
    Find_it cend();

    const auto& get_slicing_status(){return is_slicing;} //
    std::string mask;
#ifndef DEBUG_V
private:
#endif
    const size_t slicer_num;
    std::queue<Request> data_arr; // consider switching to deQ
    std::mutex data_arr_protect;//for mt rw to vec // might be switching to semaphores

    std::queue<Actor_processor> proc_instances_q;
    std::queue<Actor_slicer> slicer_instances_q;

    std::vector<Finding> result_arr;
    std::mutex result_protect;//for mt write to vec

    // between 2 slicers overlapping area with possible rc
    // char_segments_protect.size() == slicer_no + 1
protected:
    friend class Actor_slicer; // we need access from slicers to mutexes as they're pretty lowlvl
    std::vector<std::mutex> char_segments_protect;
    std::vector<size_t> finish_line_no;
    std::atomic<size_t> is_slicing; // for break cond 
};

class Actor_processor
{
// class for processing batches (line and line no)
public:
    Actor_processor();
    void operator() (Scheduler&);
//private:
    Finding parse_request(const Request&, const std::string& mask);
};

class Actor_slicer
{
public:
    using It = decltype(std::declval<const std::vector<char>&>().cbegin());
private:
    Scheduler& sc;
    It begin;
    It end;
    It begin_global;
    It end_global;
    std::mutex& m_left;
    std::mutex& m_right;
    size_t slicer_no;
    size_t& finish_line_no;
public:
    Actor_slicer(
            Scheduler& sc_,
            It begin_,
            It end_,
            It begin_global_,
            It end_global_,
            std::mutex& m_left_,
            std::mutex& m_right_,
            size_t slicer_no_,
            size_t& finish_line_no_
    ):
        sc(sc_),
        begin(begin_),
        end(end_),
        begin_global(begin_global_),
        end_global(end_global_),
        m_left(m_left_),
        m_right(m_right_),
        slicer_no(slicer_no_),
        finish_line_no(finish_line_no_)
    {};
    void operator()();
};

};// namespace how2thread

#endif