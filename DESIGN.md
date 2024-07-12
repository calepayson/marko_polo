# Markov Design Document

## Data Structures

### MarkovContext

**Description:** 
Holds an array of previous words of size MARKOV_CONTEXT_SIZE

**Example:**
MarkovContext {
    previous_words = [NULL, "Hello", "World"]
}

**Methods:**
- markov_context_new
    - Description: Returns a NULLed MarkovContext
    - Takes: void
    - Returns: MarkovContext
- markov_context_reset
    - Description: Returns a MarkovContext with its paramaters set to NULL
    - Takes: MarkovContext
    - Returns: MarkovContext
- markov_context_push_word
    - Description: Takes a MarkovContext (or NULL) and a word and adds the word to the MarkovContext. If MarkovContext is NULL, allocates a new MarkovContext
    - Takes: MarkovContext *, char *
    - Returns: MarkovContext *
- markov_context_print_
    - Description: Debugging function used to print a markov context
    - Takes: MarkovContext *
    - Returns: void
- markov_context_free
    - Description: Frees all memory associated with the provided MarkovContext
    - Takes: MarkovContext *
    - Returns: void
- markov_context_get_hash
    - Description: Returns a hash of a given markov context. Based on djb2
    - Takes: MarkovContext *
    - Returns: size_t
- markov_context_check_match
    - Description: Checks if two provided MarkovContexts are a match and returns a bool of the result
    - Takes: MarkovContext *, MarkovContext *
    - Returns: bool
- markov_context_copy
    - Description: Returns a deep copy of the provided MarkovContext
    - Takes: MarkovContext *
    - Returns: MarkovContext *

### MarkovValue

**Description:** 
A linked-list structure that holds words and their counts

**Example:**
MarkovValue {
    word = "Hello"
    count = 4
    next = NULL
}

**Methods:**
- markov_value_add_word
    - Description: Takes a MarkovValue or NULL and returns a linked list of MarkovValues after either incrementing the counter on an existing node or creating a new node.
    - Takes: MarkovValue *, char *
    - Returns: MarkovValue *
- markov_value_print
    - Description: Debugging function used to print all the items in a MarkovValue linked list
    - Takes: MarkovValue *
    - Returns: void
- markov_value_free_all
    - Description: Takes a MarkovValue linked list and frees all nodes
    - Takes: MarkovValue *
    - Returns: void

### MarkovNode

**Description:**
A linked list object containing references to a MarkovContext and its associated MarkovValues

**Example:**
MarkovNode {
    context = MarkovContext *
    value = MarkovValue *
    next = NULL
}

**Methods:**
- markov_node_add_node
    - Description: Takes all the data needed for a node and either adds it to the node with the proper context or appends a new node to the list
    - Takes: MarkovNode *, MarkovContext *, char *
    - Returns: MarkovNode *
- markov_node_print
    - Description: Debugging function used to print all the data in a MarkovNode linked list structure
    - Takes: MarkovNode *
    - Returns: void
- markov_node_free
    - Descritpion: Frees all the data associated with a MarkovNode linked list structure
    - Takes: MarkovNode *
    - Returns: void

### MarkovModel

**Description:**
A Hashmap pointing to MarkovNode linked lists

**Example:**
MarkovModel {
    size = 2
    nodes = [NULL, MarkovNode *]
}

**Methods:**
- markov_model_new
    - Description: Returns a new MarkovModel of the provided size
    - Takes: usize
    - Returns: MarkovModel *
- markov_model_add_data
    - Description: Takes a MarkovContext and a word and adds them to the model
    - Takes: MarkovContext *, char *
    - Returns: void
- markov_model_print_data
    - Description: Debugging function used to print all the data associated with a given MarkovModel
    - Takes: MarkovModel *
    - Returns: void
- markov_model_free
    - Description: Frees all data associated witha MarkovModel
    - Takes: MarkovModel
    - Returns: void
    








