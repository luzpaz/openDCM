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

#ifndef DCM_MODULE_3D_H
#define DCM_MODULE_3D_H

#include "opendcm/core/object.hpp"
#include "opendcm/core/module.hpp"
#include "opendcm/core/utilities.hpp"
#include "opendcm/core/geometry.hpp"
#include "opendcm/core/constraint.hpp"
#include "opendcm/core/typeadaption.hpp"

#include "geometry.hpp"


#include <type_traits>

namespace dcm {
       
template<typename Type>
struct Part3D {

    typedef boost::mpl::int_<6> ID;

    template<typename Final, typename Stacked>
    struct type : public Stacked {

        typedef typename Stacked::Kernel Kernel;
      
        type() : Stacked() {};        
        ~type() {}
        
        //we setup the reduction graph after the constructors are done, as only then the graph 
        //is available (as well as the virtual function to get it)
        void init() {
            
            //that is our responsibility in the init stack
            Stacked::init();
            
        };
        
        /**
         * @brief Container for 3D user parts
         * 
         * A Part is a container for geometry, which is rigidly connected. It provides a way to move 
         * the rigid system in 6 degrees of freedom. This movement is what defines the Part and what 
         * the user must supply via the given Module type.
         * 
         * To add geometry to the part simple use the addGeometry() method of the object excactly the 
         * same as its system counterpart. 
         */
        struct Part3D : public Stacked::Object {

            typedef typename Stacked::Object   Inherited;                       
            typedef graph::VertexProperty      VertexProperty;
            
            DCM_OBJECT_ADD_PROPERTIES( Final, (VertexProperty) )
            
        public:            
            Part3D(Final* system) 
                : Stacked::Object( Final::template objectTypeID<typename Final::Part3D>::ID::value ),
                m_system(system) {};
            
            /**
             * @brief Set the content and stores the user type
             * 
             * This function is used to set the content of the Part3D container. It extract the contents of 
             * the given user Type and stores it in an accessible manner for the solver. Furhtermore it stores
             * the supplied user type for later access. The exact behaviour of this feature depends on the specified
             * qualifiers of the storable types as given as the Module3D template arguments. If the type was given
             * without any special qualifiers then a copy of the given user type is strored.
            */
            void set(const Type& part) {
                /*
                typedef typename Final::Kernel  Kernel;
                typedef typename Kernel::Scalar Scalar;
                typedef geometry::extractor<typename geometry_traits<T>::type> extractor;
                typedef typename extractor::template primitive<typename Final::Kernel> Geometry;
                typedef symbolic::TypeGeometry<Geometry> TypeGeometry;

                BOOST_MPL_ASSERT((mpl::contains<mpl::vector<types...>, T>));
                
                //we may need to setup the graph. We do this here to allow to clear the geometry and later reinitialize by setting a new geometry.
                std::shared_ptr<typename Final::Graph> cluster = std::static_pointer_cast<typename Final::Graph>(m_system->getGraph());
                if(!holdsGeometry()) {                    
                    fusion::vector<graph::LocalVertex, graph::GlobalVertex> res = cluster->addVertex();
                    cluster->template setProperty<details::GraphObjectProperty>(fusion::at_c<0>(res), InheritedO::shared_from_this());
                    setVertexProperty(fusion::at_c<1>(res));
                };
                
                //store the type
                m_type = Geometry::index();
                InheritedV::m_variant = geometry;*/                
            };
            
            /**
             * @brief Adds geometry to the Part
             * The geometry is rigidly connected with all other geometries within the Part, that means
             * all constraints with other inner part geometries are ignored. However, constraints to 
             * geometries outside the Part or within other Parts can be created. They are than resolved 
             * by moving the Part in such a way, that the geometry constraints are fullfiled.
             */
            template<typename T>
            std::shared_ptr<Geometry3D> addGeometry3D(const T& geom) {};
        };

            
        protected:
            Final* m_system;
         
        DCM_MODULE_ADD_OBJECTS(Stacked, (Part3D))

    public:  
        std::shared_ptr<Geometry3D> addPart3D(const Type& part) {
            
            auto p = std::make_shared<Part3D>(static_cast<Final*>(this));
            p->set(part);
            return p;
        };
        
    protected:
        friend struct Part3D;
        
    };
    
};

}//dcm

#endif //DCM_GEOMETRY3D_H







