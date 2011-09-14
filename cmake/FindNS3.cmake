INCLUDE(SelectLibraryConfigurations)
INCLUDE(FindPackageHandleStandardArgs)

# List of the valid ns3 components.
SET(NS3_VALID_COMPONENTS
	ns3-aodv
	ns3-applications
	ns3-bridge
	ns3-config-store
	ns3-core
	ns3-csma-layout
	ns3-csma
	ns3-dsdv
	ns3-emu
	ns3-energy
	ns3-flow-monitor
	ns3-internet
	ns3-lte
	ns3-mesh
	ns3-mobility
	ns3-mpi
	ns3-netanim
	ns3-network
	ns3-nix-vector-routing
	ns3-ns3tcp
	ns3-ns3wifi
	ns3-olsr
	ns3-point-to-point-layout
	ns3-point-to-point
	ns3-propagation
	ns3-spectrum
	ns3-stats
	ns3-tap-bridge
	ns3-template
	ns3-test
	ns3-tools
	ns3-topology-read
	ns3-uan
	ns3-virtual-net-device
	ns3-visualizer
	ns3-wifi
	ns3-wimax
)

# Find the ns3 core library.
FIND_LIBRARY(NS3_LIBRARIES
	NAME ns3-core
	HINTS
	${NS3_PATH}
	$ENV{LD_LIBRARY_PATH}
	$ENV{NS3_PATH}
	PATH_SUFFIXES lib64 lib ns3/lib
	PATHS
	/opt
	/opt/local
	/opt/csw
	/sw
	/usr
)

# Find the include dir for ns3.
FIND_PATH(NS3_INCLUDE_DIR
	NAME ns3/core-module.h
	HINTS
	${NS3_PATH}
	$ENV{NS3_PATH}
	PATH_SUFFIXES include ns3/include
	PATHS
	/opt
	/opt/local
	/opt/csw
	/sw
	/usr
)

MESSAGE(STATUS "Looking for core-module.h")
IF(NS3_INCLUDE_DIR)
	MESSAGE(STATUS "Looking for core-module.h - found")
ELSE()
	MESSAGE(STATUS "Looking for core-module.h - not found")
ENDIF()

MESSAGE(STATUS "Looking for lib ns3")
IF(NS3_LIBRARIES)
	MESSAGE(STATUS "Looking for lib ns3 - found")
ELSE()
	MESSAGE(STATUS "Looking for lib ns3 - not found")
ENDIF()

# Validate the list of find components.
SET(NS3_CHOSEN_COMPONENTS "ns3-core")
IF(NS3_FIND_COMPONENTS)
	FOREACH(component ${NS3_FIND_COMPONENTS})
		LIST(FIND NS3_VALID_COMPONENTS ${component} component_location)
		IF(${component_location} EQUAL -1)
			MESSAGE(FATAL_ERROR "\"${component}\" is not a valid NS3 component.")
		ELSE()
			LIST(FIND NS3_CHOSEN_COMPONENTS ${component} component_location)
			IF(${component_location} EQUAL -1)
				LIST(APPEND NS3_CHOSEN_COMPONENTS ${component})
			ENDIF()
		ENDIF()
	ENDFOREACH()
ENDIF()

# Library + Include Directories
IF(NS3_LIBRARIES AND NS3_INCLUDE_DIR)
	GET_FILENAME_COMPONENT(NS3_LIBRARY_DIR ${NS3_LIBRARIES} PATH)
	MESSAGE(STATUS "NS3 Library directory is ${NS3_LIBRARY_DIR}")
	STRING(REGEX MATCH "${NS3_LIBRARY_DIR}" in_path "$ENV{LD_LIBRARY_PATH}")

	IF(NOT in_path)
		MESSAGE(STATUS "Warning: To use NS-3 don't forget to set LD_LIBRARY_PATH with:	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${NS3_LIBRARY_DIR}")
	ELSE()
		STRING(REGEX MATCH "-L${NS3_LIBRARY_DIR} " have_Lflag "${CMAKE_C_FLAGS}")
		IF(NOT have_Lflag)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-L${NS3_LIBRARY_DIR} ")
		ENDIF()

		STRING(REGEX MATCH "-I${NS3_INCLUDE_DIR} " have_Iflag "${CMAKE_C_FLAGS}")
		IF(NOT have_Iflag`)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${NS3_INCLUDE_DIR} ")
		ENDIF()

		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-I${NS3_INCLUDE_DIR} -L${NS3_LIBRARY_DIR} ")
	ENDIF()
	SET(NS3_FOUND 1)
ENDIF()

# Try to find components
IF(NS3_FOUND)
	FOREACH(_component ${NS3_CHOSEN_COMPONENTS})
		FIND_LIBRARY(NS3_${_component}_LIBRARY
			NAME ${_component}
			HINTS
			${NS3_PATH}
			$ENV{LD_LIBRARY_PATH}
			$ENV{NS3_PATH}
			PATH_SUFFIXES lib64 lib ns3/lib
			PATHS
			/opt
			/opt/local
			/opt/csw
			/sw
			/usr
			${NS3_LIBRARY_DIR}
		)
		MARK_AS_ADVANCED(NS3_${_component}_LIBRARY)
		LIST(APPEND NS3_LIBRARIES ${NS3_${_component}_LIBRARY})
	ENDFOREACH()
ENDIF()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(NS3 DEFAULT_MSG NS3_LIBRARIES NS3_INCLUDE_DIR)
MARK_AS_ADVANCED(NS3_LIBRARIES NS3_INCLUDE_DIR)
