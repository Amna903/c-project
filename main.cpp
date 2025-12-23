#include <iostream>
#include <string>
#include <vector>
#include <map>       // For std::map
#include <iomanip>   // For std::fixed, std::setprecision
#include <algorithm> // For std::min
#include <cmath>     // For std::isinf

// NOTE: Assuming these header files and structs (like DocumentScore, ScholarResult) are defined correctly.
#include "pdf_extractor.h"
#include "file_manager.h" 
#include "relevance_scorer.h"
#include "scholar_search.h" 

// Define the custom types from relevance_scorer.h for main.cpp use
using CorpusMap = std::map<std::string, std::string>; // Map<FilePath, ExtractedText>

int main() {
    // --- 1. Define Search Parameters ---
    // Target Topic: Highly specific topic to test ranking accuracy
    const std::string searchTopic = "Title: AI-Powered Social Media Automation App";
    
    // !!! CRITICAL: Set this path to the root folder you want to scan !!!
    const std::string searchDirectory = "/Users/amanmalik/Downloads/pdfs"; 
    // ------------------------------------------------------------------

    std::cout << "\n--- Starting PDF Organizer Application ---\n";
    std::cout << "Target Topic: '" << searchTopic << "'" << std::endl;

    // --- 2. Module A: Local File Search (File System Manager) ---
    FileSystemManager fsManager;
    std::vector<std::string> pdfPaths = fsManager.findPdfs(searchDirectory);
    std::cout << "\n[Local Status] Found " << pdfPaths.size() << " potential PDF files in " << searchDirectory << std::endl;

    // --- 3. Module B & C: Local Processing and Scoring (PdfTextExtractor & RelevanceScorer) ---
    std::vector<DocumentScore> localResults;
    CorpusMap documentCorpus; 
    double maxScore = 0.0; // Raw maximum score found

    if (!pdfPaths.empty()) {
        PdfTextExtractor extractor;
        std::cout << "[Local Status] Extracting text and building corpus..." << std::endl;
        for (const auto& path : pdfPaths) {
            std::string text = extractor.extractText(path);
            if (!text.empty()) {
                documentCorpus[path] = text;
            }
        }
        std::cout << "[Local Status] Corpus built from " << documentCorpus.size() << " usable documents." << std::endl;
        
        if (!documentCorpus.empty()) {
            RelevanceScorer scorer;
            localResults = scorer.scoreDocuments(documentCorpus, searchTopic);
            
            // Determine raw max score for normalization
            if (!localResults.empty() && localResults[0].score > 0.0) {
                maxScore = localResults[0].score;
            }
        }
    }
    
    // --- 4. Fallback Condition Check (Quantity OR Absolute Quality) ---
    
    // Absolute Threshold: The raw score of the top document must be above this value to be considered truly relevant.
    const double MIN_ABSOLUTE_SCORE_THRESHOLD = 0.500000; 
    
    double topScoreRaw = localResults.empty() ? 0.0 : localResults[0].score;

    // Fallback is required if:
    // 1. Too few documents found (less than 5), OR
    // 2. The most relevant document's raw score is still below the absolute quality threshold.
    bool fallbackNeeded = localResults.size() < 5 || topScoreRaw < MIN_ABSOLUTE_SCORE_THRESHOLD;
    
    // --- DEBUGGING LOG ---
    std::cout << "[DEBUG] Top document raw score: " << std::fixed << std::setprecision(6) << topScoreRaw << std::endl;
    std::cout << "[DEBUG] Absolute threshold: " << std::fixed << std::setprecision(6) << MIN_ABSOLUTE_SCORE_THRESHOLD << std::endl;
    std::cout << "[DEBUG] Fallback needed: " << (fallbackNeeded ? "YES" : "NO") << std::endl;
    // ---------------------

    // --- 5. Display Results or Run Fallback ---
    if (fallbackNeeded) {
        std::cout << "\n[Next Step] Local search failed the quality check.\n";
        
        std::cout << "\n--- Initiating Google Scholar Search Fallback ---" << std::endl;
        ScholarSearch scholar;
        // Search using the clean topic
        std::vector<ScholarResult> onlineResults = scholar.search(searchTopic);

        // --- 6. Report Online Results ---
        std::cout << "\n--- Online Search Results (Google Scholar) ---" << std::endl;
        if (onlineResults.empty()) {
            std::cout << "[Online Result] No online results found or fetching failed (Check network/firewall)." << std::endl;
        } else {
            std::cout << "Top " << std::min(onlineResults.size(), (size_t)5) << " Online Results:" << std::endl;
            for (size_t i = 0; i < std::min(onlineResults.size(), (size_t)5); ++i) {
                const auto& res = onlineResults[i];
                std::cout << i + 1 << ". Title: " << res.title << std::endl;
                std::cout << "   URL: " << res.url << std::endl;
                // Output only the first 70 characters of the snippet for clean display
                std::string display_snippet = (res.snippet.length() > 70) ? 
                                              res.snippet.substr(0, 70) + "..." : res.snippet;
                std::cout << "   Snippet: " << display_snippet << std::endl;
            }
        }
    } else {
        // --- Display Local Results (Only if Fallback NOT Needed) ---
        std::cout << "\n[Next Step] Local search yielded sufficient, high-relevance results. Skipping online fallback." << std::endl;
        
        std::cout << "\n--- Local Search Results (TF-IDF Ranked) ---" << std::endl;
        
        std::cout << "Top 5 Most Relevant Local Documents (Score indicates relevance):" << std::endl;
        int rank = 1;

        for (const auto& res : localResults) {
            if (rank > 5 || res.score <= 0.0) break;
            // Normalize score for display using the determined maxScore
            double displayScore = (res.score / maxScore) * 100.0;
            
            // Extract only the filename from the full path for cleaner display
            std::string full_path = res.filePath;
            size_t last_slash = full_path.find_last_of("/\\");
            std::string display_path = (last_slash == std::string::npos) ? 
                                       full_path : full_path.substr(last_slash + 1);

            std::cout << rank++ << ". [" << std::fixed << std::setprecision(2) 
                      << displayScore << "%] - " << display_path << std::endl;
        }
    }

    std::cout << "\n--- Application Finished ---\n";

    return 0;
}