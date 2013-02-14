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

#ifndef GCM_MODULE_PART_H
#define GCM_MODULE_PART_H

#include "opendcm/Core"
#include "opendcm/core/traits.hpp"
#include "opendcm/core/clustergraph.hpp"
#include "opendcm/core/property.hpp"
#include "opendcm/Module3D"

#include <boost/mpl/assert.hpp>
#include <boost/utility/enable_if.hpp>

namespace mpl = boost::mpl;

namespace dcm {

enum { clusterPart = 110};

enum CoordinateFrame {Local, Global};

template<typename Typelist, typename ID = No_Identifier>
struct ModulePart {

    template<typename Sys>
    struct type {


        class Part;
        struct PrepareCluster;
        struct EvaljuateCluster;
        typedef boost::shared_ptr<Part> Partptr;
        typedef mpl::map1< mpl::pair<remove, boost::function<void (Partptr) > > >  PartSignal;

        typedef ID Identifier;

        class Part_base : public Object<Sys, Part, PartSignal > {
        protected:
            using Object<Sys, Part, PartSignal >::m_system;

            //check if we have module3d in this system
            typedef typename system_traits<Sys>::template getModule<m3d>::type module3d;
            BOOST_MPL_ASSERT((mpl::not_<boost::is_same<module3d, mpl::void_> >));

            //define what we need
            typedef typename module3d::Geometry3D Geometry3D;
            typedef boost::shared_ptr<Geometry3D> Geom;

            typedef typename boost::make_variant_over< Typelist >::type Variant;
            typedef Object<Sys, Part, PartSignal> base;
            typedef typename system_traits<Sys>::Kernel Kernel;
            typedef typename system_traits<Sys>::Cluster Cluster;
            typedef typename Kernel::number_type Scalar;
            typedef typename Kernel::Transform3D Transform;

            struct cloner : boost::static_visitor<void> {

                Variant variant;
                cloner(Variant& v) : variant(v) {};

                template<typename T>
                void operator()(T& t) {
                    variant = geometry_clone_traits<T>()(t);
                };
            };

        public:
            template<typename T>
            Part_base(T geometry, Sys& system, Cluster& cluster) : base(system),
                m_geometry(geometry), m_cluster(&cluster)  {

                (typename geometry_traits<T>::modell()).template extract<Kernel,
                typename geometry_traits<T>::accessor >(geometry, m_transform);

                //the cluster needs initial values but they are set by preprocess job

                cluster.template setClusterProperty<typename module3d::fix_prop>(false);
            };

            template<typename Visitor>
            typename Visitor::result_type apply(Visitor& vis) {
                return boost::apply_visitor(vis, m_geometry);
            };

            template<typename T>
            Geom addGeometry3D(T geom, CoordinateFrame frame = Global) {
                Geom g(new Geometry3D(geom, *m_system));
                if(frame == Local)
                    g->transform(m_transform);

                fusion::vector<LocalVertex, GlobalVertex> res = m_cluster->addVertex();
                m_cluster->template setObject<Geometry3D> (fusion::at_c<0> (res), g);
                g->template setProperty<typename module3d::vertex_prop>(fusion::at_c<1>(res));
                m_system->template objectVector<Geometry3D>().push_back(g);

                return g;
            };

            template<typename T>
            void set(T geometry) {
                Part_base::m_geometry = geometry;
                (typename geometry_traits<T>::modell()).template extract<Kernel,
                typename geometry_traits<T>::accessor >(geometry, Part_base::m_transform);
            };

            virtual boost::shared_ptr<Part> clone(Sys& newSys) {

                boost::shared_ptr<Part> np = base::clone(newSys);
                //there may be  pointer inside the variant
                cloner clone_fnc(np->m_geometry);
                boost::apply_visitor(clone_fnc, m_geometry);
                //we need to reset the cluster pointer to the new system cluster
                LocalVertex  lv = base::m_system->m_cluster.getClusterVertex(*m_cluster);
                GlobalVertex gv = base::m_system->m_cluster.getGlobalVertex(lv);
                fusion::vector<LocalVertex, typename Sys::Cluster*, bool> res = newSys.m_cluster.getLocalVertexGraph(gv);
                np->m_cluster = &(fusion::at_c<1>(res)->getVertexCluster(fusion::at_c<0>(res)));

                return np;
            };

        public:
            Variant 	m_geometry;
            Transform 	m_transform;
            Cluster* 	m_cluster;


            //visitor to write the calculated value into the variant
            struct apply_visitor : public boost::static_visitor<void> {

                apply_visitor(Transform& t) : m_transform(t) {};

                template <typename T>
                void operator()(T& t) const  {
                    (typename geometry_traits<T>::modell()).template inject<Kernel,
                    typename geometry_traits<T>::accessor >(t, m_transform);
                }
                Transform& m_transform;
            };

            void finishCalculation() {
                m_transform.normalize();
                apply_visitor vis(m_transform);
                apply(vis);
            };

            void fix(bool fix_value) {
                m_cluster->template setClusterProperty<typename module3d::fix_prop>(fix_value);
            };
        };

        class Part_id : public Part_base {

        public:
            template<typename T>
            Part_id(T geometry, Sys& system,  typename Part_base::Cluster& cluster) : Part_base(geometry, system, cluster) {};

            template<typename T>
            typename Part_base::Geom addGeometry3D(T geom, Identifier id, CoordinateFrame frame = Global) {

                typename Part_base::Geom g = Part_base::addGeometry3D(geom, frame);
                g->setIdentifier(id);
                return g;
            };

            template<typename T>
            void set(T geometry, Identifier id) {
                Part_base::set(geometry);
                setIdentifier(id);
            };

            bool hasGeometry3D(Identifier id) {
                typename Part_base::Geom g = Part_base::m_system->getGeometry3D(id);
                if(!g) return false;

                //get the global vertex and check if it is a child of the part cluster
                GlobalVertex v = g->template getProperty<typename Part_base::module3d::vertex_prop>();
                return Part_base::m_cluster->getLocalVertex(v).second;
            };

            Identifier& getIdentifier() {
                return  this->template getProperty<id_prop<Identifier> >();
            };
            void setIdentifier(Identifier id) {
                this->template setProperty<id_prop<Identifier> >(id);
            };
        };

        struct Part : public mpl::if_<boost::is_same<Identifier, No_Identifier>, Part_base, Part_id>::type {

            typedef typename mpl::if_<boost::is_same<Identifier, No_Identifier>, Part_base, Part_id>::type base;
            template<typename T>
            Part(T geometry, Sys& system, typename base::Cluster& cluster) : base(geometry, system, cluster) {};

            friend struct PrepareCluster;
            friend struct EvaljuateCluster;
        };


        struct inheriter_base {

            inheriter_base() {
                m_this = ((Sys*) this);
            };

            Sys* m_this;

            template<typename T>
            Partptr createPart(T geometry) {
                typedef typename system_traits<Sys>::Cluster Cluster;
                std::pair<Cluster&, LocalVertex>  res = m_this->m_cluster.createCluster();
                Partptr p(new Part(geometry, * ((Sys*) this), res.first));

                m_this->m_cluster.template setObject<Part> (res.second, p);
                m_this->push_back(p);

                res.first.template setClusterProperty<type_prop>(clusterPart);
                return p;
            }

            void removePart(Partptr p) {
                remover r(*m_this);
                m_this->m_cluster.removeCluster(*(p->m_cluster), r);
                p->template emitSignal<remove>(p);
                m_this->erase(p);
            }

        protected:
            //function object to emit remove signal too al geometry which is deleted by part deletion
            struct remover {
                typedef typename system_traits<Sys>::Cluster Cluster;
                typedef typename system_traits<Sys>::template getModule<m3d>::type module3d;
                typedef typename module3d::Geometry3D Geometry3D;
                typedef boost::shared_ptr<Geometry3D> Geom;
                typedef typename module3d::Constraint3D Constraint3D;
                typedef boost::shared_ptr<Constraint3D> Cons;

                Sys& system;
                remover(Sys& s) : system(s) {};
                //see if we have a geometry or a constraint and emit the remove signal
                void operator()(GlobalVertex v) {
                    Geom g = system.m_cluster.template getObject<Geometry3D>(v);
                    if(g) {
                        g->template emitSignal<remove>(g);
                        system.erase(g);
                    }
                    Cons c = system.m_cluster.template getObject<Constraint3D>(v);
                    if(c) {
                        c->template emitSignal<remove>(c);
                        system.erase(c);
                    }
                };
                //we delete all global edges connecting to this part
                void operator()(GlobalEdge e) {
                    Cons c = system.m_cluster.template getObject<Constraint3D>(e);
                    if(c) {
                        c->template emitSignal<remove>(c);
                        system.erase(c);
                    }
                };
                void operator()(Cluster& g) {};
            };
        };

        struct inheriter_id : public inheriter_base {

            Identifier drag_id;

            template<typename T>
            Partptr createPart(T geometry, Identifier id) {
                Partptr p = inheriter_base::createPart(geometry);
                p->setIdentifier(id);
                return p;
            };

            bool hasPart(Identifier id) {
                if(getPart(id)) return true;
                return false;
            };

            Partptr getPart(Identifier id) {
                std::vector< Partptr >& vec = inheriter_base::m_this->template objectVector<Part>();
                typedef typename std::vector<Partptr>::iterator iter;
                for(iter it=vec.begin(); it!=vec.end(); it++) {
                    if(compare_traits<Identifier>::compare((*it)->getIdentifier(), id)) return *it;
                };
                return Partptr();
            };
        };

        struct inheriter : public mpl::if_<boost::is_same<Identifier, No_Identifier>, inheriter_base, inheriter_id>::type {};

        typedef mpl::vector0<>  properties;
        typedef mpl::vector1<Part>  objects;

        struct PrepareCluster : public Job<Sys> {

            typedef typename system_traits<Sys>::Cluster Cluster;
            typedef typename system_traits<Sys>::Kernel Kernel;
            typedef typename system_traits<Sys>::template getModule<m3d>::type module3d;

            PrepareCluster() {
                Job<Sys>::priority = 1000;
            };

            virtual void execute(Sys& sys) {
                //get all parts and set their values to the cluster's
                typedef typename std::vector<Partptr>::iterator iter;
                for(iter it = sys.template begin<Part>(); it != sys.template end<Part>(); it++) {

                    details::ClusterMath<Sys>& cm = (*it)->m_cluster->template getClusterProperty<typename module3d::math_prop>();
                    cm.getTransform() = (*it)->m_transform;
                };
            };
        };

        struct EvaljuateCluster : public Job<Sys> {

            typedef typename system_traits<Sys>::Cluster Cluster;
            typedef typename system_traits<Sys>::Kernel Kernel;
            typedef typename system_traits<Sys>::template getModule<m3d>::type module3d;

            EvaljuateCluster() {
                Job<Sys>::priority = 1000;
            };

            virtual void execute(Sys& sys) {
                //get all parts and set their values to the cluster's
                typedef typename std::vector<Partptr>::iterator iter;
                for(iter it = sys.template begin<Part>(); it != sys.template end<Part>(); it++) {

                    details::ClusterMath<Sys>& cm = (*it)->m_cluster->template getClusterProperty<typename module3d::math_prop>();
                    (*it)->m_transform =  cm.getTransform();
                    (*it)->finishCalculation();
                };
            };
        };

        static void system_init(Sys& sys) {
            sys.m_sheduler.addPreprocessJob(new PrepareCluster());
            sys.m_sheduler.addPostprocessJob(new EvaljuateCluster());
        };
	static void system_copy(Sys& from, Sys& into) {
            //our part objects are not in the clustergraph, and therefore are not copied until now
            typedef typename std::vector< boost::shared_ptr<Part> > iter;
            std::vector< boost::shared_ptr<Part> > vec = from.template objectVector<Part>();
	    for(iter it=vec.begin(); it != vec.end(); it++) 
	      into.push_back( (*it)->clone(into) );	    
        };
    };
};

}

#endif //GCM_MODULEPART_H




