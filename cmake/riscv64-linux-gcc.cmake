set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# set(CMAKE_C_COMPILER /nfs/home/share/riscv-v/bin/riscv64-unknown-linux-gnu-gcc)
# set(CMAKE_CXX_COMPILER /nfs/home/share/riscv-v/bin/riscv64-unknown-linux-gnu-g++)
set(CMAKE_C_COMPILER /nfs/home/share/riscv-toolchain-gcc15-240613/bin/riscv64-unknown-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /nfs/home/share/riscv-toolchain-gcc15-240613/bin/riscv64-unknown-linux-gnu-g++)

set( target riscv64-unknown-linux-gnu )

set( CMAKE_C_COMPILER_TARGET   ${target} )
set( CMAKE_CXX_COMPILER_TARGET ${target} )

set( arch_c_flags "-march=rv64gc" )
# set( warn_c_flags "-Wno-format -Wno-unused-variable -Wno-unused-function" )

set( CMAKE_C_FLAGS   "${arch_c_flags}" )
set( CMAKE_CXX_FLAGS "${arch_c_flags}" )
