plugins {
    alias(libs.plugins.androidApplication)
}

android {
    namespace = "com.example.aaudioplayer"
    compileSdk = 36
    ndkVersion = "29.0.14206865"
    buildToolsVersion = "36.0.0"

    defaultConfig {
        applicationId = "com.example.aaudioplayer"
        minSdk = 32
        versionCode = 20200
        versionName = "2.2.0"
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