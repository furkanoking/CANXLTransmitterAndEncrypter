cmake_minimum_required(VERSION 3.25)
project(CANSecKodCANXL)

set(CMAKE_CXX_STANDARD 23)

add_library(CANSecKod STATIC CANSec.cpp
        CANXLStructure/CANXL.cpp)

target_include_directories(CANSecKod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/CANXLStructure)

target_include_directories(CANSecKod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/CANFDStructure)

# Link OpenSSL (required for encryption/decryption)
find_package(OpenSSL REQUIRED)
target_link_libraries(CANSecKod PRIVATE OpenSSL::SSL OpenSSL::Crypto)

enable_testing()
find_package(Threads REQUIRED) # GTest'in buna da ihtiyacı var

# 1. Gerekli tüm yolları en başta tanımlayın
set(GTEST_INCLUDE_DIR "/usr/include")
set(GTEST_LIBRARY "/usr/lib/libgtest.a")
set(GTEST_MAIN_LIBRARY "/usr/lib/libgtest_main.a")

# 2. GTest::gtest hedefini ve özelliklerini tanımlayın (HATANIN ÇÖZÜMÜ)
add_library(GTest::gtest STATIC IMPORTED)
set_target_properties(GTest::gtest PROPERTIES
        IMPORTED_LOCATION ${GTEST_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIR}
)

# 3. GTest::gtest_main hedefini ve özelliklerini tanımlayın (Bu sizde vardı)
add_library(GTest::gtest_main STATIC IMPORTED)
set_target_properties(GTest::gtest_main PROPERTIES
        IMPORTED_LOCATION ${GTEST_MAIN_LIBRARY}
        INTERFACE_LINK_LIBRARIES GTest::gtest  # gtest_main'i gtest'e bağla
)

# 4. Executable'ı oluşturun
add_executable(test_main tests/test.cpp)

# 5. Linklemeyi yapın (tek komut yeterli)
target_link_libraries(test_main PRIVATE
        CANSecKod
        GTest::gtest_main # gtest_main zaten gtest'i ve Threads'i getirecek
        Threads::Threads
)

add_executable(test_linux tests/sendandreceive.cpp)
target_link_libraries(test_linux PRIVATE
        CANSecKod
        GTest::gtest_main
        Threads::Threads
)
target_include_directories(test_linux PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CANXLStructure)

add_executable(receiver tests/receiver.cpp)
target_link_libraries(receiver PRIVATE
        CANSecKod
        GTest::gtest_main
        Threads::Threads
)
target_include_directories(receiver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CANXLStructure)


# canbus-common include
target_include_directories(CANSecKod PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party-library/canbus-common/include
)

# canbus-common .so imported target
set(CANBUS_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party-library/canbus-common")


add_library(canbus-common SHARED IMPORTED)

set_target_properties(canbus-common PROPERTIES
  IMPORTED_LOCATION "${CANBUS_COMMON_DIR}/libcanbus_common.so"
  INTERFACE_INCLUDE_DIRECTORIES "${CANBUS_COMMON_DIR}"
)

# link
target_link_libraries(CANSecKod PUBLIC canbus-common)


# for testing we link the dynamic library 
add_executable(counter_test tests/counter_test.cpp)
target_link_libraries(counter_test PRIVATE CANSecKod)

add_executable(canxl_sender tests/CANXL.sender.cpp)
target_link_libraries(canxl_sender PRIVATE CANSecKod Threads::Threads)
target_include_directories(canxl_sender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CANXLStructure)

add_executable(canxl_receiver tests/CANXL_receiver.cpp)
target_link_libraries(canxl_receiver PRIVATE CANSecKod Threads::Threads)
target_include_directories(canxl_receiver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CANXLStructure)

install(TARGETS counter_test
    RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin
)

set_target_properties(counter_test PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)
