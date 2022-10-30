#include <set>
#include <vector>

#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer &search_server) {
    set<set<string>> unique_docs;
    vector<int> docs_to_delete;
    for (const int id : search_server) {
        const map<string, double> word_freqs = search_server.GetWordFrequencies(id);
        set<string> words;
        transform(word_freqs.begin(), word_freqs.end(), inserter(words, words.begin()),
                  [](const pair<string, double> word) {
            return word.first;
        });

        if (unique_docs.count(words) == 0) {
            unique_docs.insert(words);
        } else {
            docs_to_delete.push_back(id);
        }
    }

    for (const int id : docs_to_delete) {
        cout << "Found duplicate document id " + to_string(id) << "\n";
        search_server.RemoveDocument(id);
    }
}