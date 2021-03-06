
add_definitions(-DDCM_TESTING)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${EIGEN3_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${TBB_INCLUDE_DIR})

link_directories ( ${Boost_LIBRARY_DIRS} ${TBB_LIBRARY_DIR} )

set(framework_SRC framework.cpp)

set(test_SRC  define.cpp
              transform.cpp
              #kernel.cpp
              system.cpp
              numeric.cpp
              constraint.cpp
	      clustergraph.cpp
	      reduction.cpp
	      module3d.cpp
	      modulepart.cpp
              #clustermath.cpp
              #constraints3d.cpp
	      #misc.cpp
	      #userbugs.cpp
	      #scheduler.cpp
	      #${state_SRC}	      
)

if(LOGGING)
set(LOG_LIBS  ${Boost_LIBRARIES})
endif(LOGGING)

add_library(framework ${framework_SRC})

add_executable(solvertest ${test_SRC})

# visual studio has problems which huge cpp files
IF (MSVC)
SET_TARGET_PROPERTIES(solvertest PROPERTIES COMPILE_FLAGS "/bigobj")
ENDIF (MSVC)

target_link_libraries(solvertest framework ${LOG_LIBS} ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY} ${TBB_LIBRARY})
