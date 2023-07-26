#include <hpda/common/common.h>
#include <hpda/engine/engine.h>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>

constexpr static uint32_t status_null = 0;
constexpr static uint32_t status_to_p = 1;
constexpr static uint32_t status_p = 2;
constexpr static uint32_t status_end = 3;


namespace hpda
{

    void engine::add_functor(functor *f)
    {
        m_functors.push_back(f);
    }
    void engine::remove_functor(functor *f)
    {

        bool found = true;
        while (found)
        {
            found = false;
            for (std::vector<functor *>::const_iterator it = m_functors.begin();
                 it != m_functors.end(); it++)
            {
                if (*it == f)
                {
                    m_functors.erase(it);
                    found = true;
                    break;
                }
            }
        }
    }
    void engine::run()
    {
        build_graph();
        auto outputs = find_outputs();
        if (outputs.empty())
        {
            throw std::runtime_error("cannot find output");
        }
        std::queue<functor *> to_process;
        std::stack<functor *> processing;

        for (auto output : outputs)
        {
            output->m_status = status_to_p;
            to_process.push(output);
        }

        //int i = 0;// IDEA: not used, can delete
        // 1. BFS
        while (!to_process.empty())
        {
            do
            {
                auto f = to_process.front();
                to_process.pop();
                if (f->predecessors().empty())
                {
                    // it's an extractor
                    // IDEA: extractor can also be pushed to stack, no need to process here.
                    
                    /*
                    f->m_status = status_null;
                    bool v = f->process();
                    if (v)
                    {
                        f->done_value();
                    }
                    else
                    {
                        m_reach_ends.insert(f);
                    }
                    */
                }
                else
                {
                }
                f->m_status = status_p; 
                processing.push(f);
                for (auto pre : f->predecessors())
                {
                    // IDEA: only push f->m_status not ended.
                    // utilize the end information here.
                    // add check for m_status == end
                    // also add m_status init state.
                    if (!pre->has_value() && pre->m_status != status_to_p && pre->m_status != status_end)
                    {
                        to_process.push(pre);
                        pre->m_status = status_to_p;
                    }
                }
            } while (!to_process.empty());
            // 2. pop stack
            while (!processing.empty())
            {
                auto f = processing.top();
                
                /*
                if (!functor_has_input(f)) 
                // where is f.process called?
                // "to end" f get break here and execute in the next run's BFS stage
                {
                    // IDEA: also add this to end state
                    // so this need to move to f->process() failed case
                    break;
                }
                */

                if (f->process())
                {
                    f->done_value();
                    processing.pop();
                    f->m_status = status_null;
                } else {
                    f->reset_done_value();
                    bool end = true;
                    for (auto pre : f->predecessors())
                    {
                        if (!pre->has_value() && pre->m_status != status_to_p && pre->m_status != status_end)
                        {
                            to_process.push(pre); // ISSUE: pre 可能已经在队列中
                            pre->m_status = status_to_p;
                            end = false;
                        }
                    }
                    if (end)
                    {
                        f->m_status = status_end;
                        processing.pop();
                        continue; // ending process
                    }
                    break; //should go to top not
                }
            }
            if (to_process.empty())
            {
                for (auto output : outputs)
                {
                    if (output->m_status != status_end)
                    {
                        to_process.push(output);
                        output->m_status = status_to_p;
                    }
                }
            }
        }
    }

    // We assume there is no circle, it's a DAG
    void engine::build_graph()
    {
        m_predecessors.clear();
        m_successors.clear();
        m_reach_ends.clear();
        for (auto f : m_functors)
        {
            if (m_predecessors.find(f) == m_predecessors.end())
            {
                m_predecessors.insert(std::make_pair(f, std::unordered_set<functor *>()));
            }
            if (m_successors.find(f) == m_successors.end())
            {
                m_successors.insert(std::make_pair(f, std::unordered_set<functor *>()));
            }
            for (auto input : f->predecessors())
            {
                if (m_successors.find(input) == m_successors.end())
                {
                    m_successors.insert(
                        std::make_pair(input, std::unordered_set<functor *>()));
                }
                m_successors[input].insert(f);
                m_predecessors[f].insert(input);
            }
            f->m_status = status_null;
        }
    }
    std::vector<functor *> engine::find_outputs() const
    {
        std::vector<functor *> ret;
        for (auto it : m_successors)
        {
            if (it.second.empty())
            {
                ret.push_back(it.first);
            }
        }
        return ret;
    }

    bool engine::contain_same_successor(functor *f1, functor *f2,
                                        functor *succ) const
    {
        auto s1 = m_successors.find(f1)->second;
        auto s2 = m_successors.find(f2)->second;
        if (s1.find(succ) != s1.end() && s2.find(succ) != s2.end())
        {
            return true;
        }
        return false;
    }

    bool engine::functor_has_input(functor *f) const
    {
        const std::vector<functor *> &inputs = f->predecessors();
        bool ret = true;
        for (auto input : inputs)
        {
            if (m_reach_ends.find(input) != m_reach_ends.end())
            {
                continue;
            }
            ret = ret && input->has_value();
        }
        return ret;
    }

    bool engine::is_output(functor *f) const
    {
        return m_successors.find(f)->second.empty();
    }
} // namespace hpda
