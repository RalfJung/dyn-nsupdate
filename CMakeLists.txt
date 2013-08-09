cmake_minimum_required(VERSION 2.6) 
project(Dyn-NSupdate)

FIND_PACKAGE( Boost 1.40 REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

set(CMAKE_CXX_FLAGS "-Wall -Wextra ${CMAKE_CXX_FLAGS}")

ADD_EXECUTABLE( dyn-nsupdate dyn-nsupdate.cpp )

TARGET_LINK_LIBRARIES( dyn-nsupdate ${Boost_LIBRARIES} )
