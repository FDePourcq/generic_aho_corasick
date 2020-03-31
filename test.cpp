

#include "aho_corasick.hpp"

#include <iostream>

#include <string>
#include <iostream>
#include <vector>
#include <cassert>

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <string.h>

inline std::string read_from_file(char const *infile) {
    std::ifstream instream(infile);
    if (!instream.is_open()) {
        std::cerr << "Couldn't open file: " << infile << std::endl;
        //exit();
    }
    instream.unsetf(std::ios::skipws); // No white space skipping!
    return std::string(std::istreambuf_iterator<char>(instream.rdbuf()), std::istreambuf_iterator<char>());
}

void writeToFile(const std::string file, const std::string &s) {
    FILE *f = fopen(file.c_str(), "w");
    if (f != NULL) {
        fputs(s.c_str(), f);
        fclose(f);
    }
}

void test0() {
    aho_corasick::trie trie;
    trie.insert("ah");
    trie.insert("ahah");

    auto result = trie.collect_matches("ahahahah");
    for (auto i : result) {
      std::cerr << "pos " << i.begin << " to " << i.end  << " matches " << i.v << std::endl;
    }
    writeToFile("/tmp/test0.dot", trie.toDot());
}

void test1() {
    aho_corasick::trie trie;
    trie.insert("hers");
    trie.insert("his");
    trie.insert("she");
    trie.insert("he");
    auto result = trie.collect_matches("ushers");
    for (auto i : result) {
      std::cerr << "pos " << i.begin << " to " << i.end  << " matches " << i.v << std::endl;
    }
    writeToFile("/tmp/blaat.dot", trie.toDot());
}

void test2() {
    struct blaat {
        int f;

        bool operator<(const blaat &o) const {
            return f < o.f;
        }

        bool operator==(const blaat &o) const {
            return f == o.f;
        }
    };

    aho_corasick::basic_trie<std::vector<blaat>, std::size_t> trie;

    trie.map(std::vector<blaat>{blaat{4}}, 10);
    trie.map(std::vector<blaat>{blaat{5}, blaat{2}, blaat{4}}, 11);
    trie.map(std::vector<blaat>{blaat{2}}, 12);
    auto results = trie.collect_matches(std::vector<blaat>{blaat{6}, blaat{5}, blaat{2}, blaat{4}, blaat{5}});
    for (const auto &i : results) {
      std::cerr << "pos " << i.begin << " to " << i.end  << " matches " << i.v << std::endl;
    }
}

void test3() {
    std::string words_uncut = read_from_file("words");
    std::cerr << words_uncut.size() << std::endl;
    std::vector<std::string> words;
    char *begin = (char *) words_uncut.c_str();
    std::size_t size = strcspn(begin, "\n");

    while (size && words.size() < 1000000) {
        words.push_back(std::string(begin, begin + size));
        begin += size + 1;
        size = strcspn(begin, "\n");
    }

    aho_corasick::basic_trie<std::string, std::size_t> trie;
    typedef aho_corasick::basic_trie<std::string, std::size_t>::BeginEndValue bev;
    for (std::size_t i = 0; i < words.size(); i++) {
        trie.map(words[i], i);
	assert(trie.getNoCreate(words[i]) && *trie.getNoCreate(words[i]) == i);
    }
    trie.check_construct_failure_states();
    std::cerr << "writing dot-file ... "; 
    writeToFile("/tmp/test3.dot", trie.toDot());
    std::cerr << " done" << std::endl;
    for (std::size_t i = 0; i < words.size(); i++) {
        std::cerr << i << " ";
        std::set<bev > expected;
        for (std::size_t j = 0; j < words.size(); j++) {
            size_t pos = words[i].find(words[j], 0);
            while (pos != std::string::npos) {
	      
	      expected.insert(bev{pos, pos + words[j].size(),*(new std::size_t(j))});
	      pos = words[i].find(words[j], pos + 1);
            }
        }
        std::set<bev > actual;
        {
            auto res = trie.collect_matches(words[i]);
            assert(res.size() > 0);
            for (const auto &r : res) {
                actual.insert(r);
            }
        }
	// std::cerr << expected.begin()->begin << " " << actual.begin()->begin << std::endl;
	// std::cerr << expected.begin()->end << " " << actual.begin()->end << std::endl;
	// std::cerr << expected.begin()->v << " " << actual.begin()->v << std::endl;
	
        assert(expected == actual);
    }


}


int main() {
    test0();
    test1();
    test2();
    test3();
    // dot -Tpng /tmp/blaat.dot  > /tmp/blaat.png && feh /tmp/blaat.png
}
