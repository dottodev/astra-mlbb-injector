plugins {
    id("com.android.library")
}

android {
    namespace = "com.astra.core"
    compileSdk = 34

    defaultConfig {
        minSdk = 26
        ndk {
            abiFilters += "arm64-v8a"
            ndkVersion = "25.1.8937393"
        }
        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++17"
                arguments += "-DANDROID_STL=none"
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}