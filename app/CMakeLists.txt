# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(smartpark ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(smartpark LINK_PRIVATE hal)

# Copy executable to final location (change `smartpark` to project name as needed)
add_custom_command(TARGET smartpark POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:smartpark>"
     "$ENV{HOME}/ensc351/public/myApps/smartpark" 
  COMMENT "Copying ARM executable to public NFS directory")
