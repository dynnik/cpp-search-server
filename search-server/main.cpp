#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

struct Query {
    set <string> plus_words;
    set <string> minus_words;
};

class SearchServer {
public:

    int document_count_ = 0;

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        
        const vector<string> words = SplitIntoWordsNoStop(document);

        for (const string& word : words)
        {
            if (word_to_document_freqs_.count(word) && word_to_document_freqs_[word].count(document_id))
                continue;

            word_to_document_freqs_[word][document_id] = count(words.begin(), words.end(), word) / (double)words.size();
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    map<string/*word*/, map<int, double>/*doc, relevance in TF*/> word_to_document_freqs_;

    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-')
            {
                word.substr(1);
                if (!IsStopWord(word))
                {
                    query_words.minus_words.insert(word);
                }
            }
            query_words.plus_words.insert(word);
        }
        return query_words;
    }

    vector<Document> FindAllDocuments(Query& query_words) const {
        
        vector<Document> matched_documents;

        map<int, double> documents_to_relevance;

        for (const string& word : query_words.plus_words)
        {
            if (word_to_document_freqs_.count(word) != 0)
            {
                double IDF = log(document_count_ / (double)word_to_document_freqs_.at(word).size());
                double rel;
                for(auto& val : word_to_document_freqs_.at(word))
                {
                    rel = val.second * IDF;
                    documents_to_relevance[val.first] += rel;
                }
            }
        }
        for (const string& word : query_words.minus_words)
        {
            if (word_to_document_freqs_.count(word) != 0)
            {
                for (auto& val : word_to_document_freqs_.at(word))
                {
                    documents_to_relevance.erase(val.first);
                }
            }
        }
        for (const auto& [id, relevance] : documents_to_relevance)
        {
            Document doc;
            doc.id = id;
            doc.relevance = relevance;
            matched_documents.push_back(doc);
        }
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    search_server.document_count_ = ReadLineWithNumber();
    for (int document_id = 0; document_id < search_server.document_count_; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}
