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

#ifndef DCM_TEST_PARSER_H
#define DCM_TEST_PARSER_H

#include <boost/test/unit_test.hpp>

#include "opendcm/core.hpp"
#include "opendcm/modulestate.hpp"
#include "opendcm/externalize.hpp"

#include <iosfwd>
#include <sstream>

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>



namespace karma = boost::spirit::karma;

struct TestModule1 {

    template<typename Sys>
    struct type {
        typedef mpl::map0<> signal_map;

        struct test_object1 : public dcm::Object<Sys, test_object1, signal_map > {
            test_object1(Sys& system) : dcm::Object<Sys, test_object1, signal_map >(system) { };
        };
        struct test_object2 : public dcm::Object<Sys, test_object2, signal_map > {
            test_object2(Sys& system) : dcm::Object<Sys, test_object2, signal_map >(system) { };
        };

        struct inheriter {};

        struct test_object1_prop {
            typedef int type;
            typedef test_object1 kind;
        };
        struct test_object2_prop {
            typedef std::string type;
            typedef test_object1 kind;
        };
        struct test_vertex1_prop {
            typedef std::string type;
            typedef dcm::vertex_property kind;
        };
        struct test_edge1_prop {
            typedef int type;
            typedef dcm::edge_property kind;
        };

        typedef mpl::vector2<test_object1, test_object2> objects;
        typedef mpl::vector4<test_object1_prop, test_object2_prop, test_vertex1_prop, test_edge1_prop>   properties;
	typedef dcm::Unspecified_Identifier Identifier;
	
        static void system_init(Sys& sys) {};
    };
};

struct test_prop {
    typedef std::string type;
    typedef int kind;
};

namespace dcm {

//template<typename System, typename iterator>
//struct parser_generator< typename TestModule1::type<System>::test_object1_prop, System, iterator > {

//    typedef rule<iterator, int()> generator;
//    static void init(generator& r) {
//        r = lit("<type>test o1 property</type>\n<value>")<<int_<<"</value>";
//    };
//};

template<typename System>
struct parser_generate< typename TestModule1::type<System>::test_vertex1_prop, System>
  : public mpl::true_{};
  
template<typename System, typename iterator>
struct parser_generator< typename TestModule1::type<System>::test_vertex1_prop, System, iterator > {

    typedef karma::rule<iterator, std::string()> generator;
    static void init(generator& r) {
        r = karma::lit("<type>vertex 1 prop</type>\n<value>")<<boost::spirit::ascii::string<<"</value>";
    };
};

template<typename System>
struct parser_parse< typename TestModule1::type<System>::test_vertex1_prop, System>
  : public mpl::true_{};
  
template<typename System, typename iterator>
struct parser_parser< typename TestModule1::type<System>::test_vertex1_prop, System, iterator > {

    typedef qi::rule<iterator, std::string(), qi::space_type> parser;
    static void init(parser& r) {
        r = qi::lexeme[qi::lit("<type>vertex 1 prop</type>")] >> ("<value>") 
	>> -(+qi::char_("a...zA...Z0..9")) >> ("</value>");
    };
};



template<typename System>
struct parser_generate< typename TestModule1::type<System>::test_edge1_prop, System>
  : public mpl::true_{};
  
template<typename System, typename iterator>
struct parser_generator< typename TestModule1::type<System>::test_edge1_prop, System, iterator > {

    typedef karma::rule<iterator, int()> generator;
    static void init(generator& r) {
        r = karma::lit("<type>edge 1 prop</type>\n<value>")<<karma::int_<<"</value>";
    };
};

template<typename System>
struct parser_parse< typename TestModule1::type<System>::test_edge1_prop, System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_parser< typename TestModule1::type<System>::test_edge1_prop, System, iterator > {
  
    typedef qi::rule<iterator, int(), qi::space_type> parser;
    static void init(parser& r) {
        r = qi::lexeme[qi::lit("<type>edge 1 prop</type>")] >> ("<value>") 
	    >> qi::int_ >> ("</value>");
    };
};



template<typename System>
struct parser_generate< typename TestModule1::type<System>::test_object1, System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_generator< typename TestModule1::type<System>::test_object1, System, iterator > {

    typedef karma::rule<iterator, boost::shared_ptr<typename TestModule1::type<System>::test_object1>() > generator;
    static void init(generator& r) {
        r = boost::spirit::ascii::string[karma::_1="<type>object 1 prop</type>\n<value>HaHAHAHAHA</value>"];
    };
};

template<typename System>
struct parser_parse< typename TestModule1::type<System>::test_object1, System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_parser< typename TestModule1::type<System>::test_object1, System, iterator > {

    typedef typename TestModule1::type<System>::test_object1 object_type;
    
    typedef qi::rule<iterator, boost::shared_ptr<object_type>(System*), qi::space_type> parser;
    static void init(parser& r) {
        r = qi::lexeme[qi::lit("<type>object 1 prop</type>")[ qi::_val = 
		  phx::construct<boost::shared_ptr<object_type> >( phx::new_<object_type>(*qi::_r1))]] >> ("<value>HaHAHAHAHA</value>");
    };
};
}

typedef dcm::Kernel<double> Kernel;
typedef dcm::System<Kernel, dcm::ModuleState::type, TestModule1::type> System;

DCM_EXTERNALIZE( System )

#endif //DCM_PARSER_H