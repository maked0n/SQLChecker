#include <string>
#include <cmath>
#include <vector>
#include <cctype>
#include "cquery.h"
#include "utility.h"

namespace Analyzer {
	class CLexer {
		private:
			static constexpr double m_scale_prefix = 0.1;
			static constexpr double m_boost_threshold = 0.7;
			static constexpr double m_minimal_distance = 0.5;
			double m_score;
			static double _get_distance(const std::string& first, const std::string& second) {
				int matching_characters = 0, transpositions = 0;
				short common_prefix = 0;
				double d_j = 0.0;
				std::vector<bool> first_match(first.size());
				std::vector<bool> second_match(second.size());
				for(size_t i = 0; i < first.size(); ++i) first_match[i] = false;
				for(size_t i = 0; i < second.size(); ++i) second_match[i] = false;
				const int match_dist = std::floor(std::max(first.size(), second.size()) / 2) - 1;
				for(size_t i = 0; i < first.size(); ++i) { 
					int start = std::max(0, (int)(i - match_dist));
					int end = std::min(i + match_dist + 1, second.size());
					for(int j = start; j < end; ++j) {
						if(second_match[j]) continue;
						if(first[i] != second[j]) continue;
						first_match[i] = true;
						second_match[j] = true;
						++matching_characters;
					}
				}
				for(int i = 0; i < 4; ++i) common_prefix += (short)(first_match[i] & second_match[i]);
				int k = 0;
				for(size_t i = 0; i < first.size(); ++i) {
					if(!first_match[i]) continue;
					while(!second_match[k]) ++k;
					if(first_match[i] != second_match[k]) ++transpositions;
					++k;
				}
				transpositions /= 2.0;
				if(matching_characters == 0) d_j = 0.0;
				else d_j = (matching_characters / first.size() + matching_characters / second.size() 
						+ (matching_characters - transpositions) / matching_characters) / 3.0;
				if(d_j < m_boost_threshold) return d_j;
				else return d_j + common_prefix * m_scale_prefix * (1 - d_j);
			}
			static void _next_alphabet(std::string::const_iterator& begin, std::string::const_iterator& end, 
					std::string* log = NULL) {
				while(!isalpha(*end)) {
					if(log) log->push_back(*end);
					begin = ++end;
				}
			}
			static std::string _get_next_keyword(std::string::const_iterator& begin, std::string::const_iterator& end) {
				if(begin != end) begin = ++end; //always increment iterator except they both point to the beginning of the string
				while((*end) != '|' && end != Utility::KEYWORDS.cend()) ++end;
				return std::string(begin, end);
			}
			static unsigned int _count_words(const std::string& query) {
				int words_num = 0;
				std::string::const_iterator begin = query.cbegin();
				std::string::const_iterator end = query.cbegin();
				_next_alphabet(begin, end);
				while(end != query.end()) {
					if(!isalpha(*(++end))) {
						++words_num;
						_next_alphabet(begin, end);
					}
				}
				if(begin != query.end() - 1) ++words_num;
				return words_num;
			}
			std::string _get_synonym(const std::string& input, double penalty) {
				double max_dist = 0.0;
				double dist = 0.0;
				std::string::const_iterator keywords_begin = Utility::KEYWORDS.cbegin();
				std::string::const_iterator keywords_end = Utility::KEYWORDS.cbegin();
				std::string keyword = _get_next_keyword(keywords_begin, keywords_end);
				while(!keyword.empty()) {
					dist = _get_distance(input, keyword);
					if(max_dist < dist) max_dist = dist;
					if(max_dist == 1) break;
					keyword = _get_next_keyword(keywords_begin, keywords_end);
				}
				if(max_dist - m_minimal_distance < Utility::EPS) return input;
				m_score -= ((max_dist - m_minimal_distance) / (1 - m_minimal_distance)) * penalty;
				return keyword;
			}
		public:
			CAnalyzer() : m_score(100.0) {}	//TODO: construct from CQuery and CParser
			void tokenize(const CQuery& query) {
				std::string s_query = query.get_query();
				double penalty = Utility::LEXICAL_PENALTY_PERCENTAGE / _count_words(s_query);
				std::string::const_iterator begin = s_query.cbegin();
				std::string::const_iterator end = s_query.cbegin();
				_next_alphabet(begin, end, &tmp_query);
				while(end != s_query.cend()) {
					if(!isalpha(*end)) {
						query.add_word(_get_synonym(std::string(begin, end), penalty));
						_next_alphabet(begin, end);
					}
					++end;
				}
			}
	};
}
