/*
    openDCM, dimensional constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

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

#include "opendcm/Core"
#include "opendcm/Module3D"

#include "test/Octave/debugsolver.hpp"

#include <time.h>
#include <iostream>
#include <iomanip>

#include <boost/test/unit_test.hpp>

struct point : std::vector<double> {};

namespace dcm {

template<>
struct geometry_traits<point> {
    typedef tag::direction3D tag;
    typedef modell::XYZ modell;
    typedef orderd_bracket_accessor accessor;
};

template<>
struct geometry_traits<Eigen::Vector3d> {
    typedef tag::direction3D tag;
    typedef modell::XYZ modell;
    typedef orderd_roundbracket_accessor accessor;
};
}

//again, two vectors perpendicular, maybe the easiest constraints of them all
template< typename Kernel, typename Tag1, typename Tag2 >
struct test_constraint { };

template< typename Kernel >
struct test_constraint<Kernel, dcm::tag::direction3D, dcm::tag::direction3D> {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    Scalar calculate(Vector& param1,  Vector& param2) {
        return param1.dot(param2);
    };
    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {

        return dparam1.dot(param2);
    };
    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {

        return param1.dot(dparam2);
    };
    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {

        gradient(0) = param2(0);
        gradient(1) = param2(1);
        gradient(2) = param2(2);
    };
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {

        gradient(0) = param1(0);
        gradient(1) = param1(1);
        gradient(2) = param1(2);
    };
};


using namespace dcm;

BOOST_AUTO_TEST_SUITE(Module3D_test_suit);

typedef dcm::Kernel<double> Kernel;
typedef Module3D< mpl::vector<point, Eigen::Vector3d> > Module;
typedef System<Kernel, Module::type> System;
typedef typename Module::type<System>::Geometry3D geom;
typedef boost::shared_ptr<geom> geom_ptr;

typedef typename Module::type<System>::Constraint3D cons;
typedef boost::shared_ptr<cons> cons_ptr;

typedef typename System::Cluster::vertex_iterator viter;
typedef typename Module::type<System>::vertex_prop vertex_prop;

BOOST_AUTO_TEST_CASE(module3d_basic_solving) {
    
        System sys;

        point p1,p2,p3;
        p1.push_back(7);
        p1.push_back(-0.5);
        p1.push_back(0.3);
        p2.push_back(0.2);
        p2.push_back(0.5);
        p2.push_back(-0.1);
        p3.push_back(1.2);
        p3.push_back(5.9);
        p3.push_back(0.43);


        geom_ptr g1 = sys.createGeometry3D(p1);
        geom_ptr g2 = sys.createGeometry3D(p2);
        geom_ptr g3 = sys.createGeometry3D(p3);

        //check empty solving
        sys.solve();

        //simple constraint and fire
        cons_ptr c1 = sys.createConstraint3D<test_constraint>(g1, g2);
        cons_ptr c2 = sys.createConstraint3D<test_constraint>(g2, g3);
        cons_ptr c3 = sys.createConstraint3D<test_constraint>(g3, g1);
        sys.solve();

        typename Kernel::Vector3 v1,v2,v3;
        point& rp1 = get<point>(g1);
        point& rp2 = get<point>(g2);
        point& rp3 = get<point>(g3);

        v1<<rp1[0],rp1[1],rp1[2];
        v2<<rp2[0],rp2[1],rp2[2];
        v3<<rp3[0],rp3[1],rp3[2];

        BOOST_CHECK(Kernel::isSame(v1.dot(v2),0));
        BOOST_CHECK(Kernel::isSame(v2.dot(v3),0));
        BOOST_CHECK(Kernel::isSame(v3.dot(v1),0));

}

BOOST_AUTO_TEST_CASE(module3d_cluster_solving) {

    System sys;

    Eigen::Vector3d p1,p2,p3;
    p1 << 7, -0.5, 0.3;
    p2 << 0.2, 0.5, -0.1;
    p3 << 1.2, 5.9, 0.43;

    geom_ptr g1 = sys.createGeometry3D(p1);
    geom_ptr g2 = sys.createGeometry3D(p2);
    geom_ptr g3 = sys.createGeometry3D(p3);

    //now trick a bit and move two geometries manual in a subcluster
    std::pair<typename System::Cluster&, LocalVertex> sc = sys.m_cluster.createCluster();
    sys.m_cluster.moveToSubcluster(sys.m_cluster.getLocalVertex(g1->getProperty<vertex_prop>()).first, sc.second);
    sys.m_cluster.moveToSubcluster(sys.m_cluster.getLocalVertex(g2->getProperty<vertex_prop>()).first, sc.second);
    sc.first.setClusterProperty<changed_prop>(true);
    sc.first.setClusterProperty<type_prop>(details::cluster);

    //and finally add constraints
    cons_ptr c1 = sys.createConstraint3D<test_constraint>(g1, g2);
    cons_ptr c2 = sys.createConstraint3D<test_constraint>(g2, g3);
    cons_ptr c3 = sys.createConstraint3D<test_constraint>(g3, g1);

    sys.solve();

    Eigen::Vector3d v1,v2,v3;
    v1 = get<Eigen::Vector3d>(g1);
    v2 = get<Eigen::Vector3d>(g2);
    v3 = get<Eigen::Vector3d>(g3);

    BOOST_CHECK(Kernel::isSame(v1.dot(v2),0));
    BOOST_CHECK(Kernel::isSame(v2.dot(v3),0));
    BOOST_CHECK(Kernel::isSame(v3.dot(v1),0));
};

BOOST_AUTO_TEST_SUITE_END();
