#include "search_server.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

template <typename Y>
void Assertation(const Y& y, const string& y_str, const string& file,
    const string& func, unsigned line, const string& hint)
{
    if (!y)
    {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << y_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

template <typename F>
void RunTestImpl(const F& function, const string& func_name) {
    function();
    cerr << func_name << " OK" << endl;
}

//#define ASSERT(expr) Assertation((expr),#expr,__FILE__,__FUNCTION__, __LINE__, ""s)

//#define ASSERT_HINT(expr, hint) Assertation((expr),#expr,__FILE__,__FUNCTION__, __LINE__, (hint))

//#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

//#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

//#define RUN_TEST(func) RunTestImpl((func),#func)

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestAddDocument()
{
    const int doc_id = 42;
    SearchServer server;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 3, 1, 5 };
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto& found_doc = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(found_doc.size(), 1u);
    const Document& doc = found_doc[0];
    ASSERT_EQUAL(doc.id, doc_id);
}

void TestMinusWordsAccounting()
{
    SearchServer server;
    const vector<int> ratings = { 3, 1, 5 };
    server.AddDocument(42, "cat boy street", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(43, "cat girl house", DocumentStatus::ACTUAL, ratings);

    const auto& found_doc = server.FindTopDocuments("cat -boy"s);
    ASSERT_EQUAL(found_doc.size(), 1u);
    const Document& doc = found_doc[0];
    ASSERT_EQUAL(doc.id, 43);
}

void TestMatchDoc()
{
    SearchServer server;
    const vector<int> ratings = { 3, 1, 5 };
    server.AddDocument(42, "cat boy street", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(43, "cat girl wood", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(44, "", DocumentStatus::ACTUAL, ratings);
    {
        vector <string> right_words = { "cat"s,"boy"s };
        const auto [words, status] = server.MatchDocument("cat boy", 42);
        bool check = true;
        for (const string& word : words)
        {
            auto it = find(right_words.begin(), right_words.end(), word);
            if (it == right_words.end())
            {
                check = false;
                break;
            }
        }
        ASSERT_EQUAL(check, true);
    }

    {
        const auto [words, status] = server.MatchDocument("car engine", 44);
        ASSERT(words.empty());
    }
    {
        const auto [words, status] = server.MatchDocument("-cat girl", 43);
        ASSERT(words.empty());
    }
}

void TestSortRelevance()
{
    SearchServer server;
    const vector<int> ratings = { 3, 1, 5 };
    server.AddDocument(42, "cat boy street", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(43, "cat girl wood", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(44, "dog girl street", DocumentStatus::ACTUAL, ratings);
    {
        const auto& found_doc = server.FindTopDocuments("cat wood"s);
        double rel = -1.0;
        for (const auto& doc : found_doc) {
            if (rel == -1.0)
                rel = doc.relevance;
            else
            {
                ASSERT(doc.relevance <= rel);
            }
            rel = doc.relevance;
        }
    }
}

void TestCalcRating()
{
    SearchServer search_server;
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });


    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id == 3; })) {
        ASSERT_EQUAL(document.rating, 9 / 1);
    }
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id == 2; })) {
        ASSERT_EQUAL(document.rating, (5 - 12 + 2 + 1) / 4);
    }
}

void TestByFilter()
{
    SearchServer search_server;
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        ASSERT_EQUAL(document.id % 2, 0);
    }
}

void TestCorrectRelevance()
{
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL,
        { 5, -12, 2, 1 });
    const double EPSILON = 1e-6; // для проверки вещ. чисел

    for (const Document& document : search_server.FindTopDocuments("ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id == 1; })) {
        ASSERT(abs(document.relevance - 0.101366) < EPSILON);
    }

    for (const Document& document : search_server.FindTopDocuments("ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id == 2; })) {
        ASSERT(abs(document.relevance - 0.274653) < EPSILON);
    }
}

void TestSearchByStatus()
{
    SearchServer search_server;
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    vector <Document> found_doc = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(found_doc[0].id, 3);
    /*for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; })) {
        ASSERT_EQUAL(document.id, 3);
    }*/
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMinusWordsAccounting);
    RUN_TEST(TestMatchDoc);
    RUN_TEST(TestSortRelevance);
    RUN_TEST(TestCalcRating);
    RUN_TEST(TestByFilter);
    RUN_TEST(TestSearchByStatus);
    RUN_TEST(TestCorrectRelevance);
}

// ---

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
