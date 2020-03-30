

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

void test1() {
    aho_corasick::trie trie;
    trie.insert("hers");
    trie.insert("his");
    trie.insert("she");
    trie.insert("he");
    auto result = trie.parse_text("ushers");
    for (auto i : result) {
        std::cerr << i.first << " " << i.second << std::endl;
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
    auto results = trie.parse_text(std::vector<blaat>{blaat{6}, blaat{5}, blaat{2}, blaat{4}, blaat{5}});
    for (const auto &i : results) {
        std::cerr << "vtype: " << i.first << " pos: " << i.second << " " << std::endl;
    }
}

void test3() {
    std::string words_uncut = read_from_file("words");
    std::cerr << words_uncut.size() << std::endl;
    std::vector<std::string> words;
    char *begin = (char *) words_uncut.c_str();
    std::size_t size = strcspn(begin, "\n");

    while (size && words.size() < 100) {
        if (size > 15) {
            words.push_back(std::string(begin, begin + size));
        }
        begin += size + 1;
        size = strcspn(begin, "\n");
    }

    aho_corasick::basic_trie<std::string, std::size_t> trie;
    for (std::size_t i = 0; i < words.size(); i++) {
        trie.map(words[i], i);
    }
    trie.check_construct_failure_states();
    writeToFile("/tmp/test3.dot", trie.toDot());

    for (std::size_t i = 0; i < words.size(); i++) {
        std::cerr << i << " ";

        auto res = trie.parse_text(words[i]);
        assert(res.size() > 0);
        std::set<std::pair<std::size_t, std::size_t> > expected;

//            for (auto &pattern : words) {
        for (std::size_t j = 0; j < words.size(); j++) {
            size_t pos = words[i].find(words[j], 0);
            while (pos != std::string::npos) {
                expected.insert(std::make_pair(j, pos + words[j].size() - 1));
//                    std::cerr << "expected in " << words[i] << ": " << pattern << " at pos " << (pos + pattern.size() - 1) << std::endl;
                pos = words[i].find(words[j], pos + 1);
            }
        }

//            expected.insert(std::make_pair(words[i], words[i].size() - 1));
        std::set<std::pair<std::size_t, std::size_t> > actual;
        for (const auto &r : res) {
            actual.insert(r);
//                std::cerr << "seen in " << words[i] << ": " << r.first << " at pos " << r.second << std::endl;
        }

        assert(expected == actual);

    }


}


int main() {
    test1();
    test2();
    test3();
    // dot -Tpng /tmp/blaat.dot  > /tmp/blaat.png && feh /tmp/blaat.png
}
