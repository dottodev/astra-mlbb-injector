plugins {
    id("com.android.application")
}

android {
    namespace = "com.astra.injector"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.astra.injector"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"
        ndk {
            abiFilters += "arm64-v8a"
            ndkVersion = "25.1.8937393"
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    testOptions {
        unitTests.all {
            it.useJUnit()
        }
    }
}

dependencies {
    implementation(project(":core"))
    implementation("androidx.core:core:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("org.jetbrains.kotlin:kotlin-stdlib:1.9.24")
    testImplementation("junit:junit:4.13.2")
}