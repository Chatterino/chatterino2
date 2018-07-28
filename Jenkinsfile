pipeline {
    agent any

    stages {
        stage('Build') {
            parallel {
                stage('GCC') {
                    sh 'mkdir -p build-linux-gcc && cd build && qmake .. && make'
                }
                stage('Clang') {
                    sh 'mkdir -p build-linux-clang && cd build && qmake -spec linux-clang .. && make'
                }
            }
        }

        stage('Run Tests') {
            echo 'lol'
        }
    }
}
