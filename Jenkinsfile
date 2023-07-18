pipeline {
     agent any
     stages {
         stage('Build against Ubuntu') {
            agent {
                docker {
                    image 'ubuntu:latest'
                    args '-u root:root'
                }
            }
            steps {
                sh 'apt-get update'
                sh 'apt-get install make -y'
                sh 'make -version'
            }
         }
         stage('Build against Alpine Jenkins SSH Agent') {
            agent {
                docker { image 'jenkins/ssh-agent:alpine' }
            }
            steps {
                sh 'make -version'
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