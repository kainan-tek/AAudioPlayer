plugins {
    alias(libs.plugins.androidApplication)
}

android {
    namespace = "com.example.aaudioplayer"
    compileSdk = 36
    compileSdkMinor = 1
    ndkVersion = "29.0.14206865"
    buildToolsVersion = "36.1.0"

    defaultConfig {
        applicationId = "com.example.aaudioplayer"
        minSdk = 32  // AAudio requires API 26+
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