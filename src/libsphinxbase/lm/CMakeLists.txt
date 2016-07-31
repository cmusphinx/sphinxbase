SET(SOURCES	fsg_model.c
			jsgf.c
			jsgf_parser.c
			jsgf_scanner.c
			lm_trie.c
			lm_trie_quant.c
			ngram_model.c
			ngram_model_set.c
			ngram_model_trie.c
			ngrams_raw.c
	)
	
add_library(lm OBJECT ${SOURCES})