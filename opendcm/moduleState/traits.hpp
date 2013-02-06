/*
    openDCM, dimensional constraint manager
    Copyright (C) 2013  Stefan Troeger <stefantroeger@gmx.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_PARSER_TRAITS_H
#define DCM_PARSER_TRAITS_H

#include <boost/mpl/bool.hpp>

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_int.hpp>
#include <boost/spirit/include/karma_bool.hpp>
#include <boost/spirit/include/karma_rule.hpp>
#include <boost/spirit/include/karma_auto.hpp>

namespace boost { namespace spirit { namespace traits
{
    template <>
    struct create_generator<dcm::No_Identifier> {
      
        typedef BOOST_TYPEOF(karma::eps(false)) type;
        static type call()  {
            return karma::eps(false);
        }
    };
}}}

namespace dcm {

using boost::spirit::karma::rule;
namespace karma = boost::spirit::karma;

template<typename type, typename System>
struct parser_generate : public mpl::false_ {};

template<typename type, typename System, typename iterator>
struct parser_generator {
    typedef rule<iterator> generator;

    static void init(generator& r) {
        r = karma::eps(false);
    };
};

template<typename System>
struct parser_generate<type_prop, System> : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator<type_prop, System, iterator> {
    typedef rule<iterator, int()> generator;

    static void init(generator& r) {
        r = karma::lit("<type>clustertype</type>\n<value>") << karma::int_ <<"</value>";
    };
};

template<typename System>
struct parser_generate<changed_prop, System> : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator<changed_prop, System, iterator> {
    typedef rule<iterator, bool()> generator;

    static void init(generator& r) {
        r = karma::lit("<type>clusterchanged</type>\n<value>") << karma::bool_ <<"</value>";
    };
};

template<typename System>
struct parser_generate<id_prop<typename System::Identifier>, System> 
: public mpl::not_<boost::is_same<typename System::Identifier, No_Identifier> >{};

template<typename System, typename iterator>
struct parser_generator<id_prop<typename System::Identifier>, System, iterator> {
    typedef rule<iterator, typename System::Identifier()> generator;

    static void init(generator& r) {
        r = karma::lit("<type>id</type>\n<value>") << karma::auto_ <<"</value>";
    };
};

}
#endif //DCM_PARSER_TRAITS_H
