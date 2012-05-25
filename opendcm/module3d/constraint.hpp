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

#ifndef DCM_CONSTRAINT3D_H
#define DCM_CONSTRAINT3D_H

#include "geometry.hpp"

namespace dcm {


template<typename Kernel, typename Tag1, typename Tag2>
struct Coincident3D {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    Scalar calculate(Vector& param1,  Vector& param2)  {
        /*TODO:assert*/
    };

    Scalar calculateGradientFirst(Vector& param1,  Vector& param2, Vector& dparam1) {
        /*TODO:assert*/
    };

    Scalar calculateGradientSecond(Vector& param1,  Vector& param2, Vector& dparam2)  {
        /*TODO:assert*/
    };

    void calculateGradientFirstComplete(Vector& param1,  Vector& param2, Vector& gradient) {
        /*TODO:assert*/
    };

    void calculateGradientSecondComplete(Vector& param1,  Vector& param2, Vector& gradient) {
        /*TODO:assert*/
    };

};


template< typename Kernel, typename Tag1, typename Tag2 >
struct Distance3D {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    //template definition
    Scalar calculate(Vector& param1,  Vector& param2) {
        /*TODO:assert*/
    };

    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
        /*TODO:assert*/
    };

    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
        /*TODO:assert*/
    };

    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        /*TODO:assert*/
    };

    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        /*TODO:assert*/
    };
};

template< typename Kernel >
struct Distance3D< Kernel, tag::point3D, tag::point3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    //template definition
    Scalar calculate(Vector& param1,  Vector& param2) {

	
    };

    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {

    };

    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {

    };

    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {

    };

    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {

    };
   
};

}

#endif //DCM_CONSTRAINT3D_H
