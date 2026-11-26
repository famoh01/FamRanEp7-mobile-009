cmake_minimum_required(VERSION 3.22)
project(RANMobile LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RANMOBILE_SOURCES
    Engine/Core/Logger.cpp
    Engine/Core/RANMobileEngine.cpp
    Engine/Renderer/MobileRenderer.cpp
    Engine/Renderer/RenderMath.cpp
    Engine/Renderer/ShaderPipeline.cpp
    Engine/Renderer/EngineeringTestAssets.cpp
    Engine/Renderer/EngineeringTestRenderer.cpp
    Engine/Resource/ResourceManager.cpp
    Engine/Touch/TouchSystem.cpp
    Engine/Camera/MobileCamera.cpp
    Engine/Animation/AnimationManager.cpp
    Engine/Audio/AudioManager.cpp
    Engine/Network/NetworkClient.cpp
    Engine/Network/EP7Tea.cpp
    Data/Validator/DataValidator.cpp
    Data/Importer/EP7AssetImporter.cpp
    Data/Importer/EP7TextureDecoder.cpp
    Data/Importer/EP7WorldCharParser.cpp
    Data/Converter/RANMobileDataConverter.cpp
    Data/Loader/GameDataLoader.cpp
    Data/Loader/ThaiPackLoader.cpp
    Client/Login/AuthenticationManager.cpp
    Client/Character/CharacterSelectManager.cpp
    Client/Gameplay/CharacterController.cpp
    Client/Combat/TargetManager.cpp
    Client/Combat/CombatController.cpp
    Client/Inventory/InventoryManager.cpp
    Client/Quest/QuestManager.cpp
    Client/Map/MapManager.cpp
    Client/Chat/ChatManager.cpp
    Client/Input/MovementInputController.cpp
    UI/UITheme.h
    UI/Components/UIButton.h
    UI/Components/UIButton.cpp
    UI/Components/UIInputField.h
    UI/Components/UIInputField.cpp
    UI/Login/LoginScreen.h
    UI/Login/LoginScreen.cpp
    UI/CharacterSelect/CharacterSelectScreen.h
    UI/CharacterSelect/CharacterSelectScreen.cpp
    UI/Joystick/VirtualJoystick.cpp
    UI/Buttons/MobileButton.cpp
    UI/Buttons/MobileSkillButton.cpp
    UI/HUD/MobileHUD.cpp
    UI/Quest/QuestUI.cpp
    Server/MobileServerHost.cpp
)

if(ANDROID)
    if(CMAKE_ANDROID_NDK)
        set(NDK_ROOT "${CMAKE_ANDROID_NDK}")
    elseif(ANDROID_NDK)
        set(NDK_ROOT "${ANDROID_NDK}")
    elseif(DEFINED ENV{ANDROID_NDK_HOME})
        set(NDK_ROOT "$ENV{ANDROID_NDK_HOME}")
    endif()

    set(NATIVE_APP_GLUE_DIR "${NDK_ROOT}/sources/android/native_app_glue")
    if(NOT EXISTS "${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c")
        message(FATAL_ERROR "android_native_app_glue.c not found at: ${NATIVE_APP_GLUE_DIR}")
    endif()

    add_library(ranmobile SHARED
        ${RANMOBILE_SOURCES}
        Android/native_bridge.cpp
        "${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c"
    )
    target_include_directories(ranmobile PRIVATE "${NATIVE_APP_GLUE_DIR}")
    target_link_libraries(ranmobile
        android
        log
        EGL
        GLESv3
    )
else()
    add_library(ranmobile_core STATIC ${RANMOBILE_SOURCES})
    add_executable(ranmobile_tests Tests/test_main.cpp)
    target_link_libraries(ranmobile_tests PRIVATE ranmobile_core pthread)
    # Protocol test: no external assets required (offline test vectors).
    add_executable(ranmobile_stream_tests Tests/test_stream.cpp)
    target_link_libraries(ranmobile_stream_tests PRIVATE ranmobile_core pthread)
    add_executable(ranmobile_protocol_tests Tests/test_protocol.cpp)
    target_link_libraries(ranmobile_protocol_tests PRIVATE ranmobile_core pthread)
    # Protocol hardening tests (Phase 3.4, offline, no assets)
    add_executable(ranmobile_protocol_hardening_tests Tests/test_protocol_hardening.cpp)
    target_link_libraries(ranmobile_protocol_hardening_tests PRIVATE ranmobile_core pthread)
    # Phase 3.2 input binding vertical slice (offline, no assets)
    add_executable(ranmobile_input_tests Tests/test_input_binding.cpp)
    target_link_libraries(ranmobile_input_tests PRIVATE ranmobile_core pthread)
    # PR-A/PR-B: renderer foundation (CPU-side validation; GLES runtime on Android)
    add_executable(ranmobile_renderer_math_tests Tests/test_renderer_math.cpp)
    target_link_libraries(ranmobile_renderer_math_tests PRIVATE ranmobile_core pthread)
    add_executable(ranmobile_engineering_assets_tests Tests/test_engineering_assets.cpp)
    target_link_libraries(ranmobile_engineering_assets_tests PRIVATE ranmobile_core pthread)
    # Step 2: synthetic binary header fixtures (no game content)
    add_executable(ranmobile_synthetic_fixture_tests Tests/test_synthetic_fixtures.cpp)
    target_link_libraries(ranmobile_synthetic_fixture_tests PRIVATE ranmobile_core pthread)
    # Phase 5: UI redesign unit tests (layout/state/tokens, no GLES runtime)
    add_executable(ranmobile_ui_phase5_tests Tests/test_ui_phase5.cpp)
    target_link_libraries(ranmobile_ui_phase5_tests PRIVATE ranmobile_core pthread)
    # Phase 5 ext: Character Select UI shell (mock labels only, no real game data)
    add_executable(ranmobile_character_select_tests Tests/test_character_select.cpp)
    target_link_libraries(ranmobile_character_select_tests PRIVATE ranmobile_core pthread)
endif()
