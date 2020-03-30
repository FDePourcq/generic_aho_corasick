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
#include <map>
#include <memory>
#include <set>
#include <queue>
#include <vector>


#ifndef AHO_CORASICK_NOEXTRAS

#include <sstream>
#include <string>

#endif


namespace aho_corasick {

    template<class vector_type>
    void insertOrderedUnique(vector_type &v, const typename vector_type::value_type &p) {
        auto it = std::lower_bound(v.begin(), v.end(), p); // find proper position in descending order
        if (it == v.end() || *it != p) {
            v.insert(it, p);
        }
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

        std::map<CharType, unique_ptr> d_success;
        ptr d_failure;
        std::vector<value_type> values; // the values given by the user where the patterns map to. (presumably the user can use them to identify which pattern matched and more.)

        state() :
                d_success(),
                d_failure(nullptr) {
        }

        ptr lookupchild(const CharType &character, ptr fallback = nullptr) const {
            auto i = d_success.find(character);
            if (i != d_success.end())
                return i->second.get();
            return fallback;
        }

        ptr next_state_no_failure(const CharType &c, ptr root) const {
            auto ret = lookupchild(c);
            if (ret == nullptr && this == root) {
                return root;
            }
            return ret;
        }

        ptr add_state(const CharType &character) {
            auto n = lookupchild(character);
            if (!n) {
                n = new state<string_type, value_type>();
                d_success[character].reset(n);
            }
            return n;
        }

        void add_value(const value_type &keyword) {
            insertOrderedUnique(values, keyword);
        }

        void collect_values(std::set<value_type> &c) const {
            if (d_failure) {
                d_failure->collect_values(c);
            }
            c.insert(values.begin(), values.end());
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

        void map(const string_type &keyword, const value_type &value) {
            if (keyword.empty())
                return;
            state_ptr_type cur_state = d_root.get();
            for (const auto &ch : keyword) {
                cur_state = cur_state->add_state(ch);
            }
            cur_state->add_value(value);
            d_constructed_failure_states = false;
        }

        void insert(const string_type &keyword) {
            if (keyword.empty())
                return;
            state_ptr_type cur_state = d_root.get();
            for (const auto &ch : keyword) {
                cur_state = cur_state->add_state(ch);
            }
            cur_state->add_value(keyword);
            d_constructed_failure_states = false;
        }

        template<class InputIterator>
        void insert(const InputIterator &first, const InputIterator &last) {
            for (InputIterator it = first; first != last; ++it) {
                insert(*it);
            }
        }

        std::vector<std::pair<value_type, std::size_t> > parse_text(const string_type &v) {
            return parse_text(v.begin(), v.end());
        }

        template<typename iteratortype>
        std::vector<std::pair<value_type, std::size_t> > parse_text(const iteratortype &begin, const iteratortype &end) {
            check_construct_failure_states();
            size_t pos = 0;
            state_ptr_type cur_state = d_root.get();
            std::vector<std::pair<value_type, std::size_t> > hits;
            for (auto i = begin; i != end; i++) {
                cur_state = get_state(cur_state, *i);
                std::set<value_type> values;
                cur_state->collect_values(values);
                for (const auto &i : values) {
                    hits.push_back(std::make_pair(i, pos));
                }
                pos++;
            }
            return hits;
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
                        //assert(trace_failure_state != d_root.get());
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
                std::set<value_type> values;
                cur_state->collect_values(values);
                for (auto &i : values) {
                    oss << i << "\\" << "r";
                }
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