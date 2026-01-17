plugins {
    alias(libs.plugins.androidApplication)
}

android {
    namespace = "com.example.aaudioplayer"

    defaultConfig {
        applicationId = "com.example.aaudioplayer"
        minSdk = 32
        versionCode = 1
        versionName = "1.0"
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
        }
    }
    buildFeatures {
        viewBinding = true
    }
    compileSdk = 36
    ndkVersion = "29.0.14206865"
    buildToolsVersion = "36.1.0"
    compileSdkMinor = 1
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.constraintlayout)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
}