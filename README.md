# In-Memory Search Engine in C

A lightweight, in-memory search engine and inverted index written in C. 

The project indexes documents and keywords in memory, allowing fast keyword lookups, autocomplete-style prefix matching, multi-word Boolean searches (`AND` / `OR`), and relevance ranking.

---

## How It Works (Data Structures)

Instead of doing slow linear searches through every document, the engine uses classic data structures to keep searches fast:

- **Trie (Prefix Tree)**: Stores all keywords character by character. Looking up a keyword takes time proportional only to the word's length, regardless of how many documents are indexed.
- **Posting Lists (Linked Lists)**: Each keyword node in the Trie maintains a list of references to the documents containing that word, along with the keyword's Term Frequency (occurrence count).
- **Max-Heap**: Used to efficiently extract the top-$K$ documents with the highest scores without having to sort the entire dataset.

---

## Features

- **Interactive CLI**: Run the program directly in the terminal with a friendly prompt (`search> `).
- **Exact Keyword Search**: Quickly find documents containing a specific word.
- **Prefix Matching**: Search for keywords starting with a prefix (e.g., `alg` matches `algorithm`).
- **Boolean Search (`AND` / `OR`)**: Multi-keyword queries (e.g., `python AND algorithms` or `python OR rust`).
- **Dynamic Relevance Ranking**: Ranks search results by combining the document's base score with how many times the keyword appears ($\text{Relevance} = \text{Base Score} + \text{TF} \times 10$).
- **Top-K Retrieval**: Extracts the highest-scoring documents using the binary Max-Heap.
- **Batch Mode**: Can also execute automated commands from an input file.

---

## How to Compile & Run

### 1. Compilation
Using `make`:
```bash
make build
```

Or compiling directly with `gcc`:
```bash
gcc -Wall -Wextra -std=c99 -g *.c -o search_index
```

### 2. Running
To start the **interactive shell**:
```bash
./search_index
```

To run **batch commands from a file**:
```bash
./search_index input_file.in
```

---

## Example Usage

```text
search> add doc1 80 python c
[OK] Document 'doc1' added (Score: 80, 2 keyword(s)).

search> add doc2 95 python algorithms
[OK] Document 'doc2' added (Score: 95, 2 keyword(s)).

search> add doc3 70 algorithms rust
[OK] Document 'doc3' added (Score: 70, 2 keyword(s)).

search> search python AND algorithms
Found 1 document(s) (Boolean Mode: AND):
  - doc2            (Base Score:  95, TF:  1)

search> search python OR rust
Found 3 document(s) (Boolean Mode: OR):
  - doc1            (Base Score:  80, TF:  1)
  - doc2            (Base Score:  95, TF:  1)
  - doc3            (Base Score:  70, TF:  1)

search> rank algorithms
Ranked Results (Mode: AND | Relevance = Base + TF * 10):
  1. doc2            [Relevance: 105 | Base Score:  95 | Total TF:  1]
  2. doc3            [Relevance:  80 | Base Score:  70 | Total TF:  1]

search> topk python 2
Top 2 ranked document(s) for 'python':
  1. [Score:  95] doc2
  2. [Score:  80] doc1

search> prefix alg
Found 2 document(s) matching prefix 'alg':
  - doc2
  - doc3

search> stats
--- Search Engine Statistics ---
  Indexed Documents : 3
  Unique Keywords   : 4
  Trie Nodes Count  : 31
--------------------------------

search> exit
Exiting search engine. Goodbye!
```

---

## Available Commands

| Command | Usage | Description |
| :--- | :--- | :--- |
| `add` | `add <id> <score> <kw1> [kw2...]` | Add a document with a score and keywords |
| `del` | `del <id>` | Remove a document from the index |
| `addkw` | `addkw <id> <keyword>` | Add a keyword to an existing document |
| `delkw` | `delkw <id> <keyword>` | Remove a keyword from a document |
| `search` | `search <kw1> [AND\|OR kw2...]` | Search for keywords with Boolean logic |
| `rank` | `rank <query>` | Rank results by relevance (Base Score + Term Frequency) |
| `topk` | `topk <keyword> <k>` | Show top K documents by score using the Max-Heap |
| `prefix` | `prefix <prefix>` | Search keywords starting with a prefix |
| `tf` | `tf <id> <keyword>` | View occurrence count of a keyword in a document |
| `print` | `print` | Dump the complete dictionary and matching documents |
| `stats` | `stats` | View index statistics (documents, keywords, Trie nodes) |
| `help` | `help` | Show available commands |
| `exit` | `exit` | Quit the program |
