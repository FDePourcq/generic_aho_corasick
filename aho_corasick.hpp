/*
* Copyright (C) 2015 Christopher Gilbert.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef AHO_CORASICK_HPP
#define AHO_CORASICK_HPP

#include <algorithm>
#include <memory>
#include <set>
#include <queue>
#include <vector>
#include <functional>

#ifndef AHO_CORASICK_NOEXTRAS

#include <sstream>
#include <string>

#endif


namespace aho_corasick {

    template<class vector_type>
    const typename vector_type::value_type::second_type &getNoCreateOrderedUniqueKV(const vector_type &v, const typename vector_type::value_type::first_type &p) {
        auto it = std::lower_bound(v.begin(),
                                   v.end(),
                                   p,
                                   [](const typename vector_type::value_type &a,
                                      const typename vector_type::value_type::first_type &p) {
                                       return a.first < p;
                                   }); // find proper position in descending order
        if (it == v.end() || !(it->first == p)) {
            static typename vector_type::value_type::second_type local_null = typename vector_type::value_type::second_type();
            return local_null; //typename vector_type::value_type::second_type();
        }
        return it->second;
    }

    template<class vector_type>
    typename vector_type::value_type::second_type &getOrCreateOrderedUniqueKV(vector_type &v, const typename vector_type::value_type::first_type &p) {
        auto it = std::lower_bound(v.begin(),
                                   v.end(),
                                   p,
                                   [](const typename vector_type::value_type &a,
                                      const typename vector_type::value_type::first_type &p) {
                                       return a.first < p;
                                   }); // find proper position in descending order
        if (it == v.end() || !(it->first == p)) {
            it = v.insert(it, std::make_pair(p, typename vector_type::value_type::second_type()));
        }
        return it->second;
    }

    // class state
    template<typename string_type, typename value_type>
    class state {
    public:
        using CharType = typename string_type::value_type;
        typedef state<string_type, value_type> *ptr;
        typedef std::unique_ptr<state<string_type, value_type>> unique_ptr;
        typedef string_type &string_ref_type;
        typedef std::vector<ptr> state_collection;
        typedef std::vector<CharType> transition_collection;

        std::vector<std::pair<CharType, unique_ptr> > d_success;
        ptr d_failure;
        std::unique_ptr<value_type> payload; // every pattern gets only a single payload (value, whatever the pattern matches to), the submatches that end at the current position are failure-nodes of this node.
        std::size_t depth; // not needed for the actual algo.

        state(const std::size_t depth_ = 0) :
                d_success(),
                d_failure(nullptr),
                depth(depth_) {
        }

        ptr lookupchild(const CharType &character) const {
            return getNoCreateOrderedUniqueKV(d_success, character).get();
        }

        ptr next_state_no_failure(const CharType &c, ptr root) const {
            auto ret = lookupchild(c);
            if (ret == nullptr && this == root) {
                return root;
            }
            return ret;
        }

        bool set_value(const value_type &v) {
            if (payload.get() && *payload.get() == v) {
                return false;
            }
            payload.reset(new value_type(v));
            return true;
        }

        template<typename iteratortype, typename callbackfct>
        bool iterate_values(const iteratortype &pos,
                            const callbackfct &fct) const {

            if (payload.get()) {
                auto posbegin = pos; // todo: figure out how to do this the right way ...
                for (std::size_t i = 0; i + 1 < depth; ++i) {
                    --posbegin;
                }
                //	  std::advance(posbegin, -1 -(int)depth);
                auto posend = pos;
                ++posend;
                //	  std::advance(posend, 1);

                if (!fct(*payload, posbegin, posend)) {
                    return false;
                }
            }
            return d_failure ? d_failure->iterate_values(pos, fct) : true;
        }
    };

    template<typename string_type, typename value_type>
    class basic_trie {
    public:
        using CharType = typename string_type::value_type;

        typedef string_type &string_ref_type;
        typedef state<string_type, value_type> state_type;
        typedef state<string_type, value_type> *state_ptr_type;

    private:
        std::unique_ptr<state_type> d_root;
        bool d_constructed_failure_states;

    public:
        basic_trie() :
                d_root(new state_type()),
                d_constructed_failure_states(false) {
        }

        state_ptr_type add_state(state_ptr_type cur_state, const CharType &character) {
            auto &p = getOrCreateOrderedUniqueKV(cur_state->d_success, character);
            if (!p.get()) {
                p.reset(new state<string_type, value_type>(cur_state->depth + 1));
                d_constructed_failure_states = false;
            }
            return p.get();
        }

        template<typename iteratortype>
        void map(const iteratortype &begin, const iteratortype &end, const std::function<void(std::unique_ptr<value_type> &ptr)> &setter) {
            setter(getNodeOrCreate(begin, end)->payload);
        }

        template<typename iteratortype>
        bool map(const iteratortype &begin, const iteratortype &end, const value_type &value) {
            if (getNodeOrCreate(begin, end)->set_value(value)) {
                return true;
            }
            return false;
        }

        bool map(const string_type &keyword, const value_type &value) {
            return map(keyword.begin(), keyword.end(), value);
        }

        bool insert(const string_type &keyword) {
            return map(keyword, keyword);
        }

        template<typename iteratortype>
        state_ptr_type getNodeOrCreate(const iteratortype &begin, const iteratortype &end) {
            state_ptr_type cur_state = d_root.get();
            for (auto i = begin; i != end; ++i) {
                cur_state = add_state(cur_state, *i);
            }
            return cur_state;
        }

        state_ptr_type getNodeOrCreate(const string_type &s) {
            return getNodeOrCreate(s.begin(), s.end());
        }

        template<typename iteratortype>
        state_ptr_type getNodeNoCreate(const iteratortype &begin, const iteratortype &end) const {
            state_ptr_type cur_state = d_root.get();
            for (auto i = begin; i != end; ++i) {
                cur_state = cur_state->lookupchild(*i);
                if (!cur_state) {
                    return 0;
                }
            }
            return cur_state;
        }

        template<typename iteratortype>
        value_type *getNoCreate(const iteratortype &begin, const iteratortype &end) const { // use the trie as a ordinary map...
            state_ptr_type cur_state = getNodeNoCreate(begin, end);
            if (cur_state) {
                return cur_state->payload.get();
            }
            return 0;
        }

        value_type *getNoCreate(const string_type &s) const { // use the trie as a ordinary map...
            return getNoCreate(s.begin(), s.end());
        }

        template<typename iteratortype>
        value_type &getOrCreate(const iteratortype &begin, const iteratortype &end) { // useful when the value_type is a container.
            state_ptr_type cur_state = getNodeOrCreate(begin, end);
            if (!cur_state->payload) {
                cur_state->payload.reset(new value_type());
            }
            return *cur_state->payload;
        }

        value_type &getOrCreate(const string_type &s) { // useful when the value_type is a container.
            return getorCreate(s.begin(), s.end());
        }

        template<typename iteratortype>
        void erase(const iteratortype &begin, const iteratortype &end) const { // use the trie as a ordinary container...
            state_ptr_type cur_state = getNodeNoCreate(begin, end);
            if (cur_state) {
                cur_state->payload.reset(); // cant remove the node because i dont know (cheaply) how many other nodes use this one as fail/fallback (besides that it can have children too of course)
            }
        }

        void erase(const string_type &s) const { // use the trie as a ordinary container...
            forget(s.begin(), s.end());
        }

        struct BeginEndValue {
            const std::size_t begin;
            const std::size_t end;
            const value_type &v;

            bool operator<(const BeginEndValue &o) const {
                if (begin == o.begin) {
                    if (end == o.end) {
                        return v < o.v;
                    } else {
                        return end < o.end;
                    }
                } else {
                    return begin < o.begin;
                }
            }

            bool operator==(const BeginEndValue &o) const {
                return begin == o.begin && end == o.end && v == o.v;
            }
        };

        std::vector<BeginEndValue> collect_matches(const string_type &v) {
            return collect_matches(v.begin(), v.end());
        }

        template<typename iteratortype>
        std::vector<BeginEndValue> collect_matches(const iteratortype &begin, const iteratortype &end) {
            check_construct_failure_states();
            size_t pos = 0;
            state_ptr_type cur_state = d_root.get();
            std::vector<BeginEndValue> hits;
            for (auto i = begin; i != end; ++i) {
                cur_state = get_state(cur_state, *i);
                cur_state->iterate_values(pos, [&hits](const value_type &v, const std::size_t &b, const std::size_t &e) {
                    hits.push_back(BeginEndValue{b, e, v});
                    return true;
                });
                pos++;
            }
            return hits;
        }

        template<typename iteratortype, typename callbackfct>
        void iterate_matches(
                const iteratortype &begin,
                const iteratortype &end,
                const callbackfct &fct) { //std::function<bool(const value_type &, const iteratortype&, const iteratortype&)>
            check_construct_failure_states();
            state_ptr_type cur_state = d_root.get();
            for (auto i = begin; i != end; ++i) {
                cur_state = get_state(cur_state, *i);
                if (!cur_state->iterate_values(i, fct)) {
                    return;
                }
            }
        }

        template<typename callbackfct>
        void iterate_matches(const string_type &s,
                             const callbackfct &fct) {
            iterate_matches(s.begin(), s.end(), fct);
        }

        state_ptr_type get_state(state_ptr_type cur_state, CharType c) const {
            state_ptr_type result = cur_state->next_state_no_failure(c, d_root.get());
            while (result == nullptr) {
                cur_state = cur_state->d_failure;
                result = cur_state->next_state_no_failure(c, d_root.get());
            }
            return result;
        }

        void check_construct_failure_states() {
            if (!d_constructed_failure_states) {
                construct_failure_states();
            }
        }

        void construct_failure_states() {
            std::queue<state_ptr_type> q; // -> breadth-first iterative deepening...
            // root has no fail
            for (const auto &char_depth_one_state : d_root->d_success) {
                char_depth_one_state.second->d_failure = d_root.get();
                q.push(char_depth_one_state.second.get());
            }
            d_constructed_failure_states = true;

            while (!q.empty()) {
                auto cur_state = q.front();
                q.pop();
                for (const auto &char_child : cur_state->d_success) {
                    state_ptr_type target_state = char_child.second.get(); //cur_state->next_state(transition);
                    q.push(target_state);

                    state_ptr_type trace_failure_state = cur_state->d_failure;
                    while (trace_failure_state->next_state_no_failure(char_child.first, d_root.get()) == nullptr) {
                        trace_failure_state = trace_failure_state->d_failure;
                    }
                    target_state->d_failure = trace_failure_state->next_state_no_failure(char_child.first, d_root.get());
                }

            }
        }

#ifndef AHO_CORASICK_NOEXTRAS

        std::string toDot() const {
            std::ostringstream oss;
            oss << "digraph 123 { graph [rankdir=LR]; node [shape=box] " << std::endl;
            std::queue<state_ptr_type> q;
            q.push(d_root.get());
            while (!q.empty()) {
                auto cur_state = q.front();
                q.pop();


                oss << (std::size_t) cur_state << " [label=\"";
                cur_state->iterate_values((std::size_t) 0, [&oss](const value_type &v, const std::size_t &, const std::size_t &) {
                    oss << v << "\\" << "r";
                    return true;
                });
                oss << "\"];" << std::endl;
                for (const auto &char_child : cur_state->d_success) {
                    state_ptr_type target_state = char_child.second.get();// cur_state->next(transition,d_root);
                    q.push(target_state);
                    oss << (std::size_t) cur_state << " -> " << (std::size_t) target_state << " [label=\"" << char_child.first << "\",color=green]" << std::endl;
                }
                if (cur_state->d_failure && cur_state->d_failure != d_root.get()) {
                    oss << (std::size_t) cur_state << " -> " << (std::size_t) cur_state->d_failure << " [color=red,constraint=false];" << std::endl;
                }
            }

            oss << "}" << std::endl;
            return oss.str();
        }

#endif

    };

#ifndef AHO_CORASICK_NOEXTRAS
    typedef basic_trie<std::basic_string<char>, std::basic_string<char> > trie;
    typedef basic_trie<std::basic_string<wchar_t>, std::basic_string<wchar_t> > wtrie;
#endif

} // namespace aho_corasick

#endif // AHO_CORASICK_HPP
