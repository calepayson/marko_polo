/**
 * @file: main.c
 * @author: Cale Smith
 *
 * This file contains a program to train and run a quote generator based on a 
 * markov chain method. When provided with training data in the form of a file
 * filled with quotes, it loads all the data into a markov chain then uses that
 * chain to output a list of 80 results into output.txt.
*/


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Set the name of the file to be used to train the MarkovModel. The file must
 * be in the root directory of this project.
*/
#define FILE_NAME "quotes.txt"

/**
 * Set the number of words to hold in context. This constant is used in the 
 * MarkovContext data structure.
*/
#define MARKOV_CONTEXT_SIZE 3

/**
 * Set the maximum number of words allowed in generated quotes. This constant
 * is used in the markov_model_generate_quote() method.
*/
#define MAX_QUOTE_LENGTH 50

/**
 * Sets the number of buckets to be used in hashmaps. This constant is used in
 * the hashmap implemented within the MarkovModel data structure.
*/
#define HASH_MAP_SIZE 420

/**
 * MarkovContext stores an array of char pointers that represent the last X 
 * number of words.
*/
typedef struct MarkovContext {
  char *previous_words[MARKOV_CONTEXT_SIZE];
} MarkovContext;

/**
 * Return a new MarkovContext structure with a NULLed previous_words array. The
 * caller is responsible for freeing the memory allocated for the array by 
 * calling free.
 *
 * @param[out] context A fresh MarkovContext data structure.
*/
MarkovContext *markov_context_new(void) {
  MarkovContext *context = calloc(1, sizeof(MarkovContext));
  return context;
}

/**
 * Reset a provided MarkovContext, freeing all current words and resetting each
 * value in the previous_words array to NULL. If a NULL pointer is provided,
 * returns a pointer to a new MarkovContext instance. The caller is responsible 
 * for freeing memory allocated with the resulting MarkovContext instance.
*/
MarkovContext *markov_context_reset(MarkovContext *context) {
  if (!context) { return markov_context_new(); }
  for (size_t i = 0; i < MARKOV_CONTEXT_SIZE; ++i) {
    free(context->previous_words[i]);
    context->previous_words[i] = NULL;
  }
  return context;
}

/**
 * Add a word to the "front" of a given MarkovContext, shifting all the other
 * words one index toward the "back". If the context is full, frees the popped
 * word before returning. If context is a NULL pointer, initializes a new
 * MarkovContext, adds the word, and returns it. The data pointed to by word 
 * is duplicated by this function. The caller is responsible for freeing the 
 * MarkovContext and all associated data.
 *
 * @param[in,out] context A pointer to the context.
 * @param[in] word A pointer to a word.
*/
MarkovContext *markov_context_push_word(MarkovContext *context, char *word) {
  if (!context) { context = markov_context_new(); }
  size_t last_index = MARKOV_CONTEXT_SIZE - 1;
  char *new_word = strdup(word);
  if (context->previous_words[0]) {
    free(context->previous_words[0]);
  }
  for (size_t i = 0; i < last_index; i++) {
    context->previous_words[i] = context->previous_words[i + 1];
  }
  context->previous_words[last_index] = new_word;
  return context;
}

/**
 * Print a formatted string representing the given MarkovContext. Used in 
 * debugging.
 *
 * Format: [word1, word2, word3]
*/
void markov_context_print(MarkovContext *context) {
  printf("[%s", context->previous_words[0]);
  for (size_t i = 1; i < MARKOV_CONTEXT_SIZE; ++i) {
    printf(", %s", context->previous_words[i]);
  }
  printf("]");
}

/**
 * Free a provided markov context and all its associated data.
*/
void markov_context_free(MarkovContext *context) {
  if (!context) { return; }
  for (size_t i = 0; i < MARKOV_CONTEXT_SIZE; ++i) {
    if (context->previous_words[i]) {
      free(context->previous_words[i]);
    }
  }
  free(context);
}

/**
 * Return the hash for a given MarkovContext instance. The hash is calculated 
 * using the djb2 algorithm.
 *
 * @param[out] hash A cryptographic hash of the array of words in context.
*/
size_t markov_context_get_hash(MarkovContext *context) {
  size_t hash = 5381;
  for (size_t i = 0; i < MARKOV_CONTEXT_SIZE; ++i) {
    char *word = context->previous_words[i];
    if (word) {
      int c;
      while ((c = *word++)) {
        hash = ((hash << 5) + hash) + c;
      }
    } else {
      hash = ((hash << 5) + hash);
    }
  }
  return hash;
}

/**
 * Check to see if the contents of two MarkovContext instances are identical.
 * Compares the values of the two contexts at each index, returning false if 
 * they do not match.
 *
 * @return Returns false if the contexts do not match.
 *         Returns true if the contexts do match.
*/
bool markov_context_check_match(MarkovContext *a, MarkovContext *b) {
  for (size_t i = 0; i < MARKOV_CONTEXT_SIZE; ++i) {
    char *word_a = a->previous_words[i];
    char *word_b = b->previous_words[i];
    if (!word_a && !word_b) {
      continue;
    }
    if (!word_a || !word_b) {
      return false;
    }
    if (strcmp(word_a, word_b) != 0) {
      return false;
    }
  }
  return true;
}

/**
 * Make a deep copy of a provided MarkovContext instance. This function 
 * initializes a new MarkovContext instance, and loads it with duplicates of
 * each word in the original instance (in the same order). The caller is 
 * responsible for freeing the returned instance.
 *
 * @param[in] original_context A MarkovContext instance to be duplicated.
 * @param[out] new_context A deep copy of the original MarkovContext instance.
 *
 * @return Returns NULL if provided a NULL pointer.
*/
MarkovContext *markov_context_copy(MarkovContext *original_context) {
  if (!original_context) { return NULL; }
  MarkovContext *new_context = markov_context_new();
  for (size_t i = 0; i < MARKOV_CONTEXT_SIZE; ++i) {
    char *word = original_context->previous_words[i];
    if (word) {
      new_context = markov_context_push_word(new_context, word);
    }
  }
  return new_context;
}

/**
 * A linked list data structure that contains key/value mappings of words and
 * their respective counts.
*/
typedef struct MarkovValue {
  char *word;
  size_t count;
  struct MarkovValue *next;
} MarkovValue;

/**
 * Create a new MarkovValue instance loaded with the provided word.
*/
MarkovValue *markov_value_new(char *word) {
  MarkovValue *value = calloc(1, sizeof(MarkovValue));
  value->word = strdup(word);
  value->count = 1;
  return value;
}

/**
 * Add a word to an existing MarkovValue instance. This function iterates 
 * through the list, checking each existing MarkovValue. If word is already 
 * stored then it increments the value. If word is not already stored it appends
 * a new MarkovValue instance that represents word to the front of the list. The
 * caller is responsible for freeing the returned MarkovValue instance.
*/
MarkovValue *markov_value_add_word(MarkovValue *value, char *word) {
  MarkovValue *value_ptr = value;
  while (value_ptr) {
    if (strcmp(value_ptr->word, word) == 0) {
      value_ptr->count++;
      return value;
    }
    value_ptr = value_ptr->next;
  }
  MarkovValue *new_value = markov_value_new(word);
  new_value->next = value;
  return new_value;
}

/**
 * Print a formatted string representing each word/count pair in a provided
 * MarkovValue linked list. Used for debugging.
*/
void markov_value_print(MarkovValue *value) {
  printf("[ {%s: %zu}", value->word, value->count);
  value = value->next;
  while (value) {
    printf(", {%s: %zu}", value->word, value->count);
    value = value->next;
  }
  printf(" ]");
}

/**
 * Free all memory associated with a given MarkovValue instance and all 
 * subsequent MarkovValue instances that it points to.
*/
void markov_value_free(MarkovValue *value) {
  while (value) {
    free(value->word);
    MarkovValue *temp = value;
    value = value->next;
    free(temp);
  }
}

/**
 * Get a word from a provided MarkovValue linked list based on the probability
 * of its occurence. This function iterates through the list to get a total of
 * all the counts. It then chooses a random number between zero and that total.
 * Finally it walks the list subtracting the count of each value from the random
 * number until the random number is <= 0. It returns the word at that node.
*/
char *markov_value_get_random(MarkovValue *value) {
  size_t total_count = 0;
  MarkovValue *value_ptr = value;
  while (value_ptr) {
    total_count += value_ptr->count;
    value_ptr = value_ptr->next;
  }
  value_ptr = value;
  size_t r = rand() % total_count;
  while (value_ptr) {
    if (r < value_ptr->count) {
      return value_ptr->word;
    }
    r -= value_ptr->count;
    value_ptr = value_ptr->next;
  }
  return value->word;
}

/**
 * A linked list data structure that contains MarkovContexts and a MarkovValue
 * linked list representing all the values associated with that context.
*/
typedef struct MarkovNode {
  MarkovContext *context;
  MarkovValue *value;
  struct MarkovNode *next;
} MarkovNode;

/**
 * Add a word and its context to a linked list of MarkovNodes or return a new
 * linked list if node is NULL. Iterates through the provided list and, if it
 * finds a matching context, adds the word. If there is no matching context, it
 * appends the context and word to the list.
*/
MarkovNode *markov_node_add_node(MarkovNode *node, MarkovContext *context, char *word) {
  MarkovNode *node_ptr = node;
  while (node_ptr) {
    if (markov_context_check_match(node_ptr->context, context)) {
      node_ptr->value = markov_value_add_word(node_ptr->value, word);
      return node;
    }
    node_ptr = node_ptr->next;
  }
  MarkovNode *new_node = malloc(sizeof(MarkovNode));
  new_node->context = markov_context_copy(context);
  new_node->value = markov_value_add_word(NULL, word);
  new_node->next = node;
  return new_node;
}

/**
 * Prints a formatted string representation of all the data associated with a
 * linked list of MarkovNodes. Used for debugging.
*/
void markov_node_print(MarkovNode *node) {
  if (!node) { return; }
  printf("[\n");
  while (node) {
    printf("Context: ");
    markov_context_print(node->context);
    printf("\nValue: ");
    markov_value_print(node->value);
    printf("\n");

    node = node->next;
  }
  printf("]\n");
}

/**
 * Frees all the data associated with a linked list of MarkovNodes.
*/
void markov_node_free(MarkovNode *node) {
  if (!node) { return; }
  while (node) {
    markov_context_free(node->context);
    markov_value_free(node->value);
    MarkovNode *temp = node;
    node = node->next;
    free(temp);
  }
}

/**
 * A data structure representing a Markov chain. It implements a hash map so 
 * that data can be accessed in O(1) time instead of the O(n) time associated
 * with a linked list.
*/
typedef struct MarkovModel {
  size_t size;
  MarkovNode **nodes;
} MarkovModel;

/**
 * Returns a new MarkovModel instance of the specified size. Size describes the
 * number of buckets in the hashmap.
*/
MarkovModel *markov_model_new(size_t size) {
  MarkovModel *model = malloc(sizeof(MarkovModel));
  model->size = size;
  model->nodes = calloc(size, sizeof(MarkovNode*));
  return model;
}

/**
 * Adds a word and its context to the model. It finds the proper bucket and 
 * calls markov_node_add_node() with that data.
*/
void markov_model_add_data(MarkovModel *model, MarkovContext *context, char *word) {
  if (!model) {
    return;
  }
  size_t index = markov_context_get_hash(context) % model->size;
  model->nodes[index] = markov_node_add_node(model->nodes[index], context, word);
}

/**
 * Prints all the data associated with a MarkovModel. Used for debugging.
*/
void markov_model_print_data(MarkovModel *model) {
  if (!model) { return; }
  for (size_t i = 0; i < model->size; i++) {
    if (model->nodes[i]) {
      markov_node_print(model->nodes[i]);
      printf("\n");
    }
  }
}

/**
 * Frees all the data associated with a MarkovModel.
*/
void markov_model_free(MarkovModel *model) {
  if (!model) { return; }
  for (size_t i = 0; i < model->size; i++) {
    if (model->nodes[i]) {
      markov_node_free(model->nodes[i]);
    }
  }
  free(model->nodes);
  free(model);
}

/**
 * Loads the data from a provided file into a MarkovModel object and returns a
 * pointer to this object. The caller is responsible for freeing the 
 * MarkovModel.
*/
MarkovModel *markov_model_load_file(const char *file_name) {
  FILE *file = fopen(file_name, "r");
  if (!file) {
    perror("Unable to open file.");
    return NULL;
  }

  /** Initializes a new MarkovContext instance. This instance will be the 
   * running context tracker and will be copied to provide the MarkovContexts
   * specific to individual words */
  MarkovContext *context = markov_context_new();
  MarkovModel *model = markov_model_new(HASH_MAP_SIZE);

  char line[1024];
  while (fgets(line, sizeof(line), file)) {
    char *word = strtok(line, " \t\n\r");
    if (!word) {
      context = markov_context_reset(context);
    } else if (word[0] != '-'){
      while (word != NULL) {
        markov_model_add_data(model, context, word);
        markov_context_push_word(context, word);
        word = strtok(NULL, " \t\n\r");
      }
    }
  }

  markov_context_free(context);
  fclose(file);

  return model;
}

/**
 * When given a context, returns a possible next word based upon the data in a
 * provided model. The caller is responsible for updating the context.
*/
char *markov_model_get_next(MarkovModel *model, MarkovContext *context) {
  MarkovNode *node = model->nodes[markov_context_get_hash(context) % model->size];
  char *word = NULL;
  while (node) {
    if (markov_context_check_match(node->context, context)) {
      word = markov_value_get_random(node->value);
      return word;
    }
    node = node->next;
  }
  return word;
}

/**
 * Returns true if a word contains specific end conditions (. or ! or ?).
*/
bool check_end_condition(char *word) {
  if (strchr(word, '.') || strchr(word, '!') || strchr(word, '?')) {
    return true;
  }
  return false;
}

/**
 * Returns a pointer to a given quote with the given word appended. The caller
 * is responsible for freeing the quote.
*/
char *add_word_to_quote(char *quote, char *word) {
  if (quote == NULL || word == NULL) {
    return quote;
  }
  size_t length = strlen(quote) + strlen(word) + 2;
  quote = realloc(quote, length);
  if (quote[0] != '\0') {
    strcat(quote, " ");
  }
  strcat(quote, word);
  return quote;
}

/**
 * Returns a quote based upon the contained in the given MarkovModel.
*/
char *markov_model_generate_quote(MarkovModel *model) {
  MarkovContext *context = markov_context_new();
  size_t counter = 0;
  char *quote = calloc(1, 1);
  while (counter <= MAX_QUOTE_LENGTH) {
    char *word = markov_model_get_next(model, context);
    context = markov_context_push_word(context, word);
    if (!word) { break; }
    quote = add_word_to_quote(quote, word);
    if (check_end_condition(word)) { break; }
    counter++;
  }
  markov_context_free(context);
  return quote;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  srand(time(NULL));

  MarkovModel *model = markov_model_load_file(FILE_NAME);

  char *quote = markov_model_generate_quote(model);
  printf("\n%s\n\n", quote);
  free(quote);

  markov_model_free(model);
  return EXIT_SUCCESS;
}
