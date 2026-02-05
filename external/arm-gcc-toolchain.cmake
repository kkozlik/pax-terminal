# Nastavení cílového systému
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Cesty k toolchainu (pokud ho nemáš v PATH, uveď absolutní cesty)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# Důležité: Přeskoč testování linkeru vytvořením statické knihovny
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY) # nutné pro bare-metal (jinak CMake zkouší link)

# Specifické příznaky pro tvůj procesor (uprav podle potřeby, např. Cortex-M4)
# Zde jsou obecné optimalizace pro velikost
set(FLAGS "-mcpu=arm1176jzf-s -marm -mfloat-abi=soft -Os -ffunction-sections -fdata-sections")

set(CMAKE_C_FLAGS "${FLAGS}" CACHE STRING "")
set(CMAKE_CXX_FLAGS "${FLAGS}" CACHE STRING "")
set(CMAKE_ASM_FLAGS "${FLAGS}" CACHE STRING "")

# Nastavení vyhledávání (zakáže hledat v systémových cestách hostitele)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
