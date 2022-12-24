
/*
Tipue Search 7.1
Copyright (c) 2019 Tipue
Tipue Search is released under the MIT License
http://www.tipue.com/search
*/


/*
Stop words
Stop words list from http://www.ranks.nl/stopwords
*/

var tipuesearch_stop_words = ["a", "above", "after", "again", "against", "all", "am", "an", "and", "any", "are", "aren't", "as", "at", "be", "because", "been", "before", "being", "below", "between", "both", "but", "by", "can't", "cannot", "could", "couldn't", "did", "didn't", "do", "does", "doesn't", "doing", "don't", "down", "during", "each", "few", "for", "from", "further", "had", "hadn't", "has", "hasn't", "have", "haven't", "having", "he", "he'd", "he'll", "he's", "her", "here", "here's", "hers", "herself", "him", "himself", "his", "how", "how's", "i", "i'd", "i'll", "i'm", "i've", "if", "in", "into", "is", "isn't", "it", "it's", "its", "itself", "let's", "me", "more", "most", "mustn't", "my", "myself", "no", "nor", "not", "of", "off", "on", "once", "only", "or", "other", "ought", "our", "ours", "ourselves", "out", "over", "own", "same", "shan't", "she", "she'd", "she'll", "she's", "should", "shouldn't", "so", "some", "such", "than", "that", "that's", "the", "their", "theirs", "them", "themselves", "then", "there", "there's", "these", "they", "they'd", "they'll", "they're", "they've", "this", "those", "through", "to", "too", "under", "until", "up", "very", "was", "wasn't", "we", "we'd", "we'll", "we're", "we've", "were", "weren't", "what", "what's", "when", "when's", "where", "where's", "which", "while", "who", "who's", "whom", "why", "why's", "with", "won't", "would", "wouldn't", "you", "you'd", "you'll", "you're", "you've", "your", "yours", "yourself", "yourselves"];


// Word replace

var tipuesearch_replace = {'words': [
     // Not relevant for PasDoc docs: {'word': 'tip', 'replace_with': 'tipue'},
     // Not relevant for PasDoc docs: {'word': 'javscript', 'replace_with': 'javascript'},
     // Not relevant for PasDoc docs: {'word': 'jqeury', 'replace_with': 'jquery'}
]};


// Weighting

var tipuesearch_weight = {'weight': [
     // Not relevant for PasDoc docs: {'url': 'http://www.tipue.com', 'score': 60},
     // Not relevant for PasDoc docs: {'url': 'http://www.tipue.com/search', 'score': 60},
     // Not relevant for PasDoc docs: {'url': 'http://www.tipue.com/tipr', 'score': 30},
     // Not relevant for PasDoc docs: {'url': 'http://www.tipue.com/support', 'score': 20}
]};


// Illogical stemming

var tipuesearch_stem = {'words': [
     // Not relevant for PasDoc docs: {'word': 'e-mail', 'stem': 'email'},
     // Not relevant for PasDoc docs: {'word': 'javascript', 'stem': 'jquery'},
     // Not relevant for PasDoc docs: {'word': 'javascript', 'stem': 'js'}
]};


// Related
var tipuesearch_related = {'Related': [
     // Not relevant for PasDoc docs: {'search': 'tipue', 'related': 'Search', 'include': 1},
     // Not relevant for PasDoc docs: {'search': 'tipue', 'related': 'jQuery'},
     // Not relevant for PasDoc docs: {'search': 'tipue', 'related': 'Blog'},
     // Not relevant for PasDoc docs: {'search': 'tipue', 'related': 'Support'},
     // Not relevant for PasDoc docs: {'search': 'tipue search', 'related': 'Demo', 'include': 1},
     // Not relevant for PasDoc docs: {'search': 'tipue search', 'related': 'Support'}
]};


// Internal strings

var tipuesearch_string_1 = 'No title';
var tipuesearch_string_2 = 'Showing results for';
var tipuesearch_string_3 = 'Search instead for';
var tipuesearch_string_4 = '1 result';
var tipuesearch_string_5 = 'results';
var tipuesearch_string_6 = '<';
var tipuesearch_string_7 = '>';
var tipuesearch_string_8 = 'Nothing found.';
var tipuesearch_string_9 = 'Common words are largely ignored.';
var tipuesearch_string_10 = 'Related';
var tipuesearch_string_11 = 'Search should be one character or more.';
var tipuesearch_string_12 = 'Search should be';
var tipuesearch_string_13 = 'characters or more.';
var tipuesearch_string_14 = 'seconds';
var tipuesearch_string_15 = 'Open Image';
var tipuesearch_string_16 = 'Goto Page';


// Internals


// Timer for showTime

var startTimer = new Date().getTime();

