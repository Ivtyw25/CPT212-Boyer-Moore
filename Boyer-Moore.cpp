%%writefile main.cpp
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // For std::max

// =========================
// Constants and Type Aliases
// =========================
const int NUM_CHARS = 256;

// =========================
// Utility Print Functions
// =========================
void printAlignmentStep(int step, int shift) {
    std::cout << "Step " << step << ": Pattern aligned at index " << shift << std::endl;
}

void printShiftDetails(int badCharShift, int goodSuffixShift, const std::string& heuristic, int shiftAmount) {
    std::cout << "- Bad character shift: " << badCharShift;
    std::cout << "      - Good suffix shift: " << goodSuffixShift;
    std::cout << "      - Heuristic Chosen: " << heuristic << "      - Shifting right by: " << shiftAmount << std::endl;
}

void printPatternAlignment(const std::string& pattern, const std::string& text, int shift) {
    std::cout << "\nText:    " << text << std::endl;
    std::cout << "Pattern: ";
    for (int i = 0; i < shift; i++) std::cout << " ";
    std::cout << pattern << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;
}

/**
* Preprocesses the pattern to create the bad character heuristic table.
* This table stores the last index of each character's occurence in the pattern.
* When a mismatch occurs at a character `c` in the text, the pattern can be shifted
* forward so that the last occurent of `c` in the pattern aligns with the mismatched
* character in the text. If `c` is not in the pattern, the pattern can be shifted
* completely past it.
*
* @param pattern The string pattern to be searched for
* @param bacCharTable A referecen to a vector that will store the precomputed shift values.
*/
void precomputeBadCharacterTable(const std::string& pattern, std::vector<int>& badCharTable) {
    int patternLength = pattern.length();
    badCharTable.assign(NUM_CHARS, -1); // Initializes all characters to -1 (not found)

    // For each character in the pattern, record its index
    // If a character appears multiple times, it will take the last index
    for (int i = 0; i < patternLength; ++i) {
        badCharTable[(unsigned char)pattern[i]] = i;
    }
}

/**
* Preprocesses the pattern to create the good suffix heuristic table
* The good suffix rule is applied when a mismatch occurs after a suffix of the pattern
* has matched the text. The good suffix rule have two cases:
*
* 1. Find another occurence of the good suffix in the pattern that is not preceded
* by the same character as the mismatched one. Shift to align with it.
*
* 2. If no such occurence exists, find the longest prefix of the pattern that is a suffix
* of the good suffix, and shift to align them. If no such prefix exists, shift the pattern
* completely.
*
* @param pattern The string pattern to be searched for
* @param goodsuffixShifts A reference to a vector that will store the precomputed shift values.
* `goodSuffixShifts[k]` stores the shift distance for a good suffix of length `m-k`.
*/
void precomputeGoodSuffixTable(const std::string& pattern, std::vector<int>& goodSuffixShifts) {
    int m = pattern.length();
    goodSuffixShifts.assign(m + 1, 0);

    // `borderPos` stores the starting posistion of the widest border of each suffix of the pattern.
    // A "border" is a substring that is both a proper prefix and a proper suffix
    std::vector<int> borderPos(m + 1);

    int i = m;
    int j = m + 1;
    borderPos[i] = j;

    // The outer loop processes the pattern from right to left to find borders of all suffixes
    while (i > 0) {

        // Move j back until characters at pattern[i-1] & pattern[j-1] match
        // or until j moves past the end (j >m)
        while (j <= m && pattern[i - 1] != pattern[j - 1]) {
            // set it to the distance needed to align the next possible good suffix
            // when goodSuffixShifts at position j has not been set
            if (goodSuffixShifts[j] == 0) {
                goodSuffixShifts[j] = j - i;
            }
            // Follow the previously computed border chain to find a new possible border
            j = borderPos[j];
        }

        i--; j--;

        //Record the new border position for the suffix starting at position i
        borderPos[i] = j;
    }

    // Fill up the remaining shift values based on the pattern's widest border
    j = borderPos[0];
    for (i = 0; i <= m; ++i) {
        if (goodSuffixShifts[i] == 0) {
            goodSuffixShifts[i] = j;
        }
        if (i == j) {
            j = borderPos[j];
        }
    }
}

/**
* Searches for a pattern within a text using the Boyer-Moore algorithm
*
* @param text The text to be searched
* @param pattern The pattern to be searched for in the text
*/
void searchBoyerMoore(const std::string& text, const std::string& pattern) {
    int n = text.length(); // length of the text
    int m = pattern.length(); // length of the pattern

    // Edge case: if pattern is empty or longer than the text, no possible match
    if (m == 0 || n < m) {
        std::cout << "Pattern is empty or longer than the text." << std::endl;
        return;
    }

    // Vector to store the starting indices where pattern matches texxt
    std::vector<int> matchedIndex;

    // Preprocess the Bad Character heuristic table based on the pattern
    std::vector<int> badCharTable;
    precomputeBadCharacterTable(pattern, badCharTable);

    // Preprocess the Good Suffix heuristic table based on the pattern
    std::vector<int> goodSuffixShifts;
    precomputeGoodSuffixTable(pattern, goodSuffixShifts);


    int shift = 0;              // current alignment of pattern relative to text
    bool found = false;         // Flag to indicate if a match has been found
    int totalSkippedChars = 0;  // Total number of characters skipped through shifting
    int step = 1;               // Step counter for display output

    // Loop until pattern exceeds the remaining text
    while (shift <= (n - m)) {
        printAlignmentStep(step, shift); // Print current alignment at the current step
        int j = m - 1;                   // Start comparing from end of pattern
        int comparisonsThisStep = 0;     // Comparisons done in the current step

        // Compare pattern and text from right to left
        while (j >= 0) {
               if (pattern[j] == text[shift + j]) {
                j--;    // If characters match, move one position left
               } else {
                break;  // Exit when mismatch found
               }
        }

        // If j < 0 meaning a full match was found at current step
        if (j < 0) {
            matchedIndex.push_back(shift);  // Record the match position
            std::cout << "Pattern found at index: " << shift << std::endl;

            // Shift pattern using the Good suffix rule for a full match
            int finalShift = goodSuffixShifts[0];
            if (finalShift + shift <= (n-m))
              std::cout << "- Shifting right by: " << finalShift << "      - Chosen Heuristic: Good Suffix" << std::endl;

            shift += finalShift;  // Apply the shift
            if (finalShift > 1 && shift <= (n-m)) totalSkippedChars += finalShift - 1;  // Compute the skipped characters
            found = true; // Mark that at least one match was found
            step++;       // Move to the next alignment step

        }

        // Mismatched occured at patter[j]
        else {

            // Compute the number of shifts based on the current mismatched position using Bad Char Table
            int badCharShift = std::max(1, j - badCharTable[(unsigned char)text[shift + j]]);

            // Compute the number of shifts based on the current mismatched position using Good Suffix Table
            int goodSuffixShift = goodSuffixShifts[j + 1];

            // Take the largest shift among the two result
            int finalShift = std::max(badCharShift, goodSuffixShift);

            // Determine which heuristice was chosen for the current step
            std::string heuristic = (badCharShift >= goodSuffixShift) ? "Bad Character" : "Good Suffix";

            printShiftDetails(badCharShift, goodSuffixShift, heuristic, finalShift);
            shift += finalShift; // Apply the chosen shift
            if (finalShift > 1 && shift <= (n-m)) totalSkippedChars += finalShift - 1;
            step++;
        }
        if (shift <= (n - m))
          printPatternAlignment(pattern, text, shift);
    }

    if (!found) {
        std::cout << "Pattern not found in the text." << std::endl;
    }

    // Final results summary
    std::cout << "\n================================================" << std::endl;
    std::cout << "The pattern matched the text at index: ";
    for (int i = 0; i < matchedIndex.size(); i++) {
        std::cout << matchedIndex[i] << " ";
    }
    std::cout << "\nTotal Skipped Characters: " << totalSkippedChars << std::endl;
}

// ============================================================
// Main Program Entry Point
// ============================================================
int main() {
    std::string text = "AAAAAAB";
    std::string pattern = "AB";

    std::cout << "Text:    " << text << std::endl;
    std::cout << "Pattern: " << pattern << std::endl;
    std::cout << "----------------------------------" << std::endl;

    searchBoyerMoore(text, pattern);

    return 0;
}
