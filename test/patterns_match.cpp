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

#include <set>
#include <map>
#include <iostream>
#include <vector>
#include <queue>
#include <list>
#include <string>

#include "patterns_match.hpp"

using namespace std;

void print_matches(const char* text, const char* patts[], automaton_matcher& m) {
    cout << text << "\n";
    for (const char* _text = text; _text != m.text(); ++_text) {
        cout << ' ';
    }
    cout << m.text() << ":";
    const matched_output& matches = m.matches();
    for(int i=0; i<matches.count; ++i) {
        cout << patts[matches.patterns[i]] << " ";
    }
    cout << endl;
}

int main(int argc, char*argv[]) {
    const char* patterns1[] = { "he", "hers", "his", "she", NULL };
    const char* text1 = "hello world, he and she and his dog";
    pattern_automaton am(patterns1);
    am.debug_dump(cout);
    automaton_matcher m = am.match(text1);
    while(m.next()){
        print_matches(text1, patterns1, m);
    }
//    const char* text1 = "hello world, he and she and his dog";
//    const char* patterns1[] = { "he", "hers", "his", "she", NULL };
//    void* values[] = { (void*) 0, (void*) 1, (void*) 2, (void*) 3 };
//
//    patterns_matcher matcher(patterns1, values);
//    patterns_matcher::match_result result = matcher.match(text1);
//    print_all_match(text1, result);
//
//    const char* text2 = "ABC ABCDAB ABCDABCDABDE";
//    const char* patterns2[] = { "ABCDABD", NULL };
//    matcher.build(patterns2, values);
//    print_all_match(text2, matcher.match(text2));
    return 0;
}
