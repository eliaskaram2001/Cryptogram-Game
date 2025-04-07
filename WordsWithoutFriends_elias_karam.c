#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

//PROJECT: WORDS WITHOUT FRIENDS //AUTHOR: ELIAS KARAM //EMAIL: EAKARAM@ALBANY.EDU

#define MAX_INPUT_SIZE 100
#define BATCH_SIZE 10 // Number of words to display at a time

// Structure for the dictionary and game linked list
typedef struct wordListNode {
    char word[30];
    bool found; // true if the word has been found in the game
    struct wordListNode *next;
} wordListNode;

// Global variables for the root of the dictionary list and game list
wordListNode *root = NULL;
wordListNode *gameList = NULL;

// Function declarations
int initialization();
void gameLoop();
void displayWorld();
void acceptInput(bool *found);
bool isDone();
void teardown();
void getLetterDistribution(const char *word, int *letterDist);
bool compareCounts(const int *choicesDist, const int *wordDist);
wordListNode* getRandomWord(int wordCount);
void findWords(const char *masterWord);
void cleanupGameListNodes();
void cleanupWordListNodes();



// Main function
int main() {
    int wordCount = initialization();
    if (wordCount == 0) {
        printf("Error: No words loaded from dictionary.\n");
        return 1;
    }

    wordListNode *masterWordNode = getRandomWord(wordCount);
    if (masterWordNode) {
        printf("Master word: %s\n", masterWordNode->word);
        findWords(masterWordNode->word);
        displayWorld();  // Display word list only once at the beginning
        gameLoop();
    }

    teardown();
    return 0;
}

// Initialization method: loads dictionary into linked list
int initialization() {
    srand(time(NULL));

    FILE *file = fopen("2of12.txt", "r");
    if (!file) {
        printf("Error opening file\n");
        return 0;
    }

    char line[30];
    int wordCount = 0;

    // Reads each word and adds it to the linked list
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; // Removes newline character

        // Allocates memory for a new node
        wordListNode *newNode = malloc(sizeof(wordListNode));
        if (!newNode) {
            printf("Error: Memory allocation failed.\n");
            fclose(file);
            return wordCount; 
        }

        strcpy(newNode->word, line);
        newNode->found = false; 
        newNode->next = root;
        root = newNode;

        wordCount++;
    }

    fclose(file);
    return wordCount; // Returns the number of words read
}

// Gets a random long word (>6 characters) from the dictionary list
wordListNode* getRandomWord(int wordCount) {
    if (wordCount == 0) {
        return NULL; // No words to choose from
    }

    wordListNode *current = root;
    int index = rand() % wordCount;

    for (int i = 0; i < index && current != NULL; i++) {
        current = current->next;
    }

    // Searches for a word longer than 6 characters
    while (current != NULL && strlen(current->word) <= 6) {
        current = current->next;
    }

    // If no long word is found, search again from the start
    if (!current) {
        current = root;
        while (current != NULL && strlen(current->word) <= 6) {
            current = current->next;
        }
    }

    return current;
}

// Finds words that can be made from the master word's letters
void findWords(const char *masterWord) {
    int masterDist[26];
    getLetterDistribution(masterWord, masterDist);

    wordListNode *current = root;
    while (current != NULL) {
        int wordDist[26];
        getLetterDistribution(current->word, wordDist);

        if (compareCounts(masterDist, wordDist)) {
            // Add word to gameList if it can be formed from master word's letters
            wordListNode *newNode = malloc(sizeof(wordListNode));
            if (!newNode) {
                printf("Error: Memory allocation failed.\n");
                return;
            }
            strcpy(newNode->word, current->word);
            newNode->found = false;
            newNode->next = gameList;
            gameList = newNode;
        }

        current = current->next;
    }
}

// Main game loop
void gameLoop() {
    bool found;
    while (!isDone()) {
        acceptInput(&found);
        
        if (!found) {
            printf("\nWord not found!\n"); // Display message above dashed line
        }
        printf("----------\n"); // Keep the dashed line separate
    }
}

// Displays the game list in batches only once at the start
void displayWorld() {
    printf("----------\n");
    printf("Words you need to find:\n");

    // Display the game list in batches
    wordListNode *current = gameList;
    int count = 0;
    while (current != NULL) {
        if (count > 0 && count % BATCH_SIZE == 0) {
            printf("Press Enter to continue...\n");
            getchar(); // Wait for user to press Enter
        }

        if (current->found) {
            printf("FOUND: %s\n", current->word);
        } else {
            for (int i = 0; i < strlen(current->word); i++) {
                printf("_");
            }
            printf("\n");
        }
        count++;
        current = current->next;
    }
    printf("----------\n");
}

// Accepts and processes input from the user
void acceptInput(bool *found) {
    char input[MAX_INPUT_SIZE];

    printf("Enter a guess: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0'; // Remove newline

    // Convert input to uppercase for comparison
    for (int i = 0; input[i]; i++) {
        input[i] = toupper(input[i]);
    }

    // Check if input matches any word in the game list
    *found = false;
    wordListNode *current = gameList;
    while (current != NULL) {
        if (strcmp(current->word, input) == 0) {
            current->found = true;
            printf("You found: %s\n", current->word);
            *found = true;
            break;
        }
        current = current->next;
    }
}

// Checks if all words are found
bool isDone() {
    wordListNode *current = gameList;
    while (current != NULL) {
        if (!current->found) {
            return false;
        }
        current = current->next;
    }
    return true;
}

// Teardown function to clean up after the game
void teardown() {
    cleanupGameListNodes();
    cleanupWordListNodes();
    printf("All Done\n");
}

// Calculate the letter distribution of a word
void getLetterDistribution(const char *word, int *letterDist) {
    for (int i = 0; i < 26; i++) {
        letterDist[i] = 0;
    }

    // Count the occurrences of each letter
    for (int i = 0; word[i]; i++) {
        if (isalpha(word[i])) {
            letterDist[toupper(word[i]) - 'A']++;
        }
    }
}

// Compare the letter distribution of two words
bool compareCounts(const int *choicesDist, const int *wordDist) {
    for (int i = 0; i < 26; i++) {
        if (wordDist[i] > choicesDist[i]) {
            return false;
        }
    }
    return true;
}

// Frees memory for game nodes
void cleanupGameListNodes() {
    wordListNode *current = gameList;
    while (current != NULL) {
        wordListNode *next = current->next;
        free(current);
        current = next;
    }
    gameList = NULL;
}

// Frees memory for dictionary nodes
void cleanupWordListNodes() {
    wordListNode *current = root;
    while (current != NULL) {
        wordListNode *next = current->next;
        free(current);
        current = next;
    }
    root = NULL;
}
