if(TARGET nfd::nfd)
    return()
endif()

include(FetchContent)
FetchContent_Declare(
    nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
    )
FetchContent_MakeAvailable(nfd)

add_library(nfd::nfd ALIAS nfd)
set_target_properties(nfd PROPERTIES FOLDER third_party)
