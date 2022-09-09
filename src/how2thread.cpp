// test case how2thread

#include <thread>
#include "how2thread.hpp"
#include <iostream>
#include <algorithm>
// Scheduler implementation
namespace how2thread
{

// ******************************************************* //
// ********************** Scheduler ********************** //
// ******************************************************* //
Scheduler::Scheduler(const std::vector<char>& data,
                     const std::string& mask):
    mask(mask),
    slicer_num(Config_consts::slicer_no),
    data_arr(),
    data_arr_protect(),
    proc_instances_q(),
    slicer_instances_q(),
    result_arr(),
    result_protect(),
    char_segments_protect(slicer_num + 1),
    finish_line_no(slicer_num, 0),
    is_slicing(0)
{
    // left and right mutexes for algo uniform; 
    // actually not blocking
    std::queue<std::thread> thread_pool;
    is_slicing = slicer_num;
    for(size_t i = 0; i < slicer_num; ++i)
    {
        // spawn slicers
        slicer_instances_q.emplace(
                    std::ref(*this),
                    data.cbegin() + data.size() * i / slicer_num,
                    data.cbegin() + data.size() * (i + 1) / slicer_num,
                    data.cbegin(),
                    data.cend(),
                    std::ref(char_segments_protect[i]),
                    std::ref(char_segments_protect[i+1]),
                    i,
                    std::ref(finish_line_no[i]));
#ifdef DEBUG_V
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
#endif
        thread_pool.emplace(slicer_instances_q.back());
    }

    const size_t cores_amnt = std::thread::hardware_concurrency();
    for(size_t i = 0; i < cores_amnt - slicer_num - 1; ++i)
    {
        proc_instances_q.emplace();
        thread_pool.emplace(proc_instances_q.back(), std::ref(*this));
    } // now in tp first slicer_num items -- threads w/ slicers

    for(size_t i = 0; i < slicer_num; ++i)
    {
    // waiting for slicer threads
        thread_pool.front().join();
        thread_pool.pop();
        slicer_instances_q.pop();

        proc_instances_q.emplace();
        thread_pool.emplace(proc_instances_q.back(), std::ref(*this));
    //on freed threads launch processors
    }

    Actor_processor ap;
    ap(std::ref(*this)); // to not waste thread for waiting we'll launch processor there too

    while(!thread_pool.empty())
    {
        thread_pool.front().join();
        thread_pool.pop();
    }
    size_t sum = 0;
    // process line_no and sort findings by them
    std::for_each(finish_line_no.begin(), finish_line_no.end(),
        [sum](size_t& i) mutable 
        {
            i+=sum;
            sum+=i;
        });
    std::for_each(result_arr.begin(), result_arr.end(),
        [this](Finding& f) mutable
        {
            if(f.slicer_no > 0)
                f.line_no += this->finish_line_no[f.slicer_no - 1];
        });
    std::sort(result_arr.begin(), result_arr.end(),
        [](Finding& a, Finding& b){
            return a.line_no < b.line_no;
        });
}

Request Scheduler::get_request()
{
    auto m_lock = std::lock_guard<std::mutex>(data_arr_protect);
    if(!data_arr.empty())
    {
        auto ret_value = data_arr.front();
        data_arr.pop(); 
        return ret_value;
    }
    else
    {
        return {std::string(), 0, 0, false};
    }
}

void Scheduler::write_request(Request&& req)
{
    auto m_lock = std::lock_guard<std::mutex>(data_arr_protect);
    data_arr.push(req);
}

void Scheduler::write_finding(Finding&& find)
{
    auto m_lock = std::lock_guard<std::mutex>(result_protect);
    result_arr.push_back(find);
}

using Find_it = Scheduler::Find_it;
Find_it Scheduler::cbegin()
{
    return result_arr.cbegin();
}
Find_it Scheduler::cend()
{
    return result_arr.cend();
}
// ******************************************************* //
// ********************* Actor_slicer ******************** //
// ******************************************************* //
using It = Actor_slicer::It;
void Actor_slicer::operator() ( )
{
    // size_t line_start = 0;
    // implementation
    // we don't use stringstream since it copies base string
    // size_t end;
    auto new_begin = begin;
    { // finding left range
        auto m_lock = std::lock_guard<std::mutex>(m_left); 
        // locking left range for preventing data corruption
        for(; new_begin != begin_global; --new_begin)
        {
            if(*new_begin == '\n') break;
        }
        if(new_begin != begin_global) ++new_begin; // if we found line-break -- move right
    }

    auto new_end = end;
    { // finding right range
        auto m_lock = std::lock_guard<std::mutex>(m_right);
        // locking right range for preventing data corruption
        for(; new_end != end_global; --new_end)
        {
            if(*new_end == '\n') break;
        }
        if(new_begin != begin_global) --new_end;
    }
#ifdef DEBUG_V
    std::cout << slicer_no;
    std::flush(std::cout);
    for(auto i = new_begin; i < new_end; ++i)
    {
        std::cout << *i;
        std::flush(std::cout);
    }
    std::cout << std::endl;
    std::flush(std::cout);
#endif
    size_t line_no = 1;
    auto str_begin = new_begin;
    auto str_end = new_end;
    for(auto it = new_begin; it < new_end; ++it)
    {
        if(*it == '\n')
        {
            str_end = it - 1;
            sc.write_request({std::string(str_begin, str_end),
                              line_no++,
                              slicer_no,
                              true});
            if(it != new_end)
            {
                it++;
                str_begin = it;
            }
        }
    }
    sc.write_request({std::string(str_begin, new_end),
                      line_no,
                      slicer_no,
                      true}); // as we break b4 last string write, we must repeat operation
    finish_line_no = line_no;
    sc.is_slicing -= 1; // flag that this slicer finished his part
}

// ******************************************************* //
// ******************* Actor_processor ******************* //
// ******************************************************* //
Actor_processor::Actor_processor(){ };
Finding Actor_processor::parse_request(const Request& req,
                                     const std::string& mask)
{
    if(mask.size() > req.data.size())
        return {std::string(), 0, 0, 0, false};
    auto sliding_it_begin = req.data.cbegin();
    auto sliding_it_end = req.data.cbegin() + mask.size();
    for(;sliding_it_end != req.data.cend();)
    {
        auto tmp_mask_it = mask.cbegin();
        auto tmp_req_it = sliding_it_begin;
        for(;tmp_mask_it < mask.cend();)
        {
            if((*tmp_mask_it != *tmp_req_it) &&
               (*tmp_mask_it != '?')) break;
            tmp_mask_it++; tmp_req_it++;
        }
        if(tmp_mask_it == mask.cend())
        {
            return {std::string(sliding_it_begin, sliding_it_end),
                   req.line_no,
                   req.slicer_no,
                   (sliding_it_begin - req.data.begin()) + 1,
                   true};
        }
        sliding_it_end++; sliding_it_begin++;
    }
    return {std::string(), 0, 0, 0, false};
}

void Actor_processor::operator()(Scheduler& sc)
{
    using namespace std::chrono_literals;
    for(;;)
    {
        auto req = sc.get_request();
        // if we have no incoming requests and slicers finished work -- stop executing
        if(!req.fine)
        {
            if(sc.get_slicing_status())
            {
        // if slicers not finished -- wait a bit for them
                std::this_thread::sleep_for(100ms);
                continue;
            }
        // elsewise stop this thread
            else break;
        } 
        auto find = parse_request(req, sc.mask);
        if(find.fine) // if we have valid found -- write it in find pool
            sc.write_finding(std::move(find));
    }
}
}; // namespace how2thread