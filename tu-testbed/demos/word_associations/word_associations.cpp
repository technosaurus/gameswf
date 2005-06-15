// word_associations.cpp -- by Thatcher Ulrich http://tulrich.com 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Program to discover word associations within a corpus of text.


#include "base/container.h"
#include "base/tu_random.h"
#include <stdio.h>
#include <stdlib.h>


void print_usage()
{
	printf("word_associations -- discover word associations in a corpus of text\n");
	printf("\n");
	printf("Send your text in on stdin.  The program outputs a list of word-like\n");
	printf("tokens, followed by a table of the most common co-occuring words for\n");
	printf("each word in the token table.\n");
}


// Known tokens & their stats.
array<tu_string> s_tokens;
array<int> s_occurrences;
string_hash<int> s_token_index;

struct associated_word
{
	int m_id;
	float m_weight;

	// For qsort
	static int compare_weight(const void* a, const void* b)
	{
		float wa = static_cast<const associated_word*>(a)->m_weight;
		float wb = static_cast<const associated_word*>(b)->m_weight;
		
		// Sort larger to smaller.
		if (wb > wa) return -1;
		else if (wa > wb) return 1;
		else return 0;
	}
};

const int MAX_STATS = 100;

array< array<associated_word> > s_stats;



int get_token_id(const tu_string& word)
// Look up or add the given word, and return its id number.
{
	int id = -1;
	if (s_token_index.get(word, &id))
	{
		// Already in the table.
		s_occurrences[id]++;
		return id;
	}

	// Must add the word to the table.
	id = s_tokens.size();
	s_tokens.push_back(word);
	s_occurrences.push_back(1);
	s_stats.resize(s_tokens.size());
	s_token_index.add(word, id);

	// xxxx debugging
	if (id < 20 || (id % 1000) == 0)
	{
		fprintf(stderr, "Token %d is '%s'\n", id, word.c_str());
	}

	return id;
}


// Keep a FIFO history of the last N tokens, for finding
// co-occurrences as they come in.
const int WINDOW_BITS = 4;
const int WINDOW_SIZE = (1 << WINDOW_BITS);
const int WINDOW_MASK = WINDOW_SIZE - 1;
int s_window[WINDOW_SIZE];

// s_current_token points at the latest addition to the window.
int s_current_token = 0;


void clear_window()
// Clear the window FIFO.
{
	for (int i = 0; i < WINDOW_SIZE; i++)
	{
		s_window[i] = -1;
	}
}


bool is_word_separator(int c)
// Return true if the given character is a valid word-separator.
// Basically, it's a separator if it's whitespace or '-'
{
	if (c == ' '
	    || c == '\t'
	    || c == '\n'
	    || c == '\r'
	    || c == '/'
	    || c == '-')
	{
		return true;
	}

	return false;
}


bool is_trailing_punctuation(int c)
// Return true if the character is punctuation that often appears at
// the end of a word (i.e. periods, parens, etc).
{
	if (c == '.'
	    || c == ':'
	    || c == ';'
	    || c == ')'
	    || c == '\''
	    || c == '\"'
	    || c == '!'
	    || c == '?'
	    || c == ','
	    || c == '/')
	{
		return true;
	}

	return false;
}


bool validate_word(char* wordbuf)
// Return true if the word is valid, and munges the word to fix simple
// things (like trailing punctuation).  Things that make a word
// invalid are:
// 
// * less than 3 characters long
//
// * contains numbers or bad punctuation
{
	assert(wordbuf);

	int len = strlen(wordbuf);

	// Chop off trailing punctuation.
	while (len > 0 && is_trailing_punctuation(wordbuf[len - 1]))
	{
		wordbuf[len - 1] = 0;
		len--;
	}

	// Chop off trailing "'s", "Peter's" --> "Peter"
	if (len > 1 && wordbuf[len - 2] == '\'')
	{
		wordbuf[len - 2] = 0;
		len -= 2;
	}

	if (len < 3)
	{
		// Too short.
		return false;
	}

	assert(wordbuf[len] == 0);

	// Scan to make sure chars are all alphabetic.  Also lowercase
	// the whole thing.
	for (int i = 0; i < len; i++)
	{
		int c = tolower(wordbuf[i]);
		if (c < 'a' || c > 'z')
		{
			// Non alphabetic character; this is not a
			// valid word.
			return false;
		}
		wordbuf[i] = c;
	}

	return true;
}


bool get_next_token(FILE* in)
// Read the next word-like token from the input stream.  Add the token
// to the token table and the input window as appropriate.  Possibly
// clears the window if we hit some kind of obvious document boundary.
{
	// (possibly add to token table, clear window if we hit a boundary, etc)

	bool skipping = false;
	
	static const int MAX_WORD = 10;
	char buf[MAX_WORD + 1];
	int next_char = 0;

	for (;;)
	{
		int c = fgetc(in);
		if (c == EOF)
		{
			// TODO: Finish current word?
			return false;
		}

		if (is_word_separator(c))
		{
			// Finish current word.
			
			if (skipping)
			{
				skipping = false;
				continue;
			}

			// Terminate the word string.
			assert(next_char <= MAX_WORD);
			buf[next_char] = 0;

			if (validate_word(buf))
			{
				// Valid: add to the window.
				int token_id = get_token_id(tu_string(buf));
				s_current_token = (s_current_token + 1) & WINDOW_MASK;
				s_window[s_current_token] = token_id;

				return true;
			}
			else
			{
				// Not a valid word; keep scanning.
				next_char = 0;
				continue;
			}
		}
		else if (skipping)
		{
			// We're ignoring the current word.
			continue;
		}
		else
		{
			// Process the character.
			
			if (next_char >= MAX_WORD)
			{
				// This word is too long; ignore it.
				next_char = 0;
				skipping = true;
			}
			else
			{
				buf[next_char] = c;
				next_char++;
			}
		}
	}
}


void add_weight(int w0, int w1, float weight)
// Add weight to the co-occurrence score of w0[w1].
{
	array<associated_word>& a = s_stats[w0];

	// Is w1 already in the stats for w0?
	for (int i = 0, n = a.size(); i < n; i++)
	{
		if (a[i].m_id == w1)
		{
			// Add our weight.
			a[i].m_weight += weight;
			return;
		}
	}

	if (a.size() < MAX_STATS)
	{
		associated_word aw;
		aw.m_id = w1;
		aw.m_weight = weight;

		a.push_back(aw);
	}
	else
	{
		// Random replacement of a low-weighted entry.
		for (int i = 0; i < 10; i++)
		{
			int ai = tu_random::next_random() % MAX_STATS;
			float p = tu_random::get_unit_float();
			if (p >= a[ai].m_weight)
			{
				// Replace this unlucky entry.
				a[ai].m_id = w1;
				a[ai].m_weight = weight;
				return;
			}
		}
		// else we just drop this weight.
	}
}


void process_token()
// Update statistics on the current token.
{
	int curr = s_window[s_current_token];
	assert(curr != -1);

	for (int i = 1; i < WINDOW_SIZE; i++)
	{
		int window_i = (s_current_token + i) & WINDOW_MASK;
		int wi = s_window[window_i];
		if (wi <= 0) continue;

		float weight = 1.0f / i;

		add_weight(curr, wi, weight);
		add_weight(wi, curr, weight);
	}
}

 
void process_input(FILE* in)
// Scan stdin until it runs out.  For each word-like token in the
// input stream, keep statistics on other tokens that appear near it.
{
	for (;;) {
		if (get_next_token(in) == false)
		{
			break;
		}

		process_token();
	}
}


void print_tables()
// Print the token table, and the co-occurrences table.
{
	// Sort by frequency.
	// TODO qsort(...);

	// Print the co-occurrences of the N most frequent tokens.
	for (int i = 0; i < 100 && i < s_tokens.size(); i++)
	{
		printf("%s: ", s_tokens[i].c_str());

		array<associated_word>& a = s_stats[i];
		qsort(&a[0], a.size(), sizeof(a[0]), associated_word::compare_weight);
		for (int j = 0; j < a.size(); j++)
		{
			printf("%s ", s_tokens[a[j].m_id].c_str());
		}
		printf("\n");
	}
}


int	main(int argc, char *argv[])
{
	clear_window();

	process_input(stdin);
	print_tables();

	return 0;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
