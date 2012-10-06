/*
    openGCM, geometric constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more detemplate tails.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef GCM_CONSTRAINT_H
#define GCM_CONSTRAINT_H

#include <assert.h>
#include <boost/variant.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/size.hpp>


#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/for_each.hpp>

#include "traits.hpp"
#include "object.hpp"
#include "equations.hpp"

namespace mpl = boost::mpl;
namespace fusion = boost::fusion; 

namespace dcm {

namespace detail {
  
//type erasure container for constraints
template<typename Sys, typename Derived, typename Signals, typename MES, typename Geometry>
class Constraint : public Object<Sys, Derived, Signals > {

    typedef typename system_traits<Sys>::Kernel Kernel;
    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::DynStride DS;
    
    typedef boost::shared_ptr<Geometry> Geom;

public:
    Constraint(Sys& system, Geom f, Geom s) : Object<Sys, Derived, Signals > (system),
        first(f), second(s), content(0)	{

        cf = first->template connectSignal<reset> (boost::bind(&Constraint::geometryReset, this, _1));
        cs = second->template connectSignal<reset> (boost::bind(&Constraint::geometryReset, this, _1));
    };

    ~Constraint()  {
        delete content;
        first->template disconnectSignal<reset>(cf);
        second->template disconnectSignal<reset>(cs);
    }

    template<typename ConstraintVector>
    void initialize(typename fusion::result_of::as_vector<ConstraintVector>::type& obj) {

        //first create the new placeholder
        creator<ConstraintVector> c(obj);
        boost::apply_visitor(c, first->m_geometry, second->m_geometry);

        //and now store it
        content = c.p;
        //geometry order needs to be the one needed by equations
        if(c.need_swap) first.swap(second);

    };



    int equationCount() {
        return content->equationCount();
    };

    template< typename creator_type>
    void resetType(creator_type& c) {
        boost::apply_visitor(c, first->m_geometry, second->m_geometry);
        content = c.p;
        if(c.need_swap) first.swap(second);
    };

    void calculate() {
        content->calculate(first, second);
    };

    void setMaps(MES& mes) {
        content->setMaps(mes, first, second);
    };

    void geometryReset(Geom g) {
        /*    placeholder* p = content->resetConstraint(first, second);
            delete content;
            content = p;*/
    };
    
protected:
    //Equation is the constraint with types, the EquationSet hold all needed Maps for calculation
    template<typename Equation>
    struct EquationSet {
        EquationSet() : m_diff_first(NULL,0,DS(0,0)), m_diff_second(NULL,0,DS(0,0)),
            m_rot_diff_first(NULL,0,DS(0,0)), m_rot_diff_second(NULL,0,DS(0,0)),
            m_trans_diff_first(NULL,0,DS(0,0)), m_trans_diff_second(NULL,0,DS(0,0)), m_residual(NULL,0,DS(0,0)) {};

        Equation m_eq;
        typename Kernel::VectorMap m_rot_diff_first, m_trans_diff_first, m_diff_first; //first geometry diff
        typename Kernel::VectorMap m_rot_diff_second, m_trans_diff_second, m_diff_second; //second geometry diff
        typename Kernel::VectorMap m_residual;
    };

    struct placeholder  {
        virtual ~placeholder() {}
        virtual placeholder* resetConstraint(Geom first, Geom second) const = 0;
        virtual void calculate(Geom first, Geom second) = 0;
        virtual int  equationCount() = 0;
        virtual void setMaps(MES& mes, Geom first, Geom second) = 0;
    };

    template< typename ConstraintVector, typename EquationVector>
    struct holder : public placeholder  {

        //create a vector of EquationSets with some mpl trickery
        typedef typename mpl::fold< EquationVector, mpl::vector<>,
                mpl::push_back<mpl::_1, EquationSet<mpl::_2> > >::type eq_set_vector;
        typedef typename fusion::result_of::as_vector<eq_set_vector>::type EquationSets;

        typedef typename fusion::result_of::as_vector<ConstraintVector>::type Objects;

        template<typename T>
        struct has_option {
            //we get the index of the eqaution in the eqaution vector, and as it is the same
            //as the index of the constraint in the constraint vector we can extract the
            //option type and check if it is no_option
            typedef typename mpl::find<EquationVector, T>::type iterator;
            typedef typename mpl::distance<typename mpl::begin<EquationVector>::type, iterator>::type distance;
            BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<EquationVector>::type > >));
            typedef typename mpl::at<ConstraintVector, distance>::type option_type;
            typedef boost::is_same<option_type, no_option> type;
        };

        struct OptionSetter {

            Objects& objects;

            OptionSetter(Objects& val) : objects(val) {};

            //only set the value if the equation has a option
            template< typename T >
            typename boost::enable_if<typename has_option<T>::type, void>::type
            operator()(EquationSet<T>& val) const {

                //get the index of the corresbonding equation
                typedef typename mpl::find<EquationVector, T>::type iterator;
                typedef typename mpl::distance<typename mpl::begin<EquationVector>::type, iterator>::type distance;
                BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<EquationVector>::type > >));
                val.m_eq.value = fusion::at<distance>(objects).value;
            }
            //if the equation has no otpion we do nothing!
            template< typename T >
            typename boost::enable_if<mpl::not_<typename has_option<T>::type>, void>::type
            operator()(EquationSet<T>& val) const {}
        };

        struct Calculater {

            Geom first, second;
            Calculater(Geom f, Geom s) : first(f), second(s) {};

            template< typename T >
            void operator()(T& val) const {

                val.m_residual(0) = val.m_eq.calculate(first->m_parameter, second->m_parameter);

                //now see which way we should calculate the gradient (may be diffrent for both geometries)
                if(first->m_parameterCount) {
                    if(first->getClusterMode()) {
                        //when the cluster is fixed no maps are set as no parameters exist.
                        if(!first->isClusterFixed()) {

                            //cluster mode, so we do a full calculation with all 3 rotation diffparam vectors
                            for(int i=0; i<3; i++) {
                                typename Kernel::VectorMap block(&first->m_diffparam(0,i),first->m_parameterCount,1, DS(1,1));
                                val.m_rot_diff_first(i) = val.m_eq.calculateGradientFirst(first->m_parameter,
                                                          second->m_parameter, block);
                            }
                            //now the translation stuff
                            for(int i=3; i<6; i++) {
                                typename Kernel::VectorMap block(&first->m_diffparam(0,i),first->m_parameterCount,1, DS(1,1));
                                val.m_trans_diff_first(i-3) = val.m_eq.calculateGradientFirst(first->m_parameter,
                                                              second->m_parameter, block);
                            }
                        }
                    } else {
                        //not in cluster, so allow the constraint to optimize the gradient calculation
                        val.m_eq.calculateGradientFirstComplete(first->m_parameter, second->m_parameter, val.m_diff_first);
                    }
                }
                if(second->m_parameterCount) {
                    if(second->getClusterMode()) {
                        if(!second->isClusterFixed()) {

                            //cluster mode, so we do a full calculation with all 3 rotation diffparam vectors
                            for(int i=0; i<3; i++) {
                                typename Kernel::VectorMap block(&second->m_diffparam(0,i),second->m_parameterCount,1, DS(1,1));
                                val.m_rot_diff_second(i) = val.m_eq.calculateGradientSecond(first->m_parameter,
                                                           second->m_parameter, block);
                            }
                            //translational gradients
                            for(int i=3; i<6; i++) {
                                typename Kernel::VectorMap block(&second->m_diffparam(0,i),second->m_parameterCount,1, DS(1,1));
                                val.m_trans_diff_second(i-3) = val.m_eq.calculateGradientSecond(first->m_parameter,
                                                               second->m_parameter, block);
                            }
                        }
                    } else {
                        //not in cluster, so allow the constraint to optimize the gradient calculation
                        val.m_eq.calculateGradientSecondComplete(first->m_parameter, second->m_parameter, val.m_diff_second);
                    }

                }
            };
        };

        struct MapSetter {
            MES& mes;
            Geom first, second;

            MapSetter(MES& m, Geom f, Geom s) : mes(m), first(s), second(s) {};

            template< typename T >
            void operator()(T& val) const {

                //when in cluster, there are 6 clusterparameter we differentiat for, if not we differentiat
                //for every parameter in the geometry;
                int equation = mes.setResidualMap(val.m_residual);
                if(first->getClusterMode()) {
                    if(!first->isClusterFixed()) {
                        mes.setJacobiMap(equation, first->m_trans_offset, 3, val.m_trans_diff_first);
                        mes.setJacobiMap(equation, first->m_rot_offset, 3, val.m_rot_diff_first);
                    }
                } else  mes.setJacobiMap(equation, first->m_parameter_offset, first->m_parameterCount, val.m_diff_first);

                if(second->getClusterMode()) {
                    if(!second->isClusterFixed()) {
                        mes.setJacobiMap(equation, second->m_trans_offset, 3, val.m_trans_diff_second);
                        mes.setJacobiMap(equation, second->m_rot_offset, 3, val.m_rot_diff_second);
                    }
                } else mes.setJacobiMap(equation, second->m_parameter_offset, first->m_parameterCount, val.m_diff_second);
            };
        };

        holder(Objects& obj)  {
            //set the initial values in the equations
            fusion::for_each(m_sets, OptionSetter(obj));
        };

        virtual void calculate(Geom first, Geom second) {
            fusion::for_each(m_sets, Calculater(first, second));
        };

        virtual placeholder* resetConstraint(Geom first, Geom second) const {
            /*creator<ConstraintVector> creator;
            boost::apply_visitor(creator, first->m_geometry, second->m_geometry);
            if(creator.need_swap) first.swap(second);
            return creator.p;*/
        };
        virtual int equationCount() {
            return mpl::size<EquationVector>::value;
        };

        virtual void setMaps(MES& mes, Geom first, Geom second) {
            fusion::for_each(m_sets, MapSetter(mes, first, second));
        };

        EquationSets m_sets;
    };

    template< typename  ConstraintVector >
    struct creator : public boost::static_visitor<void> {

        typedef typename fusion::result_of::as_vector<ConstraintVector>::type Objects;
        Objects& objects;

        creator(Objects& obj) : objects(obj) {};

        template<typename C, typename T1, typename T2>
        struct equation {
            typedef typename C::template type<Kernel, T1, T2> type;
        };

        template<typename T1, typename T2>
        void operator()(const T1&, const T2&) {
            typedef tag_order< typename geometry_traits<T1>::tag, typename geometry_traits<T2>::tag > order;

            //transform the constraints into eqautions with the now known types
            typedef typename mpl::fold< ConstraintVector, mpl::vector<>,
                    mpl::push_back<mpl::_1, equation<mpl::_2, typename order::first_tag,
                    typename order::second_tag> > >::type EquationVector;

            //and build the placeholder
            p = new holder<ConstraintVector, EquationVector>(objects);
            need_swap = order::swapt::value;
        };
        placeholder* p;
        bool need_swap;
    };

    placeholder* content;
    Geom first, second;
    Connection cf, cs;
};

};//detail

};//dcm

#endif //GCM_CONSTRAINT_H

