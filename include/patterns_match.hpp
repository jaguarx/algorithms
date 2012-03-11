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
#include <cstddef>

//implementation of Aho-Corasick Algorithm

typedef long state_idx_t;
struct jump_entry_t {
    state_idx_t src;
    char val;
    state_idx_t dst;
};
struct matched_output {
    matched_output () { count = 0; patterns = NULL;}
    int count;
    int* patterns;
};
struct state_entry_t {
    state_entry_t(){ fail_link = 0; jump_tbl_begin = -1;}
    state_idx_t fail_link;
    long jump_tbl_begin;
    long jump_tbl_end;
    matched_output output;
};

class automaton_matcher{
public:
    automaton_matcher(const jump_entry_t*jmp_tbl, int jump_tbl_size, const state_entry_t* sta_tbl):
        _jump_tbl(jmp_tbl),_jump_tbl_size(jump_tbl_size),_state_tbl(sta_tbl) { _curr = 0; }

    void match(const char* text){ _text = text; _curr = 0;}
    bool next(const char* t = NULL) {
        if (t != NULL) _text = t;
        state_idx_t& q = _curr;
        for (; *_text != 0; ++_text) {
            long i = _find(q, *_text);
            while (_NOT_FOUND == i && _state_tbl[q].fail_link != q) {
                q = _state_tbl[q].fail_link;
                i = _find(q, *_text);
            }
            if (_NOT_FOUND != i ) {
                q = _jump_tbl[i].dst;
                if (_state_tbl[q].output.count > 0) {
                    ++_text;
                    return true;
                }
            }
        }
        return false;
    }
    const matched_output& matches() const {
        return _state_tbl[_curr].output;
    }
    const char* text() const {
        return _text;
    }
private:
    enum { _NOT_FOUND = -1 };
    // find value (v), return the jump table index
    int _find(state_idx_t q, char v){
        int s = _state_tbl[q].jump_tbl_begin;
        int e = _state_tbl[q].jump_tbl_end;
        while(s + 1 < e) {
            int i = (s+e)/2;
            if (_jump_tbl[i].val == v) return i;
            if (_jump_tbl[i].val < v)
                s = (s+e)/2;
            else
                e = (s+e)/2;
        }
        if (_jump_tbl[s].val == v) return s;
        return _NOT_FOUND;
        //return _NOT_FOUND;
    }
    const jump_entry_t* _jump_tbl;
    int _jump_tbl_size;
    const state_entry_t* _state_tbl;
    state_idx_t _curr;
    const char* _text;
};

class pattern_automaton{
public:
    enum state_idx_enum {
        INVALID_STATE = -1, ROOT_STATE = 0
    };
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
        std::set<int> matched;
    };
    typedef std::vector<state_t> states_t;
    pattern_automaton(const char* patterns[]):_jump_tbl(NULL),_state_tbl(NULL)
        { _state_tbl_size = 0; _jump_tbl_size=0;build(patterns);}
    ~pattern_automaton() {
        delete[]_jump_tbl;
        delete[]_state_tbl;
    }
    void build(const char* patterns[]) {
        trie_t::iterator itr;
        trie_t _trie;
        states_t _states;
        int i;
        _states.resize(1);
        for (i = 0; patterns[i] != NULL; ++i) {
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
            _states[state].matched.insert(i);
        }
        _cleanup();
        _state_tbl = new state_entry_t[_states.size()];
        _state_tbl_size = _states.size();
        _jump_tbl = new jump_entry_t[_trie.size()];
        _jump_tbl_size = _trie.size();
        for(i=0,itr = _trie.begin();itr!=_trie.end();++itr,++i){
            _jump_tbl[i].src = itr->first.state;
            _jump_tbl[i].val = itr->first.value;
            _jump_tbl[i].dst = itr->second;
        }
        for(int j=0; j<_states.size();++j) {
            _state_tbl[j].jump_tbl_begin = _lower_bound(j);
            int end = _state_tbl[j].jump_tbl_begin;
            while(end>=0 && end < _state_tbl_size && j==_jump_tbl[end].src){
                ++end;
            }
            _state_tbl[j].jump_tbl_end = end;
        }
        //build fail links
        _fail(ROOT_STATE, ROOT_STATE);
        std::queue<state_idx_t> que;
        for (int l=0; l<_jump_tbl_size && _jump_tbl[l].src == ROOT_STATE; ++l) {
            state_idx_t q = _jump_tbl[l].dst;
            _fail(q, ROOT_STATE);
            que.push(q);
        }
        while (!que.empty()) {
            state_idx_t r = que.front();
            que.pop();
            int l = _state_tbl[r].jump_tbl_begin;
            for (; l>0 && l < _jump_tbl_size && _jump_tbl[l].src == r; ++l) {
                state_idx_t u = _jump_tbl[l].dst;
                char a = _jump_tbl[l].val;
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
                _build_output(_state_tbl[u].output,_states[u].matched);
            }
        }
    }
    automaton_matcher match(const char* text) {
        automaton_matcher ret(_jump_tbl,_jump_tbl_size,_state_tbl);
        ret.match(text);
        return ret;
    }
    void debug_dump(std::ostream& oss) {
        for(int i=0; i<_jump_tbl_size;++i){
            jump_entry_t& entry = _jump_tbl[i];
            oss << i<<":{s:"<<entry.src<<",v:'"<<entry.val<<"',d:"<<entry.dst<<"}\n";
        }
        for(int i=0; i<_state_tbl_size;++i){
            state_entry_t& entry = _state_tbl[i];
            oss << i<<":{f:"<<entry.fail_link<<",b:"
                <<entry.jump_tbl_begin<<",e:"
                <<entry.jump_tbl_end<<",o:[";
            for(int j=0;j<entry.output.count;++j) {
                if (j!=0) oss << ',';
                oss << entry.output.patterns[j];
            }
            oss<<"]}\n";
        }
    }
private:
    void _build_output(matched_output& o, const std::set<int>& s) {
        o.count = s.size();
        if (s.empty()) {
            return;
        }
        o.patterns = new int[s.size()];
        copy(s.begin(),s.end(), o.patterns);
    }
    int _lower_bound(state_idx_t q){
        int i=0;
        for(; i<_jump_tbl_size;++i) {
            if(q == _jump_tbl[i].src)
                return i;
        }
        return -1;
    }
    state_idx_t _fail(state_idx_t q) {
        state_idx_t retval = ROOT_STATE;
        if (q >= 0)
            retval = _state_tbl[q].fail_link;
        return retval;
    }
    void _fail(state_idx_t q1, state_idx_t q2) {
        if (q1 < 0) return;
        _state_tbl[q1].fail_link = q2;
    }
    state_idx_t _goto(state_idx_t q, char v) {
        int i=0;
        for(int i=0; i<_jump_tbl_size;++i) {
            if(q == _jump_tbl[i].src && v == _jump_tbl[i].val)
                return _jump_tbl[i].dst;
        }
        return INVALID_STATE;
    }
    void _cleanup() {
        if (_jump_tbl) delete[] _jump_tbl;
        if (_state_tbl) delete[] _state_tbl;
    }
    jump_entry_t* _jump_tbl;
    int _jump_tbl_size;
    state_entry_t* _state_tbl;
    int _state_tbl_size;
};

#endif /* PATTERNS_MATCH_HPP_ */
