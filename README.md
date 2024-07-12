# Marko Polo

A simple quote generator based on markov chains and written in C.

# Usage

No command line args here. Jump in to main.c and change the parameters there. 
There are a few things you can change:

- FILE_NAME: The file to train the model.
- MARKOV_CONTEXT_SIZE: The number of words to use for context.
- MAX_QUOTE_LENGTH: The maximum number of words allowed in an outputted quote.
- HASH_MAP_SIZE: The number of buckets used by the hash map.
