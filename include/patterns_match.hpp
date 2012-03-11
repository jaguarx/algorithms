/* Copyright (C) 2012 xiong.jaguar@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef PATTERNS_MATCH_HPP_
#define PATTERNS_MATCH_HPP_

#include <set>
#include <map>
#include <iostream>
#include <vector>
#include <queue>
#include <list>

//implementation of Aho-Corasick Algorithm

class patterns_matcher {
public:
    typedef int state_idx_t;
    struct trie_key_t {
        trie_key_t(state_idx_t s, char v) :
                state(s), value(v) {
        }
        state_idx_t state;
        char value;
        bool operator<(const trie_key_t& rhs) const {
            if (state == rhs.state)
                return value < rhs.value;
            return state < rhs.state;
        }
    };
    typedef std::map<trie_key_t, state_idx_t> trie_t;
    struct state_t {
        state_t() {
            fail_state = 0;
        }
        state_idx_t fail_state;
        std::set<void*> matched;
    };
    typedef std::vector<state_t> states_t;

    patterns_matcher() {
    }
    patterns_matcher(const char* patterns[], void* values[]) {
        build(patterns, values);
    }

    void build(const char* patterns[], void* values[]) {
        trie_t::iterator itr;
        _states.resize(1); // slot for root state
        _trie.clear();
        for (int i = 0; patterns[i] != NULL; ++i) {
            state_idx_t state = 0;
            const char* pattern = patterns[i];
            for (int j = 0; pattern[j]; ++j) {
                trie_key_t k(state, pattern[j]);
                itr = _trie.find(k);
                if (_trie.end() == itr) {
                    state = _states.size();
                    _states.push_back(state_t());
                    _trie[k] = state;
                } else
                    state = itr->second;
            }
            _states[state].matched.insert(values[i]);
        }
        //build fail links
        _fail(ROOT_STATE, ROOT_STATE);
        std::queue<state_idx_t> que;
        for (itr = _trie.begin();
                itr != _trie.end() && itr->first.state == ROOT_STATE; ++itr) {
            state_idx_t q = itr->second;
            _fail(q, ROOT_STATE);
            que.push(q);
        }
        while (!que.empty()) {
            state_idx_t r = que.front();
            que.pop();
            itr = _trie.lower_bound(trie_key_t(r, 0));
            for (; itr != _trie.end() && itr->first.state == r; ++itr) {
                state_idx_t u = itr->second;
                char a = itr->first.value;
                que.push(u);
                state_idx_t v = _fail(r);
                state_idx_t f = 0;
                while (v != 0 && (INVALID_STATE == (f = _goto(v, a))))
                    v = _fail(v);
                f = _goto(v, a);
                if (f == INVALID_STATE)
                    f = ROOT_STATE;
                _fail(u, f);
                _states[u].matched.insert(_states[f].matched.begin(),
                        _states[f].matched.end());
            }
        }
    }
    void debug_dump(std::ostream& oss) const {
        trie_t::const_iterator i1 = _trie.begin();
        for (; i1 != _trie.end(); ++i1)
            dump_trie_node(oss, *i1) << "\n";
        for (int i2 = 0; i2 < _states.size(); ++i2) {
            oss << i2 << ":{f:" << _states[i2].fail_state << ",t:"
                    << _states[i2].matched.size() << "}\n";
        }
    }
    class match_result {
    public:
        match_result(const trie_t& trie, const states_t& states,
                const char* text) :
                _trie(trie), _states(states), _text(text) {
            _current_state = ROOT_STATE;
        }
        bool next() {
            state_idx_t& q = _current_state;
            for (; *_text != 0; ++_text) {
                trie_t::const_iterator i = _trie.find(trie_key_t(q, *_text));
                while (i == _trie.end() && _states[q].fail_state != q) {
                    q = _states[q].fail_state;
                    i = _trie.find(trie_key_t(q, *_text));
                }
                if (i != _trie.end()) {
                    //++_text;
                    q = i->second;
                    if (_states[q].matched.size() > 0) {
                        ++_text;
                        return true;
                    }
                }
            }
            return false;
        }
        const std::set<void*>& matches() const {
            return _states[_current_state].matched;
        }
        const char* text() const {
            return _text;
        }
    private:
        const trie_t& _trie;
        const states_t& _states;
        state_idx_t _current_state;
        const char* _text;
    };
    match_result match(const char* text) const {
        return match_result(_trie, _states, text);
    }
private:
    enum state_idx_enum {
        INVALID_STATE = -1, ROOT_STATE = 0
    };
    static std::ostream& dump_trie_node(std::ostream& oss,
            const std::pair<const trie_key_t, state_idx_t>& node) {
        return oss << "{s:" << node.first.state << ",v:'" << node.first.value
                << "'}:" << node.second;
    }

    trie_t _trie;
    states_t _states;

    state_idx_t _fail(state_idx_t q) {
        state_idx_t retval = ROOT_STATE;
        if (q < _states.size())
            retval = _states[q].fail_state;
        return retval;
    }
    void _fail(state_idx_t q1, state_idx_t q2) {
        if (q1 < 0)
            return;
        if (q1 < _states.size())
            _states[q1].fail_state = q2;
    }
    state_idx_t _goto(state_idx_t q, char v) {
        trie_key_t k(q, v);
        trie_t::const_iterator i = _trie.find(k);
        state_idx_t retval = INVALID_STATE;
        if (i != _trie.end())
            retval = i->second;
        return retval;
    }
};

#endif /* PATTERNS_MATCH_HPP_ */
