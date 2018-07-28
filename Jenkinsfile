pipeline {
    agent any

    stages {
        stage('Build') {
            parallel {
                stage('GCC') {
                    steps {
                        sh 'mkdir -p build-linux-gcc && cd build && qmake .. && make'
                    }
                }
                stage('Clang') {
                    steps {
                        sh 'mkdir -p build-linux-clang && cd build && qmake -spec linux-clang .. && make'
                    }
                }
            }
        }

        stage('Run Tests') {
            echo 'lol'
        }
    }
}
