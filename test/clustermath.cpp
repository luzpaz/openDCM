/*
    openDCM, dimensional constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <boost/test/unit_test.hpp>

#include<opendcm/core.hpp>
#include<opendcm/module3d.hpp>

//the only core component that needs direct externalisation
#ifdef DCM_EXTERNAL_CORE
#include "opendcm/core/imp/system_imp.hpp"
#endif

//the module3d externalisation components
#ifdef DCM_EXTERNAL_3D
#include "opendcm/module3d/imp/clustermath_imp.hpp"
#include "opendcm/module3d/imp/module_imp.hpp"
#include "opendcm/module3d/imp/geometry3d_imp.hpp"
#include "opendcm/module3d/imp/constraint3d_imp.hpp"
#endif

namespace dcm {

template<>
struct geometry_traits<Eigen::Vector3d> {
    typedef tag::point3D tag;
    typedef modell::XYZ modell;
    typedef orderd_roundbracket_accessor accessor;
};

}


typedef double Scalar;
typedef dcm::Kernel<Scalar> Kernel;
typedef dcm::Module3D< mpl::vector1< Eigen::Vector3d> > Module3D;
typedef dcm::System<Kernel, Module3D> System;

typedef Module3D::type<System>::Geometry3D Geometry3D;
typedef boost::shared_ptr<Geometry3D> Geom;

//namespace dcm {
typedef dcm::details::ClusterMath<System> cmath;
//};

BOOST_AUTO_TEST_SUITE(clustermath_suit);

BOOST_AUTO_TEST_CASE(clustermath_scaling) {

    System sys;
    cmath math;

    Kernel::Vector3 vec(0,0,0);
    math.initFixMaps();
    new(&math.m_normQ) Kernel::Vector3Map(&vec(0));

    for(int i=1; i<10; i++) {

        //add the amount of points
        for(int j=0; j<i; j++) {

            Eigen::Vector3d v = Eigen::Vector3d::Random()*100;
            Geom g(new Geometry3D(v, sys));
            //to set the local value which is used by scaling
            g->clusterMode(true, false);
            math.addGeometry(g);
        };

        //calculate the scale value for these points
        double scale = math.calculateClusterScale();

        //see if the scale value is correct
        if(i>1) {
            for(int j=0; j<i; j++) {
                double val = (math.getGeometry()[j]->point() - math.midpoint).norm();
                BOOST_CHECK_GE(val / scale , (MINFAKTOR)-0.01);
                BOOST_CHECK_LE(val / scale , (MAXFAKTOR)+0.01);
            }
        }

        //see if we can set arbitrary bigger scale values. no hart checking as currently the alogrithm
        //is not perfect
        math.applyClusterScale(scale, false);

        math.recalculate();

        for(int j=0; j<i; j++) {
            double val = math.getGeometry()[j]->point().norm();
            BOOST_CHECK_GE(val, (MINFAKTOR/SKALEFAKTOR)-0.01);
            BOOST_CHECK_LE(val, (MAXFAKTOR/SKALEFAKTOR)+0.01);
        };

        //now we can check if a second calculation and application of acale is the same as the first,
        //as it should be
        //calculate the scale value for these points
        scale = math.calculateClusterScale();

        BOOST_CHECK_GE(scale , (MINFAKTOR)-0.01);

        BOOST_CHECK_LE(scale , (MAXFAKTOR)+0.01);


	math.finishCalculation();
        math.clearGeometry();

        math.initFixMaps();
    }
}
/*
BOOST_AUTO_TEST_CASE(clustermath_multiscaling) {

    System sys;
    cmath math;
    Kernel::Vector3 vec(0,0,0);
    math.initFixMaps();
    new(&math.m_normQ) Kernel::Vector3Map(&vec(0));

    for(int j=0; j<5; j++) {

        Eigen::Vector3d v = Eigen::Vector3d::Random()*100;
        Geom g(new Geometry3D(v, sys));
        //to set the local value which is used by scaling
        g->clusterMode(true, false);
        math.addGeometry(g);
    };

    double scale = math.calculateClusterScale();
    math.applyClusterScale(2.*scale, false);
    scale = math.calculateClusterScale();
    math.applyClusterScale(scale, false);

    for(int j=0; j<5; j++) {
        double val = math.getGeometry()[j]->point().norm();
        BOOST_CHECK_GE(val, (MINFAKTOR/SKALEFAKTOR)-0.01);
        BOOST_CHECK_LE(val, (MAXFAKTOR/SKALEFAKTOR)+0.01);
    };
}

BOOST_AUTO_TEST_CASE(clustermath_identityhandling) {

    System sys;
    cmath math;

    Kernel::Quaternion Q(1,2,3,4);
    Kernel::DiffTransform3D trans = Q;
    trans *= Kernel::Transform3D::Translation(1,2,3);
    Kernel::DiffTransform3D init = trans;

    //need to init a few things
    Kernel::Vector3 vec(0,0,0);
    math.initFixMaps();
    new(&math.m_normQ) Kernel::Vector3Map(&vec(0));
    math.m_transform =  trans;

    Kernel::DiffTransform3D transI(trans);
    transI.invert();

    //add two points to the clustermath
    Eigen::Vector3d p1 = Eigen::Vector3d::Random()*100;
    Eigen::Vector3d p2 = Eigen::Vector3d::Random()*100;
    Geom g1(new Geometry3D(p1, sys));
    Geom g2(new Geometry3D(p2, sys));
    //the stuff that is normaly done by map downstream geometry
    g1->offset() = math.getParameterOffset(dcm::rotation);
    g1->clusterMode(true, false);
    g1->trans(transI);
    g2->offset() = math.getParameterOffset(dcm::general);
    g2->clusterMode(true, false);
    g2->trans(transI);
    math.addGeometry(g1);
    math.addGeometry(g2);

    //check if we have the local values right
    BOOST_CHECK(g1->toplocal().isApprox(trans.inverse()*p1, 1e-10));
    BOOST_CHECK(g2->toplocal().isApprox(trans.inverse()*p2, 1e-10));

    math.resetClusterRotation(trans);

    //check if the new toplocal is changed to the new transformation and that trans is adjusted
    BOOST_CHECK(g1->toplocal().isApprox(trans.inverse()*p1, 1e-10));
    BOOST_CHECK(g2->toplocal().isApprox(trans.inverse()*p2, 1e-10));


    //see if the downstream processing works
    g1->recalc(trans);
    g2->recalc(trans);
    BOOST_CHECK(g1->rotated().isApprox(p1,1e-10));
    BOOST_CHECK(g2->rotated().isApprox(p2,1e-10));

    //see if the change trqansformation is calculated right
    Kernel::Quaternion Qinit(1,2,3,4);
    Qinit.normalize();
    BOOST_CHECK(Qinit.isApprox((math.m_ssrTransform*trans).rotation(), 1e-10));

    //see if it works with shifting and scaling
    //math.resetClusterRotation(trans);
    math.m_transform = trans;
    Kernel::Transform3D ssrTrans = math.m_ssrTransform;
    math.initMaps();
    math.m_ssrTransform = ssrTrans;
    math.recalculate();

    Kernel::number_type s = math.calculateClusterScale();
    math.applyClusterScale(s, false);

    math.recalculate();
    BOOST_CHECK(Kernel::isSame((g1->rotated()*s*SKALEFAKTOR-p1).norm(),0., 1e-6));
    BOOST_CHECK(Kernel::isSame((g2->rotated()*s*SKALEFAKTOR-p2).norm(),0., 1e-6));

    math.finishCalculation();
    BOOST_CHECK(Kernel::isSame((g1->rotated()-p1).norm(),0., 1e-6));
    BOOST_CHECK(Kernel::isSame((g2->rotated()-p2).norm(),0., 1e-6));
    BOOST_CHECK(init.isApprox(math.m_transform, 1e-10));
}

BOOST_AUTO_TEST_CASE(clustermath_multiscaling_idendity) {

    System sys;
    cmath math;
    Kernel::Vector3 vec(0,0,0), trans(0,0,0);
    new(&math.m_normQ) Kernel::Vector3Map(&vec(0));
    new(&math.m_translation) Kernel::Vector3Map(&trans(0));
    math.initMaps();


    Eigen::Vector3d v1 = Eigen::Vector3d::Random()*100;
    Geom g1(new Geometry3D(v1, sys));
    g1->clusterMode(true, false);
    math.addGeometry(g1);

    Eigen::Vector3d v2 = Eigen::Vector3d::Random()*100;
    Geom g2(new Geometry3D(v2, sys));
    g2->clusterMode(true, false);
    math.addGeometry(g2);

    Eigen::Vector3d v3 = Eigen::Vector3d::Random()*100;
    Geom g3(new Geometry3D(v3, sys));
    g3->clusterMode(true, false);
    math.addGeometry(g3);

    Scalar d1 = (v1-v2).norm();
    Scalar d2 = (v1-v3).norm();
    Scalar d3 = (v2-v3).norm();

    double scale1 = math.calculateClusterScale();
    math.applyClusterScale(2.*scale1, false);
    math.recalculate();

    BOOST_CHECK( Kernel::isSame( (g1->rotated()-g2->rotated()).norm(), d1/(2.*scale1*SKALEFAKTOR), 1e-6 ) );
    BOOST_CHECK( Kernel::isSame( (g1->rotated()-g3->rotated()).norm(), d2/(2.*scale1*SKALEFAKTOR), 1e-6 ) );
    BOOST_CHECK( Kernel::isSame( (g2->rotated()-g3->rotated()).norm(), d3/(2.*scale1*SKALEFAKTOR), 1e-6 ) );

    double scale2 = math.calculateClusterScale();
    math.applyClusterScale(3*scale2, false);
    math.recalculate();


    BOOST_CHECK( Kernel::isSame( (g1->rotated()-g2->rotated()).norm(), d1/(2.*scale1*SKALEFAKTOR*3.*scale2*SKALEFAKTOR), 1e-6 ) );
    BOOST_CHECK( Kernel::isSame( (g1->rotated()-g3->rotated()).norm(), d2/(2.*scale1*SKALEFAKTOR*3.*scale2*SKALEFAKTOR), 1e-6 ) );
    BOOST_CHECK( Kernel::isSame( (g2->rotated()-g3->rotated()).norm(), d3/(2.*scale1*SKALEFAKTOR*3.*scale2*SKALEFAKTOR), 1e-6 ) );

    for(int j=0; j<3; j++) {
        double val = math.getGeometry()[j]->point().norm();
        BOOST_CHECK_GE(val, (MINFAKTOR/SKALEFAKTOR)-0.01);
        BOOST_CHECK_LE(val, (MAXFAKTOR/SKALEFAKTOR)+0.01);
    };

    math.finishCalculation();

    BOOST_CHECK(math.m_transform.isApprox(Kernel::Transform3D::Identity(), 1e-10));
    BOOST_CHECK( g1->rotated().isApprox(v1, 1e-10) );
    BOOST_CHECK( g2->rotated().isApprox(v2, 1e-10) );
    BOOST_CHECK( g3->rotated().isApprox(v3, 1e-10) );
}
*/
BOOST_AUTO_TEST_SUITE_END();
