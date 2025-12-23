#include "relevance_scorer.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>

// Helper to lower case a string strictly for the phrase matching check
std::string toLowerRaw(const std::string& input) {
    std::string out = input;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

std::vector<std::string> RelevanceScorer::tokenizeAndPreprocess(const std::string& text) const {
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string token;

    while (ss >> token) {
        // Lowercase
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        // Remove punctuation
        token.erase(std::remove_if(token.begin(), token.end(), 
                    [](char c){ return !std::isalnum(c); }), token.end());
        
        // Only add if not empty and not a stop word
        if (!token.empty() && STOP_WORDS.find(token) == STOP_WORDS.end()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

double RelevanceScorer::calculateIdf(const std::string& term, const TokenizedCorpus& tokenizedDocs) const {
    int N = tokenizedDocs.size();
    int docCount = 0;

    for (const auto& pair : tokenizedDocs) {
        if (std::find(pair.second.begin(), pair.second.end(), term) != pair.second.end()) {
            docCount++;
        }
    }
    // Standard IDF formula
    return std::log((double)N / (1.0 + docCount));
}

std::vector<DocumentScore> RelevanceScorer::scoreDocuments(
    const CorpusMap& documentTexts,
    const std::string& query
) {
    std::vector<DocumentScore> results;
    if (documentTexts.empty()) return results;

    // 1. Tokenize all documents
    TokenizedCorpus tokenizedDocs;
    for (const auto& pair : documentTexts) {
        tokenizedDocs[pair.first] = tokenizeAndPreprocess(pair.second);
    }

    // 2. Tokenize query
    std::vector<std::string> queryTokens = tokenizeAndPreprocess(query);

    // 3. Pre-calculate IDF for query terms
    std::map<std::string, double> idfScores;
    for (const std::string& term : queryTokens) {
        if (idfScores.find(term) == idfScores.end()) {
            idfScores[term] = calculateIdf(term, tokenizedDocs);
        }
    }

    // 4. Calculate Scores
    for (const auto& docPair : documentTexts) {
        DocumentScore ds;
        ds.filePath = docPair.first;
        const auto& docTokens = tokenizedDocs[ds.filePath];

        // --- A. Term Frequency Saturation (THE FIX) ---
        std::map<std::string,int> termFrequency;
        for (const auto& token : docTokens) termFrequency[token]++;

        double relevanceSum = 0.0;
        for (const auto& term : queryTokens) {
            int tfRaw = termFrequency.count(term) ? termFrequency.at(term) : 0;
            
            if (tfRaw > 0) {
                // SATURATION LOGIC:
                // Instead of just 'tfRaw', we use '1 + log(tfRaw)'
                // If count is 10, score is ~3.3. If count is 1000, score is ~7.9 (not 1000!)
                double tfSaturated = 1.0 + std::log((double)tfRaw);
                
                double idf = idfScores.count(term) ? idfScores.at(term) : 0.0;
                relevanceSum += tfSaturated * idf;
            }
        }

        // --- B. EXACT PHRASE BONUS ---
        std::string docLower = toLowerRaw(docPair.second);
        std::string queryLower = toLowerRaw(query);
        
        // Boosted from 50.0 to 100.0 to fight the large textbooks harder
        if (query.length() > 5 && docLower.find(queryLower) != std::string::npos) {
            relevanceSum += 100.0; 
        }

        // --- C. LENGTH NORMALIZATION ---
        double docLength = (double)docTokens.size();
        if (docLength < 1.0) docLength = 1.0;
        
        // Normalize
        ds.score = relevanceSum / std::sqrt(docLength);

        results.push_back(ds);
    }

    // 5. Sort descending
    std::sort(results.begin(), results.end(), [](const DocumentScore& a, const DocumentScore& b){
        return a.score > b.score;
    });

    return results;
}