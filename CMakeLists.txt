cmake_minimum_required(VERSION 2.6) 
project(Dyn-NSupdate)

FIND_PACKAGE( Boost 1.40 COMPONENTS regex program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

set(DYNNSUPDATE_CONFIG_FILE "/var/lib/bind/dyn-nsupdate.conf" CACHE FILEPATH "Choose the file dyn-nsupdate reads its configuration from (for security reasons, this is hard-coded in the binary)")

set(CMAKE_CXX_FLAGS "-Wall -Wextra ${CMAKE_CXX_FLAGS} -std=c++11")

add_definitions("-DCONFIG_FILE=\"${DYNNSUPDATE_CONFIG_FILE}\"")

ADD_EXECUTABLE( dyn-nsupdate dyn-nsupdate.cpp )

TARGET_LINK_LIBRARIES( dyn-nsupdate ${Boost_LIBRARIES} )
