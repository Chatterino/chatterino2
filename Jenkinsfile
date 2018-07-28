pipeline {                                                                                                                                               
    agent any                                                            
                                                                         
    stages {                                                             
        stage('Build') {                                                 
            steps {                                                      
                echo 'Building..'                                        
                sh 'mkdir -p build && cd build && qmake .. && make'      
                echo 'lol'                                               
            }                                                            
        }                                                                
        stage('Test') {                                                  
            steps {                                                      
                echo 'Testing..'                                         
            }                                                            
        }                                                                
        stage('Deploy') {                                                
            steps {                                                      
                echo 'Deploying....'                                     
            }                                                            
        }                                                                
    }                                                                    
}      
