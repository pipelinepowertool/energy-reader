pipeline {
     agent any
     stages {
         stage('Build against Ubuntu') {
            agent {
                docker {
                    image 'sdenboer/ubuntu-make:latest'
                }
            }
            steps {
                sh 'make -version'
                sh 'ls'
            }
         }
         stage('Build against Alpine Jenkins SSH Agent') {
            agent {
                docker {
                    image 'sdenboer/jenkins-alpine-make:latest'
                    args '-u root:root'
                }
            }
            steps {
                sh 'make -version'
                sh 'ls'
            }
         }
//          stage('Upload to AWS') {
//               steps {
//                   withAWS(region:'eu-north-1',credentials:'jenkins-s3') {
//                   sh 'echo "Uploading content with AWS creds"'
//                       s3Upload(pathStyleAccessEnabled: true, payloadSigningEnabled: true, file:'app.py', bucket:'energy-reader')
//                   }
//               }
//          }
     }
}