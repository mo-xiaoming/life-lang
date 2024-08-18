#ifndef SPIRIT_X3_HPP__
#define SPIRIT_X3_HPP__

#if !defined(BOOST_SPIRIT_X3_THROW_EXPECTATION_FAILURE) || BOOST_SPIRIT_X3_THROW_EXPECTATION_FAILURE == 0
#define BOOST_SPIRIT_X3_THROW_EXPECTATION_FAILURE 0
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#else
#error "BOOST_SPIRIT_X3_THROW_EXPECTATION_FAILURE is already defined"
#endif

#endif