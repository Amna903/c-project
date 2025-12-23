#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>

struct DocumentScore {
    std::string filePath;
    double score;
};

// TokenizedCorpus = map<FilePath, tokens>
using TokenizedCorpus = std::map<std::string, std::vector<std::string>>;
using CorpusMap = std::map<std::string, std::string>;

class RelevanceScorer {
public:
    std::vector<DocumentScore> scoreDocuments(
        const CorpusMap& corpus,
        const std::string& topic
    );

private:
    std::vector<std::string> tokenizeAndPreprocess(const std::string& text) const;
    double calculateIdf(const std::string& term, const TokenizedCorpus& tokenizedDocs) const;

    // Minimal stop words list
    const std::set<std::string> STOP_WORDS = {
        "the","and","a","an","in","on","of","for","with","to","is","are","was","were"
    };
};
