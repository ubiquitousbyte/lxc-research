include(FetchContent)

FetchContent_Declare(json-c
        GIT_REPOSITORY https://github.com/json-c/json-c.git
        GIT_TAG 2f2ddc1f2dbca56c874e8f9c31b5b963202d80e7 # Release 0.16
        )
FetchContent_MakeAvailable(json-c)

