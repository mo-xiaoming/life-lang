#ifndef CONFIG_HPP__
#define CONFIG_HPP__

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

namespace client::parser {
namespace x3 = boost::spirit::x3;
using IteratorType = std::string::const_iterator;
using SpaceType = x3::ascii::space_type;
// struct PositionCacheTag;
// using PositionCacheType = boost::spirit::x3::position_cache<std::vector<IteratorType>>;
using ErrorHandlerType = x3::error_handler<IteratorType>;
using PhraseContextType = x3::phrase_parse_context<SpaceType>::type;
using ContextType = x3::context<x3::error_handler_tag, std::reference_wrapper<ErrorHandlerType>, PhraseContextType>;
}  // namespace client::parser

#endif