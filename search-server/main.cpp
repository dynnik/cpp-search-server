#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <cassert>
#include <vector>
#include <optional>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

template <typename Function>
void RunTestImpl(Function function, const string& func) {
    function();
    cerr << func << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl( (func) , #func )

template<typename Key, typename Value>
ostream& operator<<(ostream& out, const pair<Key, Value>& container) {
    out << container.first << ": " << container.second;
    return out;
}

template <typename Container>
void Print(ostream& out, const Container& container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) {
            out << ", "s;
        }
        is_first = false;
        out << element;
    }
}

template <typename Element>
ostream& operator<<(ostream& out, const vector<Element>& container) {
    out << "["s;
    Print(out, container);
    out << "]"s;
    return out;
}

template<typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template <typename Element>
ostream& operator<<(ostream& out, const set<Element>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u,
    const string& t_str, const string& u_str,
    const string& file, const string& func,
    unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str,
    const string& file, const string& func,
    unsigned line, const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "Assert("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (word == "-"s)
                throw invalid_argument("no text after -");
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
    Document() :
        id(0),
        relevance(0.0),
        rating(0) {}

    Document(int id_, double relevance_, int rating_) :
        id(id_),
        relevance(relevance_),
        rating(rating_) {}

    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

    explicit SearchServer(const string& stop_words) {
        SetStopWords(stop_words);
    }

    explicit SearchServer(const char* stop_words) {
        SetStopWords(static_cast<string>(stop_words));
    }

    template<typename StringCollection>
    explicit SearchServer(const StringCollection& collection_stop_words) {
        for (const auto& stop_word : collection_stop_words) {
            if (!IsValidWord(stop_word))
                throw invalid_argument("incorrect word");
            if (stop_word.size() != 0) {
                stop_words_.insert(stop_word);
            }
        }
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    void AddDocument(int document_id, const string& document,
        DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0 || documents_.count(document_id) != 0) {
            throw invalid_argument("incorrect id");
        }
        vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        document_id_direct_order_.push_back(document_id);
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        vector<Document> matched_documents;
        auto query = ParseQuery(raw_query);
        /*if (!query.has_value()) {
            return nullopt;
        }*/

        matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                double eps = 1e-6;
                if (abs(lhs.relevance - rhs.relevance) < eps) {
                    return lhs.rating > rhs.rating;
                }
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus filter_status) const {
        return FindTopDocuments(raw_query, [filter_status](int document_id, DocumentStatus status, int rating) {
            return status == filter_status;
            });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const char* raw_query, DocumentPredicate document_predicate) const {
        return FindTopDocuments(static_cast<string>(raw_query), document_predicate);
    }

    vector<Document> FindTopDocuments(const char* raw_query, DocumentStatus filter_status) const {
        return FindTopDocuments(static_cast<string>(raw_query), filter_status);
    }

    vector<Document> FindTopDocuments(const char* raw_query) const {
        return FindTopDocuments(static_cast<string>(raw_query));
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        vector<string> matched_documents;
        DocumentStatus status;
        const auto query = ParseQuery(raw_query);
        /*if (!query.has_value()) {
            return nullopt;
        }*/

        if (documents_.count(document_id)) {
            status = documents_.at(document_id).status;
        }
        /*else {
            return nullopt;
        }*/

        if (!query.plus_words.empty()) {
            for (const auto& minus_word : query.minus_words) {
                if (word_to_document_freqs_.count(minus_word)) {
                    if (word_to_document_freqs_.at(minus_word).count(document_id)) {
                        return tuple(matched_documents, status);
                    }
                }
            }

            for (const auto& plus_word : query.plus_words) {
                if (word_to_document_freqs_.count(plus_word)) {
                    if (word_to_document_freqs_.at(plus_word).count(document_id)) {
                        matched_documents.push_back(plus_word);
                    }
                }
            }
        }
        return { matched_documents, status };
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const char* raw_query, int document_id) const {
        return MatchDocument(static_cast<string>(raw_query), document_id);
    }

    int GetDocumentId(int index) const { // index - порядковый номер
        if (index < 0 || index > documents_.size()) {
            throw out_of_range("index is out of range");
        }
        return document_id_direct_order_[index];
    }

private:

    static bool IsValidWord(const string& word){
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            if (IsValidWord(word))
            {
                stop_words_.insert(word);
            }
            else
            {
                throw invalid_argument("incorrect word");
            }
        }
    }

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    vector<int> document_id_direct_order_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("incorrect symbol in text");
            }

            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        int cnt_minus = 0;
        for (const char& ch : text) {
            if (ch == '-') {
                ++cnt_minus;
            }
            else {
                break;
            }
        }

        if (cnt_minus > 1 || text.size() == cnt_minus) {
            throw invalid_argument("too many - before the word");
        }
        if (!none_of(text.begin(), text.end(), [](char c) {
            return c >= '\0' && c < ' ';
            }))
        {
            throw invalid_argument("incorrect symbols in query");
        }
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("incorrect word in query");
            }

            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(documents_.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};


void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}
